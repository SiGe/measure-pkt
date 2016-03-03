#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rte_common.h"
#include "rte_cycles.h"
#include "rte_eal.h"
#include "rte_ethdev.h"
#include "rte_ip.h"
#include "rte_lcore.h"
#include "rte_malloc.h"
#include "rte_mbuf.h"

#include "common.h"
#include "console.h"
#include "hopscotch.h"
#include "memory.h"
#include "module.h"
#include "net.h"
#include "pkt.h"

#include "modules/count_array.h"
//#include "modules/super_spreader.h"

#include "vendor/murmur3.h"

#define CORE_ID 7
#define COUNT_ARRAY_SIZE ((1<<22) - 1) // Size is 32 * 4 MB
#define SUPER_SPREADER_SIZE ((1<<20) - 1) // Size is 32 * 4 MB

void rx_modules(PortPtr port, uint32_t queue, struct rte_mbuf** pkts, uint32_t count);
void null_func(PortPtr port);

void stats_ptr(PortPtr port);
void print_stats(PortPtr port);
void initialize(void);
void cleanup(void);

int  core_loop(void *);
int  stats_loop(void *);

static ConsolePtr g_console = 0;
ModulePtr g_ca_module = 0;
ModulePtr g_ss_module = 0;
HopscotchHashPtr g_table = 0;


/* Hopscotch pointer */
static void *h_entry = 0; //hopscotch_first(g_table);
static void const *h_end = 0;// hopscotch_end(g_table);


static uint32_t
count_occupied(void) {
    void *ptr = hopscotch_first(g_table);
    void const *end = hopscotch_end(g_table);
    uint32_t count = 0;

    while (ptr < end) {
        count += (hopscotch_is_set(ptr) == 1);
        ptr = hopscotch_next(g_table, ptr);
    }

    return count;
}

__attribute__((optimize("unroll-loops")))
inline void 
rx_modules(PortPtr __attribute__((unused)) port, 
        uint32_t  __attribute__((unused)) queue,
        struct rte_mbuf ** __restrict__ pkts,
        uint32_t count) {
    if (count == 0) return;


    uint16_t i = 0;
    uint8_t *pkt = 0;

    printf("Before: %u\n", count_occupied());
    for (i = 0; i < count; ++i) {
        pkt = rte_pktmbuf_mtod(pkts[i], uint8_t *);
        rte_prefetch0(pkt);
        hopscotch_add(g_table, pkt + 24 /* Get to srcip and dstip */);
    }

    //g_ca_module->execute(g_ca_module, port, h_entry, g_table);

    for (i = 0; i < HOPSCOTCH_SWEEP_SIZE; ++i) {
        hopscotch_clear(h_entry);
        h_entry = hopscotch_next(g_table, h_entry);

        if (unlikely(h_entry >= hopscotch_end(g_table))) {
            h_entry = hopscotch_first(g_table);
        }
    }

    for (i = 0; i < count; ++i) {
        rte_pktmbuf_free(pkts[i]);
    }
}

void print_stats(PortPtr port) {
    console_clear();
    port_print_stats(port);
}

int core_loop(void *ptr) {
    PortPtr port = (PortPtr)ptr;
    if (port_core_id(port) != rte_lcore_id())
        return 0;

    printf("Running function on lcore: %d\n", rte_lcore_id());
    port_loop(port, rx_modules, null_func);
    return 0;
}

int stats_loop(void *ptr) {
    PortPtr port = (PortPtr)ptr;
    printf("Running stats function on lcore: %d\n", rte_lcore_id());
    while(1) {
        stats_ptr(port);
    }
    return 0;
}

inline void 
null_func(PortPtr __attribute__((unused)) port) {
}

inline void 
stats_ptr(PortPtr port) {
    if (unlikely(console_refresh(g_console))) {
        print_stats(port);
    }
}

static inline uint32_t
murmur_hash_func(void const *ptr) {                                                                                  
    uint32_t ret = 0;
    MurmurHash3_x86_32( ptr, 2, 1, &ret);
    return ret;
}    

void
initialize(void) {
    g_table = hopscotch_create(8, 8, HASH_BUFFER_SIZE, murmur_hash_func);
    g_console = console_create(1000);
    g_ca_module = (ModulePtr)count_array_init(COUNT_ARRAY_SIZE);
    //g_ss_module = (ModulePtr)super_spreader_init(SUPER_SPREADER_SIZE);
    h_entry = hopscotch_first(g_table);
    h_end = hopscotch_end(g_table);
}

void
cleanup(void){
    count_array_delete(g_ca_module);
    free(g_table);
}

int
main(int argc, char **argv) {
    int ret = 0;
    ret = rte_eal_init(argc, argv);

    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments.");

    int nb_ports = 0;
    nb_ports = rte_eth_dev_count();

    int core_id = CORE_ID;
    int port_id = 0;

    printf("Total number of ports: %d\n", nb_ports);

    PortPtr port = port_create(port_id, core_id);
    if (!port) {
        port_delete(port);
        return EXIT_FAILURE;
    }

    initialize();
    atexit(cleanup);

    port_start(port);
    port_print_mac(port);

    rte_eal_remote_launch(core_loop, port, CORE_ID);
    rte_eal_remote_launch(stats_loop, port, 2);
    rte_eal_mp_wait_lcore();

    return 0;
}
