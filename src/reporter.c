#include "rte_malloc.h"
#include "rte_memcpy.h"

#include "string.h"

#include "reporter.h"

ReporterPtr reporter_init( unsigned size, unsigned rowsize, unsigned socket) {
    ReporterPtr reporter = rte_zmalloc_socket(0, sizeof(struct Reporter), 64, socket);

    reporter->ptr1    = rte_zmalloc_socket(0, rowsize * size, 64, socket);
    reporter->ptr2    = rte_zmalloc_socket(0, rowsize * size, 64, socket);
    reporter->active  = reporter->ptr1;
    reporter->offline = reporter->ptr2;
    reporter->size    = size;
    reporter->socket  = socket;
    reporter->rowsize = rowsize;
    reporter->idx     = 0;
    reporter->version = 0;

    return reporter;
}

inline void
reporter_add_entry(ReporterPtr rep, void *entry) {
    rte_memcpy(rep->active + (rep->rowsize * rep->idx), entry, rep->rowsize);
    rep->idx++;
}

void reporter_swap(ReporterPtr rep) {
    rep->offline_idx = rep->idx;
    rep->idx = 0;

    if (rep->active == rep->ptr2) {
        rep->active = rep->ptr1;
        rep->offline = rep->ptr2;
    }

    if (rep->active == rep->ptr1) {
        rep->active = rep->ptr2;
        rep->offline = rep->ptr1;
    }

    rep->version++;
}

uint8_t *reporter_begin(ReporterPtr rep) {
    return rep->offline;
}

uint8_t *reporter_end(ReporterPtr rep) {
    return rep->offline + (rep->rowsize * rep->offline_idx);
}

uint8_t *reporter_next(ReporterPtr rep, void *ptr) {
    return ((uint8_t*)ptr) + (rep->rowsize);
}

unsigned reporter_version(ReporterPtr rep) { return rep->version; }

void reporter_save(ReporterPtr rep, const char *fname, 
        void (*func)(FILE *, void *, unsigned)) {
    FILE *fp = fopen(fname, "w+");
    uint8_t *ptr = reporter_begin(rep);
    uint8_t *end = reporter_end(rep);
    while (ptr < end) {
        func(fp, ptr, 0);
        ptr = reporter_next(rep, ptr);
    }
    fclose(fp);
}

void reporter_reset(ReporterPtr rep) {
    memset(rep->offline, 0, rep->rowsize * rep->offline_idx);
}

void reporter_free(ReporterPtr rep) {
    rte_free(rep->ptr1);
    rte_free(rep->ptr2);
    rte_free(rep);
}
