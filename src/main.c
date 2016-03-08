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

#define PORT_COUNT 2
#define COUNT_ARRAY_SIZE ((1<<22) - 1) // Size is 32 * 4 MB
#define SUPER_SPREADER_SIZE ((1<<20) - 1) // Size is 32 * 4 MB

void rx_modules(PortPtr, uint32_t, struct rte_mbuf**, uint32_t);
void null_ptr(PortPtr);
void stats_ptr(PortPtr *);
void print_stats(PortPtr);
void initialize(void);
int core_loop(void *);
int stats_loop(void *);
void cleanup(void);

static ConsolePtr g_console = 0;
ModulePtr g_ca_module = 0;
ModulePtr g_ss_module = 0;

PortPtr ports[PORT_COUNT] = {0};

__attribute__((optimize("unroll-loops")))
inline void 
rx_modules(PortPtr port, 
        uint32_t queue,
        struct rte_mbuf ** __restrict__ pkts,
        uint32_t count) {
    uint16_t i = 0;

    for (i = 0; i < port->nrx_modules; ++i) {
        ModulePtr *m = port->rx_modules[i];
        m->execute(m, port, pkts, count);
    }

    for (i = 0; i < count; ++i) {
        rte_pktmbuf_free(pkts[i]);
    }
}

void print_stats(PortPtr port) {
    port_print_stats(port);
}

int core_loop(void *ptr) {
    PortPtr port = (PortPtr)ptr;
    port_loop(port, rx_modules, null_ptr);
    return 0;
}

int stats_loop(void *ptr) {
    PortPtr *ports = (PortPtr*)ptr;
    printf("Running stats function on lcore: %d\n", rte_lcore_id());
    while(1) {
        stats_ptr(ports);
    }
    return 0;
}

inline void 
null_ptr(PortPtr __attribute__((unused)) port) {
}

inline void 
stats_ptr(PortPtr *ports) {
    unsigned i;
    if (unlikely(console_refresh(g_console))) {
        console_clear();
        for (i = 0; i < PORT_COUNT; ++i)
            print_stats(ports[i]);
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

void
init_modules(PortPtr port) {
    
}

int
main(int argc, char **argv) {
    int ret = 0;
    ret = rte_eal_init(argc, argv);

    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments.");

    int nb_ports = 0;
    nb_ports = rte_eth_dev_count();

    printf("Total number of ports: %d\n", nb_ports);

    struct CoreQueuePair pairs0 [] = { { .core_id = 1 }, };
    struct CoreQueuePair pairs1 [] = { { .core_id = 3 }, };

    initialize();
    atexit(cleanup);

    ports[0] = port_create(0, pairs0, sizeof(pairs0)/sizeof(struct CoreQueuePair));
    if (!ports[0]) { port_delete(ports[0]); return EXIT_FAILURE; }
    port_start(ports[0]);
    port_print_mac(ports[0]);

    ports[1] = port_create(1, pairs1, sizeof(pairs1)/sizeof(struct CoreQueuePair));
    if (!ports[1]) { port_delete(ports[1]); return EXIT_FAILURE; }
    port_start(ports[1]);
    port_print_mac(ports[1]);

    rte_eal_remote_launch(core_loop, ports[0], 1);
    rte_eal_remote_launch(core_loop, ports[1], 3);

    rte_eal_remote_launch(stats_loop, ports, 2);

    rte_eal_mp_wait_lcore();

    return 0;
}
