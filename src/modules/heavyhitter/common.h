#ifndef _COUNT_ARRAY_COMMON_H_
#define _COUNT_ARRAY_COMMON_H_

#include <inttypes.h>
#include <stdbool.h>

#include "rte_memcpy.h"

static inline
bool heavyhitter_copy_and_inc(void *key, void const *pkt, uint16_t elsize) {
    uint32_t *counter = (uint32_t*)key; (*counter)++;
    rte_memcpy(key, pkt, (elsize-1)*4);
    return (*counter > HEAVY_HITTER_THRESHOLD);
}

#endif
