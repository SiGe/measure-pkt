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
#include "../pkt.h"
#include "../reporter.h"

#include "count_array_cuckoo.h"

inline static uint32_t
murmur3_func(const void *key, uint32_t keylen, uint32_t init) {
    uint32_t hash;
    (void)(init);
    MurmurHash3_x86_32(key, keylen, 1, &hash);
    return hash;
}

inline static unsigned
val_space_size(ModuleCountArrayCuckooPtr m) {
    return m->size * m->elsize * 4;
}

ModulePtr count_array_cuckoo_init(ModuleConfigPtr conf) {
    uint32_t size    = mc_uint32_get(conf, "size");
    uint32_t keysize = mc_uint32_get(conf, "keysize");
    uint32_t elsize  = mc_uint32_get(conf, "valsize");
    uint32_t socket  = mc_uint32_get(conf, "socket");

    ReporterPtr reporter = reporter_init(
            mc_uint32_get(conf, "reporter-size"), 
            keysize * 4, socket,
            mc_string_get(conf, "file-prefix"));

    struct rte_hash_parameters hash_params_1 = {
        .name = "cuckoo_1",
        .entries = size,
        .key_len  = keysize * 4,
        .socket_id = socket,
        .hash_func = murmur3_func,
    };

    struct rte_hash_parameters hash_params_2 = {
        .name = "cuckoo_2",
        .entries = size,
        .key_len  = keysize * 4,
        .socket_id = socket,
        .hash_func = murmur3_func,
    };

    ModuleCountArrayCuckooPtr module = rte_zmalloc_socket(0,
            sizeof(struct ModuleCountArrayCuckoo), 64, socket); 


    module->_m.execute = count_array_cuckoo_execute;
    module->size  = size;
    module->keysize = keysize;
    module->elsize = elsize;
    module->socket = socket;
    module->reporter = reporter;

    /* Create key/value stores */
    unsigned val_buf_size = val_space_size(module);

    module->val_buf1 = rte_zmalloc_socket(0, val_buf_size, 64, socket);
    module->val_buf2 = rte_zmalloc_socket(0, val_buf_size, 64, socket);
    module->counters = module->val_buf1;

    module->key_buf1 = rte_hash_create(&hash_params_1);
    module->key_buf2 = rte_hash_create(&hash_params_2);
    module->hashmap = module->key_buf1;

    return (ModulePtr)module;
}

void count_array_cuckoo_delete(ModulePtr module_) {
    ModuleCountArrayCuckooPtr module = (ModuleCountArrayCuckooPtr)module_;
    rte_free(module->val_buf1);
    rte_free(module->val_buf2);
    rte_hash_free(module->key_buf1);
    rte_hash_free(module->key_buf2);
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

    /* Prefetch hashmap entries */
    for (i = 0; i < count; ++i) {
        uint8_t const* pkt = rte_pktmbuf_mtod(pkts[i], uint8_t const*);
        int key = rte_hash_add_key(hashmap, pkt+26);

        void *ptr = end + (elsize * 4 * key);
        //printf("Key is: %d\n", key);
        //pkt_print(pkts[i]);

        ptrs[i] = ptr;
        rte_prefetch0(ptr);
    }

    ReporterPtr reporter = module->reporter;

    /* Save and report if necessary */
    for (i = 0; i < count; ++i) { 
        void *ptr = ptrs[i];
        uint32_t *bc = (uint32_t*)(ptr); (*bc)++;

        if (*bc == HEAVY_HITTER_THRESHOLD) {
            uint8_t const* pkt = rte_pktmbuf_mtod(pkts[i], uint8_t const*);
            reporter_add_entry(reporter, pkt+26);
        }

        uint64_t *time = (uint64_t*)(bc + 1);
        *time = timer;
    }
}

void
count_array_cuckoo_reset(ModulePtr module_) {
    ModuleCountArrayCuckooPtr module = (ModuleCountArrayCuckooPtr)module_;
    struct rte_hash *key_tmp = module->hashmap;
    reporter_tick(module->reporter);

    uint8_t *val_tmp = module->counters;

    if (module->hashmap == module->key_buf1) {
        module->hashmap = module->key_buf2;
        module->counters = module->val_buf2;
    } else {
        module->hashmap = module->key_buf1;
        module->counters = module->val_buf1;
    }

    rte_hash_reset(key_tmp);
    memset(val_tmp, 0, val_space_size(module));
}
