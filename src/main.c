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
#include "reporter.h"

#include "modules/consumer.h"
#include "modules/count_array.h"
#include "modules/count_array_cuckoo.h"
#include "modules/count_array_hashmap.h"
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

ModulePtr g_ca_hm_module = 0;
ModulePtr g_ca_cc_module = 0;
ReporterPtr g_reporter = 0;
int         g_report   = 0;

HashMapPtr  g_hashmap  = 0;
ConsumerPtr g_consumer = 0;

PortPtr ports[PORT_COUNT] = {0};

inline void 
rx_modules(PortPtr port, 
        uint32_t queue,
        struct rte_mbuf ** __restrict__ pkts,
        uint32_t count) {
    (void)(port);
    (void)(queue);

    uint16_t i = 0;
    for (i = 0; i < count; ++i) {
        rte_prefetch0(pkts[i]);
    }

    port_exec_rx_modules(port, queue, pkts, count);

    for (i = 0; i < count; ++i) { 
        uint8_t const *pkt = rte_pktmbuf_mtod(pkts[i], uint8_t const*);
        pkt += 38;
        uint32_t pktid = *((uint32_t const*)(pkt));
        if (!(pktid & REPORT_THRESHOLD)) {
            g_report = 1;
        }
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

static inline void
_rsave(FILE *fp, void *data, unsigned unused) {
    (void)(unused);
    unsigned *key = (unsigned*)(data);
    fprintf(fp, "%u, %u, %u\n", *(key), *(key+1), *(key+2));
}

int stats_loop(void *ptr) {
    PortPtr *ports = (PortPtr*)ptr;
    printf("Running stats function on lcore: %d\n", rte_lcore_id());
    while(1) {
        stats_ptr(ports);

        if (g_report) {
            g_report = 0;
            uint32_t version = reporter_version(g_reporter);
            char buf[255] = {0};
            snprintf(buf, 255, "log-%d.log", version);
            reporter_swap(g_reporter);
            reporter_save(g_reporter, buf, _rsave);
            reporter_reset(g_reporter);
        }
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

        printf("Num heavy-hitters: %u\n", g_reporter->offline_idx);
        consumer_print_stats(g_consumer);
    }
}

void
initialize(void) {
    g_console = console_create(1000);
    g_reporter= reporter_init(1024, 4, 1);

    g_ca_module = (ModulePtr)count_array_init(COUNT_ARRAY_SIZE);
    g_ss_module = (ModulePtr)super_spreader_init(SUPER_SPREADER_SIZE);

    g_ca_hm_module = (ModulePtr)count_array_hashmap_init(COUNT_ARRAY_SIZE, 3, 3, 1, g_reporter);
    g_ca_cc_module = (ModulePtr)count_array_cuckoo_init(COUNT_ARRAY_SIZE, 3, 3, 1, g_reporter);
    g_consumer = consumer_init();
}

void
cleanup(void){
    count_array_delete(g_ca_module);
    count_array_hashmap_delete(g_ca_hm_module);
    count_array_cuckoo_delete(g_ca_cc_module);
    reporter_free(g_reporter);
}

static void
init_modules(PortPtr port) {
    port_add_rx_module(port, (ModulePtr)g_ca_hm_module);
    //port_add_rx_module(port, (ModulePtr)g_ca_cc_module);

    //static int ring_id = 0;
    //ModuleRingPtr ring = ring_init(ring_id++, 256, 16, port_socket_id(port));
    //port_add_rx_module(port, (ModulePtr)ring);
    //consumer_add_ring(g_consumer, ring_get_ring(ring));
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
