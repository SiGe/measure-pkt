#ifndef _PQUEUE_H_
#define _PQUEUE_H_

#include <inttypes.h>

typedef struct PriorityQueue* PriorityQueuePtr;

typedef int (*pqueue_comp_func)(void const*, void const *);

PriorityQueuePtr pqueue_create(unsigned size, uint16_t keysize,
        uint16_t elsize, unsigned socket, pqueue_comp_func func);
void pqueue_reset(PriorityQueuePtr);
void* pqueue_get_copy_key(PriorityQueuePtr, void const *key, uint16_t *idx);
void pqueue_heapify_index(PriorityQueuePtr, uint16_t idx);
void pqueue_swim(PriorityQueuePtr, uint16_t idx);
void pqueue_sift(PriorityQueuePtr, uint16_t idx);
void pqueue_delete(PriorityQueuePtr);
void *pqueue_peak(PriorityQueuePtr);
void pqueue_print(PriorityQueuePtr pq);
void *pqueue_iterate(PriorityQueuePtr pq, uint16_t *);
void pqueue_assert_correctness(PriorityQueuePtr pq);

#endif
