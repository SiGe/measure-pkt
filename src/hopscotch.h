#ifndef _HOPSCOTCH_H_
#define _HOPSCOTCH_H_

typedef struct HopscotchHash * HopscotchHashPtr;
typedef uint32_t (*HopscotchHashFunc)(void const *);

/* Creates a new hash table */
HopscotchHashPtr hopscotch_create(uint16_t, uint16_t, uint32_t, HopscotchHashFunc);

/* Adds an entry to the hash table */
int hopscotch_add(HopscotchHashPtr, void const *);

/* Returns the next element of the table */
void *hopscotch_next(HopscotchHashPtr, void const *);

/* Returns the first entry of the table */
void *hopscotch_first(HopscotchHashPtr);

/* Reset the whole data-structure */
void  hopscotch_reset(HopscotchHashPtr);

/* Returns the end of the hash table */
void const *hopscotch_end(HopscotchHashPtr);

/* Count the number of entries associated with a cell in the table */
uint16_t hopscotch_count(void const *);

/* Mark the entry as used */
void hopscotch_set(void *);

/* Reset the entry as used */
void hopscotch_clear(void *);

/* Returns true if the entry is in use */
uint8_t hopscotch_is_set(void const *);

/* Reset an entry in the Hashtable */
void hopscotch_clear(void *);

#endif // 
