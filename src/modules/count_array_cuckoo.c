#include <stdio.h>
#include <stdlib.h>

#include "rte_cycles.h"
#include "rte_ethdev.h"
#include "rte_hash.h"
#include "rte_ip.h"
#include "rte_malloc.h"
#include "rte_mbuf.h"

#include "../common.h"
#include "../vendor/murmur3.h"
#include "../module.h"
#include "../net.h"
#include "../reporter.h"

#include "count_array_cuckoo.h"

static inline uint32_t
murmur3_func(const void *key, uint32_t keylen, uint32_t init) {
    uint32_t hash;
    (void)(init);
    MurmurHash3_x86_32(key, keylen, 1, &hash);
    return hash;
}

ModuleCountArrayCuckooPtr count_array_cuckoo_init(
        uint32_t size, unsigned keysize,
        unsigned elsize, unsigned socket, ReporterPtr reporter) {

    struct rte_hash_parameters hash_params = {
        .entries = size,
        .key_len  = keysize,
        .socket_id = socket,
        .hash_func = murmur3_func,
    };

    ModuleCountArrayCuckooPtr module = rte_zmalloc_socket(0,
            sizeof(struct ModuleCountArrayCuckoo) +
            size * elsize * 4, 64, socket); 

    module->_m.execute = count_array_cuckoo_execute;
    module->size  = size;
    module->keysize = keysize;
    module->elsize = elsize;
    module->socket = socket;
    module->reporter = reporter;

    module->hashmap = rte_hash_create(&hash_params);

    return module;
}

void count_array_cuckoo_delete(ModulePtr module_) {
    ModuleCountArrayCuckooPtr module = (ModuleCountArrayCuckooPtr)module_;
    rte_hash_free(module->hashmap);
    rte_free(module);
}

#define get_ipv4(mbuf) ((struct ipv4_hdr const*) \
        ((rte_pktmbuf_mtod(pkts[i], uint8_t const*)) + \
        (sizeof(struct ether_addr))))

inline void
count_array_cuckoo_execute(
        ModulePtr module_,
        PortPtr port __attribute__((unused)),
        struct rte_mbuf ** __restrict__ pkts,
        uint32_t count) {
    (void)(port);

    ModuleCountArrayCuckooPtr module = (ModuleCountArrayCuckooPtr)module_;
    uint16_t i = 0;
    uint64_t timer = rte_get_tsc_cycles();
    void *ptrs[MAX_PKT_BURST];
    struct rte_hash *hashmap = module->hashmap;
    uint8_t *end = module->counters;
    unsigned elsize = module->elsize;

    for (i = 0; i < count; ++i) {
        uint8_t const* pkt = rte_pktmbuf_mtod(pkts[i], uint8_t const*);
        int key = rte_hash_add_key(hashmap, pkt+26);

        void *ptr = end + (elsize * 4 * key);

        ptrs[i] = ptr;
        rte_prefetch0(ptr);
    }


    ReporterPtr reporter = module->reporter;
    unsigned keysize = module->keysize;

    for (i = 0; i < count; ++i) { 
        void *ptr = ptrs[i];
        uint32_t *bc = (uint32_t*)(ptr); (*bc)++;

        if (*bc == HEAVY_HITTER_THRESHOLD) {
            reporter_add_entry(reporter, ((uint8_t*)ptr) - (keysize * 4));
        }

        uint64_t *time = (uint64_t*)(bc + 1);
        *time = timer;
    }
}
