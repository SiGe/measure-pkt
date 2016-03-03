#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hopscotch.h"

#define KEY_SIZE 12

#define likely(x)   __builtin_expect((x),1)
#define unlikely(x)   __builtin_expect((x),0)

__attribute__((packed))
struct HopscotchEntry {
    union {
        uint32_t opt_;
        __attribute__((packed))
        struct {
            uint8_t occupied;
            uint16_t count;
        } opt;
    };
    uint8_t val[];
};

__attribute__((packed))
struct HopscotchHash {
    uint16_t H;
    uint16_t size;
    uint32_t count;
    void const *end;
    HopscotchHashFunc hash;
    struct HopscotchEntry table[];
};

HopscotchHashPtr
hopscotch_create(uint16_t H, uint16_t el_size, uint32_t count,
        HopscotchHashFunc func) {
    uint32_t size = sizeof(struct HopscotchHash) +
        (el_size + sizeof(struct HopscotchEntry)) * (count+1);
    HopscotchHashPtr ret = (HopscotchHashPtr)malloc(size);

    ret->H = H;
    ret->size = el_size;
    ret->count = count;
    ret->hash = func;
    ret->end = size + ((char const *)ret);

    hopscotch_reset(ret);

    return ret;
}

static inline void *
hash_to_key(HopscotchHashPtr table, uint32_t hash) {
    return ((char*)table->table) + (table->count & hash) * (table->size + sizeof(struct HopscotchEntry));
}

static inline struct HopscotchEntry *
hash_next(HopscotchHashPtr table, struct HopscotchEntry *ptr) {
    return (struct HopscotchEntry *)(
            ((char*)ptr) + (table->size) + sizeof(struct HopscotchEntry));
}

/*
static uint32_t
count_occupied(HopscotchHashPtr table) {
    void *ptr = hopscotch_first(table);
    void const *end = hopscotch_end(table);
    uint32_t count = 0;

    while (ptr < end) {
        count += (hopscotch_is_set(ptr) == 1);
        ptr = hopscotch_next(table, ptr);
    }

    return count;
}*/

inline int
hopscotch_add(HopscotchHashPtr table, void const * val) {
    struct HopscotchEntry *entry = 
        (struct HopscotchEntry*)hash_to_key(table, table->hash(val));
    uint32_t const *ptr1 = (uint32_t const *)val;
    //printf("Count: %d\n", count_occupied(table));

    while (1) {
        uint32_t *ptr2 = (uint32_t *)(entry->val);
        if (!entry->opt.occupied) {
            ptr2[0] = ptr1[0];
            ptr2[1] = ptr1[1];
            entry->opt.occupied = 1;
            entry->opt.count = 1;
            return 0;
        }

        if (ptr1[0] == ptr2[0] &&
            ptr1[1] == ptr2[1]) {
            entry->opt.count++;
            return 0;
        }

        entry = hash_next(table, entry);
        if (unlikely((void *)entry >= table->end)) {
            entry = (table->table);
        }
    }
}

inline void *
hopscotch_next(HopscotchHashPtr table, void const *last) {
    return ((char*)(uintptr_t)last) + table->size + sizeof(struct HopscotchEntry);
}

inline void *
hopscotch_first(HopscotchHashPtr table) {
    return (void *)((struct HopscotchEntry *)table->table)->val;
}

inline void const*
hopscotch_end(HopscotchHashPtr table) {
    return table->end;
}

inline void
hopscotch_reset(HopscotchHashPtr table) {
    uint32_t size = (table->size + sizeof(struct HopscotchEntry)) * (table->count+1);
    memset(table->table, 0, size);
}

inline uint16_t
hopscotch_count(void const *ptr) {
    return ((struct HopscotchEntry const*)(((char const*)ptr) - sizeof(struct HopscotchEntry)))->opt.count;
}

inline uint8_t
hopscotch_is_set(void const *ptr) {
    return ((struct HopscotchEntry const*)(((char const*)ptr) - sizeof(struct HopscotchEntry)))->opt.occupied;
}

inline void
hopscotch_set(void *ptr) {
    ((struct HopscotchEntry *)(((char *)ptr) - sizeof(struct HopscotchEntry)))->opt.occupied = 1;
}

inline void
hopscotch_clear(void *ptr) {
    ((struct HopscotchEntry *)(((char *)ptr) - sizeof(struct HopscotchEntry)))->opt.occupied = 0;
}
