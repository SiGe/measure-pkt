#include <assert.h>

#include "../common.h"
#include "../vendor/murmur3.h"

#include "rte_malloc.h"
#include "rte_memcpy.h"
#include "rte_memory.h"

#include "hashmap_cuckoo.h"

#define HASHMAP_CUCKOO_MAX_TRIES 32

struct HashMapCuckoo {
    uint32_t count;
    uint32_t size;
    uint16_t rowsize;
    uint16_t keysize;
    uint16_t elsize;

    uint8_t *table;

    uint8_t *tblpri;
    uint8_t *tblsec;

    uint8_t *scratch_1;
    uint8_t *scratch_2;

    uint8_t end[];
};

/* Algorithm and ideas mostly taken from:
 *  1) http://cs.stanford.edu/~rishig/courses/ref/l13a.pdf
 *  2) librte_hash
 */

inline static uint32_t
hashmap_cuckoo_total_size(HashMapCuckooPtr ptr) {
    return ((ptr->size+1) * 2 * (ptr->rowsize)) * sizeof(uint32_t);
}

inline void
hashmap_cuckoo_reset(HashMapCuckooPtr ptr) {
    ptr->count = 0;

    memset(ptr->table, 0, hashmap_cuckoo_total_size(ptr));
    memset(ptr->scratch_1, 0, ptr->rowsize);
    memset(ptr->scratch_2, 0, ptr->rowsize);
}

HashMapCuckooPtr
hashmap_cuckoo_create(uint32_t size, uint16_t keysize,
    uint16_t elsize, int socket) {

    /* Size up so that we are a power of two ... no wasted space */
    size = rte_align32pow2(size);

    HashMapCuckooPtr ptr = rte_zmalloc_socket(0, 
        sizeof(struct HashMapCuckoo) + 4*(elsize + keysize + 1)*2,
        64, socket);

    ptr->size    = size-1;
    ptr->count   = 0;
    ptr->elsize  = elsize;
    ptr->keysize = keysize;
    ptr->rowsize = ptr->elsize + ptr->keysize + 1;

    ptr->table = rte_zmalloc_socket(0,
            hashmap_cuckoo_total_size(ptr), 64, socket);

    ptr->tblpri = ptr->table;
    ptr->tblsec = ptr->table + hashmap_cuckoo_total_size(ptr) / 2;

    ptr->scratch_1 = ptr->end;
    ptr->scratch_2 = ptr->end + ptr->rowsize * 4;

    return ptr;
}

inline static uint32_t hash_key(
        HashMapCuckooPtr ptr, void const *key)  {
    uint32_t hash;
    MurmurHash3_x86_32(key, ptr->keysize * 4, 1, &hash);
    return hash;
}

/* Taken from rte_hash library */
inline static uint32_t
hash_sec(uint32_t primary_hash) {
    static const unsigned all_bits_shift = 12;
    static const unsigned alt_bits_xor = 0x5bd1e995;

    uint32_t tag = primary_hash >> all_bits_shift;

    return (primary_hash ^ ((tag + 1) * alt_bits_xor));
}

inline static uint32_t hash_offset(
        HashMapCuckooPtr ptr, uint32_t idx) {
    return ((idx & ptr->size) * (ptr->rowsize) * sizeof(uint32_t));
}

inline static void *find_key(
        HashMapCuckooPtr ptr, void const *key) {
    /* Check primary and secondary table */
    uint32_t hash = hash_key(ptr, key);
    uint8_t *keypos = &ptr->tblpri[hash_offset(ptr, hash)];
    rte_prefetch0(keypos);

    uint32_t hash_2 = hash_sec(hash);
    uint8_t *keypos_2 = &ptr->tblsec[hash_offset(ptr, hash_2)];
    rte_prefetch0(keypos_2);

    if (memcmp(keypos, key, ptr->keysize * 4) == 0)
        return keypos;

    if (memcmp(keypos_2, key, ptr->keysize * 4) == 0)
        return keypos_2;

    return 0;
}

inline static void print_line(void const *key) {
    uint32_t const *ptr = (uint32_t const*)key;
    printf("%p, %"PRIu64" -> %"PRIu64" (hash: %u)\n",
            ptr, *(uint64_t const*)ptr, *(uint64_t const*)(ptr+3), *(ptr+2));

}

