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

#ifndef _HASHMAP_H_
#define _HASHMAP_H_

/* Implementation of a hashmap with no collision resolution. If collisions
 * happen we simply overwrite */
typedef struct HashMap* HashMapPtr; 

/* Create a new hashmap ... row and key sizes are in multiples of 4 bytes -- for
 * better performance due to alignment */
HashMapPtr hashmap_create(uint32_t, uint16_t, uint16_t, int);

inline void *hashmap_get_copy_key(HashMapPtr, void const *);
inline void *hashmap_get_nocopy_key(HashMapPtr, void const *);
inline void *hashmap_get_with_hash(HashMapPtr, uint32_t);
inline void hashmap_delete(HashMapPtr);

inline void *hashmap_begin(HashMapPtr);
inline void *hashmap_end(HashMapPtr);
inline void *hashmap_next(HashMapPtr, void *);

uint32_t hashmap_size(HashMapPtr);
uint32_t hashmap_count(HashMapPtr);
inline void hashmap_reset(HashMapPtr);
uint32_t hashmap_num_searches(HashMapPtr);

#endif // _HASHMAP_H_
