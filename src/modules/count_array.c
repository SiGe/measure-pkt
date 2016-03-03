#include "rte_ethdev.h"
#include "rte_ip.h"
#include "rte_malloc.h"
#include "rte_mbuf.h"

#include "../common.h"
#include "../vendor/murmur3.h"
#include "../module.h"
#include "../net.h"

#include "count_array.h"

ModuleCountArrayPtr count_array_init(uint32_t size) {
    ModuleCountArrayPtr module = rte_zmalloc(0,
                sizeof(struct Module) + 
                sizeof(uint32_t) + 
                sizeof(uint32_t) * MAX_PKT_BURST + 
                size * sizeof(Counter), 64); 

    module->_m.execute = count_array_execute;
    module->size  = size;
    return module;
}

void count_array_delete(ModulePtr module) {
    rte_free(module);
}

#define get_ipv4(mbuf) ((struct ipv4_hdr const*) \
        ((rte_pktmbuf_mtod(pkts[i], uint8_t const*)) + \
        (sizeof(struct ether_addr))))

inline void
count_array_execute(
        ModulePtr module_,
        PortPtr port __attribute__((unused)),
        void const *pkts,
        HopscotchHashPtr htable) {
    ModuleCountArrayPtr module = (ModuleCountArrayPtr)module_;

    uint32_t    size = module->size;
    Counter   *table = module->table;
    uint32_t *hashes = module->hashes;

    uint32_t i = 0, hash = 0;
    void const *ptr = pkts;

    for (i = 0; i < HOPSCOTCH_SWEEP_SIZE; ++i) {
        ptr = hopscotch_next(htable, ptr);
        if (likely(!hopscotch_is_set(ptr))) continue;
        MurmurHash3_x86_32(ptr, 8, 1, &hash);
        hash = hash & size;
        rte_prefetch0(&table[hash]);
        hashes[i] = hash;
    }

    ptr = pkts;
    for (i = 0; i < HOPSCOTCH_SWEEP_SIZE; ++i) {
        ptr = hopscotch_next(htable, ptr);
        if (likely(!hopscotch_is_set(ptr))) continue;
        table[hashes[i]] += hopscotch_count(ptr);
    }

    /*

    while (ptr < end) {
        for (; ptr < end; ptr = hopscotch_next(htable, ptr)) {
            if (likely(!hopscotch_set(ptr))) continue;
            // MurmurHash3_x86_32(ptr, 8, 1, &hash);
            hash = hash & size;
            rte_prefetch0(&table[hash]);
            hashes[hash_count] = hash;
            hash_count += 1;

            if (hash_count == PREFETCH_SIZE)
                break;
        }

        for (i = 0; i < hash_count; ++i) {
            table[hashes[i]]++;
        }

        hash_count = 0;
    }
    */
}
