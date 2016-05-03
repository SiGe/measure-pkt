#include "../common.h"
#include "../vendor/murmur3.h"

#include "rte_atomic.h"
#include "rte_malloc.h"
#include "rte_memcpy.h"

#include "hashmap.h"

struct HashMap {
    uint32_t count;
    uint32_t size;
    uint16_t rowsize;
    uint16_t keysize;
    uint16_t elsize;

    rte_atomic32_t stats_search;

    int table[];
};

inline void
hashmap_reset(HashMapPtr ptr) {
    ptr->count = 0;
    rte_atomic32_set(&ptr->stats_search, 0);
    memset(ptr->table, 0, ptr->size * (ptr->elsize + ptr->keysize) * sizeof(uint32_t));
}

HashMapPtr
hashmap_create(uint32_t size, uint16_t keysize,
    uint16_t elsize, int socket) {
    /* Size up so that we are a power of two ... no wasted space */
    size = rte_align32pow2(size);

    HashMapPtr ptr = rte_zmalloc_socket(0, 
        sizeof(struct HashMap) + /* DS */
        (size) * (elsize + keysize) * sizeof(uint32_t), /* Size of elements */
        64, socket);

    ptr->size = size-1;
    ptr->count = 0;
    ptr->elsize = elsize;
    ptr->keysize = keysize;
    ptr->rowsize = ptr->elsize + ptr->keysize;

    return ptr;
}

inline void *
hashmap_get_copy_key(HashMapPtr ptr, void const *key) {
    uint32_t hash = 0;
    MurmurHash3_x86_32(key, ptr->keysize * 4, 1, &hash);
    uint32_t *ret = (((uint32_t*)ptr->table) + /* Base addr */
            (hash & ptr->size) * (ptr->rowsize)); /* Index */
    rte_memcpy(ret, key, ptr->keysize*4);
    ptr->count++;
    rte_atomic32_inc(&ptr->stats_search);
    return (ret + ptr->keysize);
}

inline void *
hashmap_get_nocopy_key(HashMapPtr ptr, void const *key) {
    uint32_t hash = 0;
    MurmurHash3_x86_32(key, ptr->keysize * 4, 1, &hash);
    uint32_t *ret = (((uint32_t*)ptr->table) + /* Base addr */
            (hash & ptr->size) * (ptr->rowsize)); /* Index */
    ptr->count++;
    rte_atomic32_inc(&ptr->stats_search);
    return (ret + ptr->keysize);
}

inline void *
hashmap_get_with_hash(HashMapPtr ptr, uint32_t hash) {
    uint32_t *ret = (((uint32_t*)ptr->table) + /* Base addr */
            (hash & ptr->size) * (ptr->rowsize)); /* Index */
    ptr->count++;
    rte_atomic32_inc(&ptr->stats_search);
    return (ret + ptr->keysize);
}

inline void
hashmap_delete(HashMapPtr ptr) {
    rte_free(ptr);
}

inline void *
hashmap_begin(HashMapPtr ptr){
    return (((uint32_t*)ptr->table) + (ptr->keysize));
}

inline void *
hashmap_end(HashMapPtr ptr){
    return (((uint32_t*)ptr->table) + (ptr->rowsize * (ptr->size)) + ptr->keysize);
}

inline void *
hashmap_next(HashMapPtr ptr, void *cur) {
    return ((uint32_t*)cur) + ptr->rowsize;
}

inline uint32_t 
hashmap_size(HashMapPtr ptr) {
    return ptr->size;
}

inline uint32_t
hashmap_count(HashMapPtr ptr) {
    return ptr->count;
}

inline uint32_t
hashmap_num_searches(HashMapPtr ptr){ 
    return (uint32_t)(rte_atomic32_read(&ptr->stats_search));
}
