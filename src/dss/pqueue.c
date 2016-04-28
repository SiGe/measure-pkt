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

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "rte_hash.h"
#include "rte_jhash.h"
#include "rte_malloc.h"
#include "rte_memcpy.h"

#include "hashmap_cuckoo.h"
#include "pqueue.h"

struct PriorityQueue {
    uint32_t count;
    uint32_t size;
    uint32_t socket;
    uint16_t rowsize;
    uint16_t keysize;
    uint16_t elsize;
    uint16_t last_index;
    uint8_t  lock;

    //HashMapCuckooPtr hashmap;
    struct rte_hash  *hashmap;
    uint8_t          *values;
    pqueue_comp_func fcomp;
    uint8_t table[];
};

struct PriorityQueueNode {
    uint8_t  used;
    uint8_t  new;
    uint16_t index;
};

typedef struct PriorityQueueNode * PQNPtr;

/*
 * We use the first 4 byte of the value as the index into the priority queue
 */

static inline unsigned pq_size(unsigned size) {
    return sizeof(struct PriorityQueue) + (size * sizeof(uintptr_t));
}

static inline void
pq_create_hash(PriorityQueuePtr pq) {
    struct rte_hash_parameters params;
    char name[255];
    snprintf(name, 255, "hash-%d", pq->size*8);
    params.entries = pq->size * 8;
    params.key_len = pq->keysize * 4;
    params.socket_id = pq->socket;
    params.hash_func = rte_jhash;
    params.name = name;
    pq->hashmap = rte_hash_create(&params);
}

PriorityQueuePtr pqueue_create(unsigned size, uint16_t keysize, uint16_t elsize,
        unsigned socket, pqueue_comp_func func) {

    PriorityQueuePtr ptr = rte_zmalloc_socket(0, pq_size(size), 64, socket);
    ptr->size    = size;
    ptr->rowsize = keysize + elsize + 1;
    ptr->elsize  = elsize;
    ptr->keysize = keysize;
    ptr->socket  = socket;
    ptr->count   = 0;

    //ptr->hashmap = hashmap_cuckoo_create(size * 4, keysize, elsize+1, socket);
    pq_create_hash(ptr);

    ptr->values = rte_zmalloc_socket(0, sizeof(uint32_t) * \
            (ptr->elsize+1) * ptr->size * 8, 64, socket);

    ptr->fcomp   = func;
    ptr->last_index = 0;

    return ptr;
}

void pqueue_reset(PriorityQueuePtr pq) {
    memset(pq->table, 0, pq_size(pq->size));
    rte_hash_reset(pq->hashmap);
    memset(pq->values, 0, pq->size * 8 * (pq->elsize+1) * 4);
    pq->last_index = 0;
}

/* Save a pointer to the hash-table in the specified index */
inline static void pqueue_set_heap(PriorityQueuePtr pq, uint16_t idx, void const *value) {
    uintptr_t *pos = (uintptr_t*)(pq->table + (sizeof(uintptr_t) * idx));
    *pos = (uintptr_t)value;
}

/* Return the pointer to the hash-table in the specified index */
inline static void *pqueue_get_heap(PriorityQueuePtr pq, uint16_t idx) {
    uintptr_t* ptr = (uintptr_t*)(pq->table + (sizeof(uintptr_t) * idx));
    return (void*)(*ptr);
}


void *pqueue_get_copy_key(PriorityQueuePtr pq, void const *key, uint16_t *idx) {
    //HashMapCuckooPtr hm = pq->hashmap;
    //void *ret = hashmap_cuckoo_get_copy_key(hm, key);
    int32_t keyidx = rte_hash_add_key(pq->hashmap, key);
    void *ret = (pq->values + sizeof(uint32_t)*(pq->elsize+1)*keyidx);

    PQNPtr pqn = (PQNPtr)(ret);
    uint16_t is_new_node = !pqn->used;

    if (is_new_node && pq->last_index >= pq->size) {
        /* Remove the first element from the heap and add the new key */
        void *q = pqueue_get_heap(pq, 0);
        rte_memcpy(ret, q, (pq->elsize+1)*sizeof(uint32_t));
        pqueue_set_heap(pq, 0, ret);
        is_new_node = 0;
    }


    /* branch-less implementation */
    /* Set the PQueue node properties */
    pqn->index = pqn->index ^ ((pqn->index ^ pq->last_index) & -is_new_node);
    pqn->used  = 1;
    pqn->new   = is_new_node;

    /* Save the last index */
    pq->last_index += is_new_node;
    *idx            = pqn->index;

    /* Save the key in the heap -- we are going to run heapify on it */
    pqueue_set_heap(pq, pqn->index, ret);

    // Jump over the pqueue node
    return ((uint32_t*)ret)+1;
}

static inline
uint16_t pq_parent(uint16_t idx) {
    return ((idx - 1) >> 1);
}

static inline
uint16_t pq_left(uint16_t idx) {
    return (idx << 1) + 1;
}

static inline
uint16_t pq_right(uint16_t idx) {
    return (idx << 1) + 2;
}

/* Swap the pointers in the heap */
static inline
void pq_swap(PriorityQueuePtr pq, uint16_t aidx, uint16_t bidx) {
    uintptr_t *aptr = (uintptr_t*)(pq->table + (sizeof(uintptr_t) * aidx));
    uintptr_t *bptr = (uintptr_t*)(pq->table + (sizeof(uintptr_t) * bidx));
    uintptr_t tmp = *aptr;
    *aptr = *bptr;
    *bptr = tmp;
}

