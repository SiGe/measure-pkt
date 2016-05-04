#ifndef _DSS_HASH_H_
#define _DSS_HASH_H_

#include "../vendor/murmur3.h"

inline static uint32_t
dss_hash(void const* key, uint16_t keysize) {
    uint32_t hash;
    MurmurHash3_x86_32(key, keysize * 4, 1, &hash);
    return hash;
}

#endif
