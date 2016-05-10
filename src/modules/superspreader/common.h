#ifndef _HEAVYHITTER_COMMON_H_
#define _HEAVYHITTER_COMMON_H_

#include <inttypes.h>
#include <stdbool.h>

#include "../../dss/bloomfilter.h"

#include "rte_memcpy.h"

static inline
bool superspreader_copy_and_inc(BFPropPtr prop, void *val, void const *pkt, uint16_t keysize) {
    uint32_t *bc = (uint32_t*)val;
    uint32_t ret = bloomfilter_add_key(prop, bc + 1, pkt + 26 + keysize*4);
    *bc += !ret;
    return (*bc == SUPERSPREADER_THRESHOLD);
}

#endif
