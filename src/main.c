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
#include "memory.h"
#include "module.h"
#include "net.h"
#include "pkt.h"

#include "modules/count_array.h"
#include "modules/super_spreader.h"

#define CORE_ID 7
#define COUNT_ARRAY_SIZE ((1<<22) - 1) // Size is 8 * 4 MB
#define SUPER_SPREADER_SIZE ((1<<20) - 1) // Size is 2 * 16 MB

void null_rx_ptr(PortPtr port, uint32_t queue, struct rte_mbuf** pkts, uint32_t count);
void null_ptr(PortPtr port);
void stats_ptr(PortPtr port);
void print_stats(PortPtr port);
void initialize(void);
int core_loop(void *);
int stats_loop(void *);
void cleanup(void);

static ConsolePtr g_console = 0;
ModulePtr g_ca_module = 0;
ModulePtr g_ss_module = 0;

__attribute__((optimize("unroll-loops")))
inline void 
null_rx_ptr(PortPtr __attribute__((unused)) port, 
        uint32_t  __attribute__((unused)) queue,
        struct rte_mbuf ** __restrict__ pkts,
        uint32_t count) {
    uint16_t i = 0;

    g_ca_module->execute(
           g_ca_module, port, pkts, count);
    g_ss_module->execute(
            g_ss_module, port, pkts, count);

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
    port_loop(port, null_rx_ptr, null_ptr);
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
null_ptr(PortPtr __attribute__((unused)) port) {
}

inline void 
stats_ptr(PortPtr port) {
    if (unlikely(console_refresh(g_console))) {
        print_stats(port);
    }
}

void
initialize(void) {
    g_console = console_create(1000);
    g_ca_module = (ModulePtr)count_array_init(COUNT_ARRAY_SIZE);
    g_ss_module = (ModulePtr)super_spreader_init(SUPER_SPREADER_SIZE);
}

void
cleanup(void){
    count_array_delete(g_ca_module);
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
