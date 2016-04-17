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

#ifndef _COUNT_ARRAY_HASHMAP_H_
#define _COUNT_ARRAY_HASHMAP_H_

#include "../common.h"
#include "../module.h"
#include "../net.h"
#include "../reporter.h"

#include "../dss/hashmap.h"

typedef uint32_t Counter;
struct ModuleCountArrayHashmap {
    struct Module _m;

    uint32_t  size;
    unsigned  keysize;
    unsigned  elsize;
    unsigned  socket;

    ReporterPtr reporter;
    HashMapPtr hashmap;
};

typedef struct ModuleCountArrayHashmap* ModuleCountArrayHashmapPtr;

ModuleCountArrayHashmapPtr count_array_hashmap_init(uint32_t, unsigned,
        unsigned, unsigned, ReporterPtr);

void count_array_hashmap_delete(ModulePtr);
void count_array_hashmap_execute(ModulePtr, PortPtr, struct rte_mbuf **, uint32_t);
void count_array_hashmap_reset(ModulePtr);

#endif