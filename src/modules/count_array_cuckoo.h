/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Omid Alipourfard <g@omid.io>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _COUNT_ARRAY_CUCKOO_H_
#define _COUNT_ARRAY_CUCKOO_H_

#include "../common.h"
#include "../module.h"
#include "../net.h"
#include "../reporter.h"

typedef uint32_t Counter;
struct ModuleCountArrayCuckoo {
    struct Module _m;

    uint32_t  size;
    unsigned  keysize;
    unsigned  elsize;
    unsigned  socket;

    struct rte_hash *hashmap;
    uint8_t *counters;

    ReporterPtr reporter;

    struct rte_hash *key_buf1;
    struct rte_hash *key_buf2;

    uint8_t *val_buf1;
    uint8_t *val_buf2;
};

typedef struct ModuleCountArrayCuckoo* ModuleCountArrayCuckooPtr;

ModuleCountArrayCuckooPtr count_array_cuckoo_init(uint32_t, unsigned,
        unsigned, unsigned, ReporterPtr);

void count_array_cuckoo_delete(ModulePtr);
void count_array_cuckoo_execute(ModulePtr, PortPtr, struct rte_mbuf **, uint32_t);

void count_array_cuckoo_reset(ModulePtr);

#endif