static inline
void pqueue_swap(PriorityQueuePtr pq, uint16_t aidx, uint16_t bidx,
        uint32_t *aptr, uint32_t *bptr) {
    //printf("Swapping: %d and %d\n", aidx, bidx);
    pq_swap(pq, aidx, bidx);
    ((PQNPtr)(aptr))->index = bidx;
    ((PQNPtr)(bptr))->index = aidx;
}

void pqueue_swim(PriorityQueuePtr pq, uint16_t idx) {
    //printf("Running swim.\n");
    while (idx != 0) {
        uint16_t pidx = pq_parent(idx);
        uint32_t *cur = (uint32_t*)pqueue_get_heap(pq, idx);
        uint32_t *prt = (uint32_t*)pqueue_get_heap(pq, pidx);

        int comp = pq->fcomp(cur + 1, prt + 1);
        if (comp <= 0) return;

        /* Swap the element with the prt */
        pqueue_swap(pq, idx, pidx, cur, prt);

        idx = pidx;
    }
}

void pqueue_sift(PriorityQueuePtr pq, uint16_t idx) {
    //printf("Running sift.\n");
    uint16_t last_index = pq->last_index;

    while (idx < last_index) {
        uint16_t lidx = pq_left(idx);
        uint16_t ridx = pq_right(idx);

        if (lidx >= last_index) return;

        uint32_t *cur = (uint32_t*)pqueue_get_heap(pq, idx);
        uint32_t *lft = (uint32_t*)pqueue_get_heap(pq, lidx);
        int ccl = pq->fcomp(cur + 1, lft + 1);

        if (ridx >= last_index) {
            if (ccl <= 0) { return; }
            /* swap left and current */
            pqueue_swap(pq, lidx, idx, lft, cur);

            /* Can't perculate any lower than this */
            return ;
        }

        uint32_t *rgt = (uint32_t*)pqueue_get_heap(pq, ridx);
        int ccr = pq->fcomp(cur + 1, rgt + 1);
        int clr = pq->fcomp(lft + 1, rgt + 1);

        if (ccl >= 0) {
            if (ccr >= 0) { return; }
            /* swap right and current */
            //printf("Swapping R and C\n");
            pqueue_swap(pq, idx, ridx, cur, rgt);
            idx = ridx;
        } else if (ccr >= 0) {
            if (ccl >= 0) { return; }
            /* swap left and current */
            //printf("Swapping L and C\n");
            pqueue_swap(pq, lidx, idx, lft, cur);
            idx = lidx;
        } else if (clr < 0) {
            /* swap right and current */
            //printf("Swapping R and C\n");
            pqueue_swap(pq, ridx, idx, rgt, cur);
            idx = ridx;
        } else {
            /* swap left and current */
            //printf("Swapping L and C\n");
            pqueue_swap(pq, lidx, idx, lft, cur);
            idx = lidx;
        }
    }
}

/* We assume that nodes are always incremented */
void pqueue_heapify_index(PriorityQueuePtr pq, uint16_t idx) {
    if (idx >= pq->last_index) return;
    PQNPtr ptr = (PQNPtr)pqueue_get_heap(pq, idx);
    //printf("----------------------------------------\n");
    //pqueue_print(pq);
    //printf("----------------------------------------\n");
    /* Sift or swim depending whether the node is new or old */
    if (ptr->new) pqueue_swim(pq, idx);
    else          pqueue_sift(pq, idx);
    //printf("----------------------------------------\n");
    //pqueue_print(pq);
    //printf("========================================\n");
}

void *pqueue_peak(PriorityQueuePtr pq) {
    void *head = ((void *)pqueue_get_heap(pq, 0));
    if (!((PQNPtr)head)->used) return 0;
    return (((uint32_t*)head)+1);
}

void pqueue_delete(PriorityQueuePtr pq) {
    //hashmap_cuckoo_delete(pq->hashmap);
    rte_hash_free(pq->hashmap);
    rte_free(pq);
}

void pqueue_print(PriorityQueuePtr pq) {
    int i = 1;
    while (i < pq->last_index+1) {
        int j = i - 1;
        for (; j < ((i * 2) - 1) && j < pq->last_index; ++j) {
            void *ptr = pqueue_get_heap(pq, j);
            printf("%u\t",*((uint32_t*)ptr+1));
        }
        printf("\n");
        i <<=1;
    }
}

void *pqueue_iterate(PriorityQueuePtr pq, uint16_t *pidx) {
    uint16_t idx = *pidx;
    if (idx >= pq->last_index) return 0;
    (*pidx)++;
    return ((uint32_t*)pqueue_get_heap(pq, idx))+1;
}

void pqueue_assert_correctness(PriorityQueuePtr pq) {
    void *cell = 0; uint16_t idx = 0;
    while ((cell = pqueue_iterate(pq, &idx)) != 0) {
        uint16_t ridx = pq_right(idx-1);
        uint16_t lidx = pq_left(idx-1);

        if (ridx >= pq->last_index) return;

        uint32_t *lft = pqueue_get_heap(pq, lidx);
        uint32_t *rgt = pqueue_get_heap(pq, ridx);
        uint32_t *cur = (uint32_t*)cell;

        lft++;
        rgt++;

        assert(*lft >= *cur);
        assert(*rgt >= *cur);
    }
}
