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

#include "modules/consumer.h"
#include "modules/count_array.h"
#include "modules/ring.h"
#include "modules/super_spreader.h"

#include "dss/hashmap.h"

#define PORT_COUNT 2

void rx_modules(PortPtr, uint32_t, struct rte_mbuf**, uint32_t);
void null_ptr(PortPtr);
void stats_ptr(PortPtr *);
void print_stats(PortPtr);
void initialize(void);
int core_loop(void *);
int cons_loop(void *);
int stats_loop(void *);
void cleanup(void);

static ConsolePtr g_console = 0;
ModulePtr g_ca_module = 0;
ModulePtr g_ss_module = 0;
HashMapPtr g_hashmap = 0;
ConsumerPtr g_consumer = 0;

PortPtr ports[PORT_COUNT] = {0};

inline void 
rx_modules(PortPtr port, 
        uint32_t queue,
        struct rte_mbuf ** __restrict__ pkts,
        uint32_t count) {
    uint16_t i = 0;
    (void)(port);
    (void)(queue);

    static int pcount = 0;
    uint64_t timer = rte_get_tsc_cycles();

    void *ptrs[MAX_PKT_BURST];

    //port_exec_rx_modules(port, queue, pkts, count);
    for (i = 0; i < count; ++i) {
        rte_prefetch0(pkts[i]);
    }

    for (i = 0; i < count; ++i) {
        uint8_t const* pkt = rte_pktmbuf_mtod(pkts[i], uint8_t const*);
        void *ptr = hashmap_get_copy_key(g_hashmap, (pkt + 26));
        //void *ptr = hashmap_get_with_hash(g_hashmap, pkts[i]->hash.rss);

        ptrs[i] = ptr;
        rte_prefetch0(ptr);
    }

   for (i = 0; i < count; ++i) { 
        void *ptr = ptrs[i];
        uint32_t *bc = (uint32_t*)(ptr); (*bc)++;
        uint64_t *time = (uint64_t*)(bc + 1);
        *time = timer;
        rte_pktmbuf_free(pkts[i]);
    }

    pcount += count;

    if (unlikely(pcount > 100000000)) {
        print_stats(port);
        exit(0);
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

int cons_loop(void *ptr) {
    ConsumerPtr cn = (ConsumerPtr)ptr;
    consumer_loop(cn);
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
            if (ports[i])
                print_stats(ports[i]);

        void *end = hashmap_end(g_hashmap);
        void *ptr = hashmap_begin(g_hashmap);

        uint32_t count = 0;
        uint32_t uniq = 0;

        for (; ptr != end; ptr = hashmap_next(g_hashmap, ptr)) {
            uint32_t val = *((uint32_t*)ptr);
            if (val != 0) uniq++;
            count += val;
        };
        printf("Wololo: %d, %d/%d\n", count, uniq, hashmap_size(g_hashmap));

        consumer_print_stats(g_consumer);
    }
}

void
initialize(void) {
    g_console = console_create(1000);
    g_ca_module = (ModulePtr)count_array_init(COUNT_ARRAY_SIZE);
    g_ss_module = (ModulePtr)super_spreader_init(SUPER_SPREADER_SIZE);

    g_hashmap = hashmap_create(COUNT_ARRAY_SIZE, 3, 3, 1);
    g_consumer = consumer_init();
}

void
cleanup(void){
    count_array_delete(g_ca_module);
}

static void
init_modules(PortPtr port) {
    static int ring_id = 0;
    ModuleRingPtr ring = ring_init(ring_id++, 256, 16, port_socket_id(port));
    port_add_rx_module(port, (ModulePtr)ring);
    consumer_add_ring(g_consumer, ring_get_ring(ring));
}

static int
run_port_at(uint32_t port_id, uint32_t core_id) {
    struct CoreQueuePair pairs0 [] = { { .core_id = 0 }, };
    pairs0[0].core_id = core_id;
    PortPtr port = ports[port_id] = port_create(port_id, pairs0,
            sizeof(pairs0)/sizeof(struct CoreQueuePair));
    if (!port) { port_delete(port); return EXIT_FAILURE; }
    port_start(port);
    port_print_mac(port);
    init_modules(port);
    rte_eal_remote_launch(core_loop, port, core_id);
    return 0;
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

    initialize();
    atexit(cleanup);

    if (run_port_at(0, 1) != 0) exit(-1);


    //rte_eal_remote_launch(cons_loop, g_consumer, 11);
    rte_eal_remote_launch(stats_loop, ports, 3);
    rte_eal_mp_wait_lcore();

    return 0;
}
