#include "rte_ethdev.h"
#include "rte_ip.h"
#include "rte_malloc.h"
#include "rte_mbuf.h"

#include "../common.h"
#include "../vendor/murmur3.h"
#include "../module.h"
#include "../net.h"

#include "super_spreader.h"


ModuleSuperSpreaderPtr super_spreader_init(uint32_t size) {
    ModuleSuperSpreaderPtr module = rte_zmalloc(0,
                sizeof(struct Module) + 
                sizeof(uint32_t) + 
                sizeof(uint32_t) * MAX_PKT_BURST + 
                size * sizeof(struct SSField), 64); 
    module->_m.execute = super_spreader_execute;
    module->size  = size;
    return module;
}

void super_spreader_delete(ModulePtr module) {
    rte_free(module);
}

#define get_ipv4(mbuf) ((struct ipv4_hdr const*) \
        ((rte_pktmbuf_mtod(pkts[i], uint8_t const*)) + \
        (sizeof(struct ether_addr))))

#define bit_hash(bit, ip) {\
            bit = (ip[0] ^ ip[1] ^ ip[2] ^ ip[3]);\
            bit ^= (bit >> 2);\
            bit &= 63;\
        }

__attribute__((optimize("unroll-loops")))
inline void
super_spreader_execute(
        ModulePtr module_,
        PortPtr port __attribute__((unused)),
        void const **pkts,
        uint32_t count) {

    ModuleSuperSpreaderPtr module = (ModuleSuperSpreaderPtr)module_;
    uint32_t size = module->size;
    struct SSField *table = module->table;
    uint32_t *hashes = module->hashes;

    uint32_t hash = 0;
    void *ptr = hopscotch_first(htable);
    void const *end = hopscotch_end(htable);
    uint16_t hash_count = 0;

    while (ptr < end) {
        for (; ptr < end; ptr = hopscotch_next(htable, ptr)) {
            if (likely(!hopscotch_set(ptr))) continue;
            // MurmurHash3_x86_32(ptr, 4, 1, &hash);
            hash = hash & size;
            rte_prefetch0(&table[hash]);
            hashes[hash_count] = hash;
            hash_count += 1;

            if (hash_count == PREFETCH_SIZE)
                break;
        }

        // for (i = 0; i < hash_count; ++i) {
        //     dstip = (uint8_t const*)&(hdr->dst_addr);
        //     field->src = srcip;
        //     bit_hash(bit, dstip);
        //     bit_field = 1 << bit;
        //     seen = field->bits & bit_field;
        //     field->bits |= bit_field;
        //     field->count += (seen == 0);
        //     table[hashes[i]]++;

        //     if (field->count > 32)
        //         printf("Superspreader: %d, %d\n", srcip, field->count);
        // }
        hash_count = 0;
    }
}
