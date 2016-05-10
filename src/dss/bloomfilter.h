#ifndef _DSS_BLOOMFILTER_
#define _DSS_BLOOMFILTER_


struct BFProp {
    int reserved; // nbits later?
    int keylen;
};

// Implementation of a 1024 bit bloomfilter with 3 hash functions
typedef struct BFProp * BFPropPtr;

int bloomfilter_add_key(BFPropPtr bfp, void *bf, void const *key);
int bloomfilter_is_member(BFPropPtr bfp, void *bf, void const *key);
void bloomfilter_reset(BFPropPtr bfp, void *);

#endif
