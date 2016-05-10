#include "rte_prefetch.h"

#include "string.h"

#include "hash.h"
#include "bloomfilter.h"

int bloomfilter_add_key(BFPropPtr bfp, void *bf, void const *key) {
    rte_prefetch0(bf);

    uint32_t *bfa = (uint32_t*)bf;

    uint32_t hash = dss_hash(key, bfp->keylen);
    uint16_t h1 = (hash      ) & 0b1111111111;
    uint16_t h2 = (hash >> 10) & 0b1111111111;
    uint16_t h3 = (hash >> 20) & 0b1111111111;

    uint16_t idx1 = h1 >> 5;
    uint16_t idx2 = h2 >> 5;
    uint16_t idx3 = h3 >> 5;
    
    uint32_t *bf1 = bfa + idx1;
    uint32_t *bf2 = bfa + idx2;
    uint32_t *bf3 = bfa + idx3;

    uint32_t off1 = (1<<(h1 & 0b11111));
    uint32_t off2 = (1<<(h2 & 0b11111));
    uint32_t off3 = (1<<(h3 & 0b11111));

    int is_member = (*bf1 & off1) && (*bf2 & off2) && (*bf3 & off3);

    *bf1 = *bf1 | off1;
    *bf2 = *bf2 | off2;
    *bf3 = *bf3 | off3;

    return is_member;
}

void bloomfilter_reset(BFPropPtr bfp __attribute__((unused)), void *bf) {
    rte_prefetch0(bf);
    memset(bf, 0, 1024 / sizeof(uint8_t));
}

int bloomfilter_is_member(BFPropPtr bfp, void *bf, void const *key) {
    rte_prefetch0(bf);

    uint32_t *bfa = (uint32_t*)bf;

    uint32_t hash = dss_hash(key, bfp->keylen);
    uint16_t h1 = (hash      ) & 0b1111111111;
    uint16_t h2 = (hash >> 10) & 0b1111111111;
    uint16_t h3 = (hash >> 20) & 0b1111111111;

    uint16_t idx1 = h1 >> 5;
    uint16_t idx2 = h2 >> 5;
    uint16_t idx3 = h3 >> 5;
    
    uint32_t *bf1 = bfa + idx1;
    uint32_t *bf2 = bfa + idx2;
    uint32_t *bf3 = bfa + idx3;

    uint32_t off1 = (1<<(h1 & 0b11111));
    uint32_t off2 = (1<<(h2 & 0b11111));
    uint32_t off3 = (1<<(h3 & 0b11111));

    return (*bf1 & off1) && (*bf2 & off2) && (*bf3 & off3);
}
