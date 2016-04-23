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

#include "../dss/hashmap_linear.h"
#include "count_array_hashmap_linear.h"

ModulePtr count_array_hashmap_linear_init(ModuleConfigPtr conf) {
    uint32_t size    = mc_uint32_get(conf, "size");
    uint32_t keysize = mc_uint32_get(conf, "keysize");
    uint32_t elsize  = mc_uint32_get(conf, "valsize");
    uint32_t socket  = mc_uint32_get(conf, "socket");

    ReporterPtr reporter = reporter_init(
            mc_uint32_get(conf, "reporter-size"), 
            keysize * 4, socket,
            mc_string_get(conf, "file-prefix"));

    ModuleCountArrayHashmapLinearPtr module = rte_zmalloc_socket(0,
            sizeof(struct ModuleCountArrayHashmapLinear), 64, socket); 

    module->_m.execute = count_array_hashmap_linear_execute;
    module->size  = size;
    module->keysize = keysize;
    module->elsize = elsize;
    module->socket = socket;
    module->reporter = reporter;

    module->hashmap_linear_ptr1 = hashmap_linear_create(size, keysize, elsize, socket);
    module->hashmap_linear_ptr2 = hashmap_linear_create(size, keysize, elsize, socket);

    module->hashmap_linear = module->hashmap_linear_ptr1;

    return (ModulePtr)module;
}

void count_array_hashmap_linear_delete(ModulePtr module_) {
    ModuleCountArrayHashmapLinearPtr module = (ModuleCountArrayHashmapLinearPtr)module_;

    hashmap_linear_delete(module->hashmap_linear_ptr1);
    hashmap_linear_delete(module->hashmap_linear_ptr2);

    rte_free(module);
}

#define get_ipv4(mbuf) ((struct ipv4_hdr const*) \
        ((rte_pktmbuf_mtod(pkts[i], uint8_t const*)) + \
        (sizeof(struct ether_addr))))

inline void
count_array_hashmap_linear_execute(
        ModulePtr module_,
        PortPtr port __attribute__((unused)),
        struct rte_mbuf ** __restrict__ pkts,
        uint32_t count) {
    (void)(port);

    ModuleCountArrayHashmapLinearPtr module = (ModuleCountArrayHashmapLinearPtr)module_;
    uint16_t i = 0;
    uint64_t timer = rte_get_tsc_cycles();
    void *ptrs[MAX_PKT_BURST];
    HashMapLinearPtr hashmap_linear = module->hashmap_linear;

    /* Prefetch hashmap_linear entries */
    for (i = 0; i < count; ++i) {
        uint8_t const* pkt = rte_pktmbuf_mtod(pkts[i], uint8_t const*);
        void *ptr = hashmap_linear_get_copy_key(hashmap_linear, (pkt + 26));

        ptrs[i] = ptr;
        rte_prefetch0(ptr);
    }

    /* Save and report if necessary */
    ReporterPtr reporter = module->reporter;
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

inline void
count_array_hashmap_linear_reset(ModulePtr module_) {
    ModuleCountArrayHashmapLinearPtr module = (ModuleCountArrayHashmapLinearPtr)module_;
    HashMapLinearPtr prev = module->hashmap_linear;
    reporter_tick(module->reporter);

    if (module->hashmap_linear == module->hashmap_linear_ptr1) {
        module->hashmap_linear = module->hashmap_linear_ptr2;
    } else {
        module->hashmap_linear = module->hashmap_linear_ptr1;
    }

    hashmap_linear_reset(prev);
}
