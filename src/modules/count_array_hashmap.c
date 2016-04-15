#include <stdio.h>
#include <stdlib.h>

#include "rte_cycles.h"
#include "rte_ethdev.h"
#include "rte_ip.h"
#include "rte_malloc.h"
#include "rte_mbuf.h"

#include "../common.h"
#include "../vendor/murmur3.h"
#include "../module.h"
#include "../net.h"
#include "../reporter.h"

#include "../dss/hashmap.h"
#include "count_array_hashmap.h"

ModuleCountArrayHashmapPtr count_array_hashmap_init(
        uint32_t size, unsigned keysize,
        unsigned elsize, unsigned socket, ReporterPtr reporter) {
    ModuleCountArrayHashmapPtr module = rte_zmalloc_socket(0,
            sizeof(struct ModuleCountArrayHashmap), 64, socket); 

    module->_m.execute = count_array_hashmap_execute;
    module->size  = size;
    module->keysize = keysize;
    module->elsize = elsize;
    module->socket = socket;
    module->reporter = reporter;

    module->hashmap = hashmap_create(size, keysize, elsize, socket);

    return module;
}

void count_array_hashmap_delete(ModulePtr module_) {
    ModuleCountArrayHashmapPtr module = (ModuleCountArrayHashmapPtr)module_;
    hashmap_delete(module->hashmap);
    rte_free(module);
}

#define get_ipv4(mbuf) ((struct ipv4_hdr const*) \
        ((rte_pktmbuf_mtod(pkts[i], uint8_t const*)) + \
        (sizeof(struct ether_addr))))

inline void
count_array_hashmap_execute(
        ModulePtr module_,
        PortPtr port __attribute__((unused)),
        struct rte_mbuf ** __restrict__ pkts,
        uint32_t count) {
    (void)(port);

    ModuleCountArrayHashmapPtr module = (ModuleCountArrayHashmapPtr)module_;
    uint16_t i = 0;
    uint64_t timer = rte_get_tsc_cycles();
    void *ptrs[MAX_PKT_BURST];
    HashMapPtr hashmap = module->hashmap;

    for (i = 0; i < count; ++i) {
        uint8_t const* pkt = rte_pktmbuf_mtod(pkts[i], uint8_t const*);
        void *ptr = hashmap_get_copy_key(hashmap, (pkt + 26));
        //void *ptr = hashmap_get_with_hash(hashmap, pkts[i]->hash.rss);

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