inline static void *find_or_insert_key(
        HashMapCuckooPtr ptr, void const *key) {
    //printf("\n\n");
    //printf("Inserting: %"PRIu64"\n", *(uint64_t const*)key);
    /* Check primary and secondary table */
    uint32_t hash = hash_key(ptr, key);
    uint8_t *keypos = &ptr->tblpri[hash_offset(ptr, hash)];
    rte_prefetch0(keypos);
    //printf("Trying (Pri): "); print_line(keypos);

    uint32_t hash_2 = hash_sec(hash);
    uint8_t *keypos_2 = &ptr->tblsec[hash_offset(ptr, hash_2)];
    rte_prefetch0(keypos_2);
    //printf("Trying (Sec): "); print_line(keypos_2);

    if (memcmp(keypos, key, ptr->keysize * 4) == 0)
        return keypos;

    if (memcmp(keypos_2, key, ptr->keysize * 4) == 0)
        return keypos_2;

    uint8_t i = 0;
    uint8_t *ret = keypos;

    memset(ptr->scratch_1, 0, ptr->rowsize*4);
    rte_memcpy(ptr->scratch_1, key, ptr->keysize*4);
    rte_memcpy(ptr->scratch_1 + ptr->keysize*4, &hash, sizeof(uint32_t));

    do {
        rte_memcpy(ptr->scratch_2, keypos, ptr->rowsize*4);
        //printf("Copying: "); print_line(ptr->scratch_1);
        //printf("     To: "); print_line(keypos);
        rte_memcpy(keypos, ptr->scratch_1, ptr->rowsize*4);
        if (memcmp(keypos, key, ptr->keysize) == 0) ret = keypos;
        //printf("    End: "); print_line(keypos);
        if (*((uint32_t*)(ptr->scratch_2)) == 0) {
        //    printf("Returning: "); print_line(ret);
            return ret;
        }

        /* Update keypos */
        hash = (*((uint32_t*)(ptr->scratch_2 + 4* ptr->keysize)));
        hash = hash_sec(hash);
        keypos = &ptr->tblsec[hash_offset(ptr, hash)];
        //printf("Table second position: %p, %u, %u\n", keypos, hash_offset(ptr, hash), hash);

        rte_memcpy(ptr->scratch_1, keypos, ptr->rowsize*4);
        //printf("Copying: "); print_line(ptr->scratch_2);
        //printf("     To: "); print_line(keypos);
        rte_memcpy(keypos, ptr->scratch_2, ptr->rowsize*4);
        if (memcmp(keypos, key, ptr->keysize) == 0) ret = keypos;
        //printf("    End: "); print_line(keypos);
        if (*((uint32_t*)(ptr->scratch_1)) == 0) {
        //  printf("Returning: "); print_line(ret);
            return ret;
        }

        hash = (*((uint32_t*)(ptr->scratch_1 + 4* ptr->keysize)));
        keypos = &ptr->tblpri[hash_offset(ptr, hash)];
        //printf("Table first position: %p, %u, %u\n", keypos, hash_offset(ptr, hash), hash);
        //printf("\n");
        i++;
    }while (i < HASHMAP_CUCKOO_MAX_TRIES);

    assert(0);

    return 0;
}

inline void *
hashmap_cuckoo_get_copy_key(HashMapCuckooPtr ptr, void const *key) {
    void *ret = find_or_insert_key(ptr, key);
    return (((uint32_t*)ret) + ptr->keysize + 1);
}

inline void *
hashmap_cuckoo_get_nocopy_key(HashMapCuckooPtr ptr, void const *key) {
    void *ret = find_or_insert_key(ptr, key);
    return (((uint32_t*)ret) + ptr->keysize + 1);
}

inline void *
hashmap_cuckoo_get_with_hash(HashMapCuckooPtr ptr, uint32_t hash) {
    (void)(ptr);
    (void)(hash);
    assert(0);
    return 0;
}

inline void
hashmap_cuckoo_delete(HashMapCuckooPtr ptr) {
    rte_free(ptr->table);
    rte_free(ptr);
}

inline void *
hashmap_cuckoo_begin(HashMapCuckooPtr ptr){
    assert(0);
    return (((uint32_t*)ptr->table) + (ptr->keysize));
}

inline void *
hashmap_cuckoo_end(HashMapCuckooPtr ptr){
    assert(0);
    return (((uint32_t*)ptr->table) + (ptr->rowsize * (ptr->size)) + ptr->keysize);
}

inline void *
hashmap_cuckoo_next(HashMapCuckooPtr ptr, void *cur) {
    assert(0);
    return ((uint32_t*)cur) + ptr->rowsize;
}

inline uint32_t 
hashmap_cuckoo_size(HashMapCuckooPtr ptr) {
    return ptr->size;
}

inline uint32_t
hashmap_cuckoo_count(HashMapCuckooPtr ptr) {
    return ptr->count;
}