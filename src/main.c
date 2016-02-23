#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pcap/pcap.h>

#include "rte_common.h"
#include "rte_cycles.h"
#include "rte_eal.h"
#include "rte_ethdev.h"
#include "rte_lcore.h"
#include "rte_malloc.h"
#include "rte_mbuf.h"

#include "common.h"
#include "console.h"
#include "memory.h"
#include "net.h"
#include "pkt.h"

/*
    How to configure the ports:
     1) Find where each port is, i.e., which socket? rte_eth_dev_socket_id
     2) Configure the port with rte_eth_dev_configure
     3) Configure the receive ports with rte_eth_rx_queue_setup (pass mbuf_pool)
     4) Configure the send ports with rte_eth_tx_queue_setup    (pass
     5) Start the device with rte_eth_dev_start
     6) Set the promisciouos mode
     7) Start a function on every core, rte_eal_mp_remote_launch:
         7.1) receive bursts with: rte_eth_rx_burst
         7.2) send packets with: rte_eth_tx_burst
         7.n) rte_eal_wait_lcore

*/

static struct rte_mempool *g_tx_pkts = NULL;
static struct rte_mbuf *g_tx_ptr[64] = {0};

static ConsolePtr g_console = 0;

char *g_pkts = 0;
uint32_t g_count = 0;
char *g_pkt_ptr = 0;
char *g_pkts_end = 0;

void print_stats(PortPtr port);
void initialize(void);
int core_loop(void *);
void pcap_load_file(const char *file);
void pcap_delete(void);
void null_rx_ptr(PortPtr port, uint32_t queue, struct rte_mbuf** pkts, uint32_t count);
void null_ptr(PortPtr port);

inline void 
null_rx_ptr(PortPtr __attribute__((unused)) port, 
        uint32_t  __attribute__((unused)) queue,
        struct rte_mbuf **pkts,
        uint32_t count) {
    uint32_t i = 0;
    for (i = 0; i <count; ++i) {
        //pkt_print(pkts[i]);
        rte_pktmbuf_free(pkts[i]);
    }
}


void print_stats(PortPtr port) {
    console_clear();
    port_print_stats(port);
}

void pcap_load_file(const char *file) {
    unsigned char const *packet = 0;
    struct pcap_pkthdr header;
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    g_count = 0;

    if (g_pkts) {
        pcap_delete();
    }

    handle = pcap_open_offline(file, errbuf);
    while ((packet = pcap_next(handle, &header))) { g_count++; }
    pcap_close(handle);

    g_pkts = rte_malloc(0, 64 * g_count, 64);
    handle = pcap_open_offline(file, errbuf);
    char *pkts = g_pkts;
    while ((packet = pcap_next(handle, &header))) { 
        if ((g_count % 1000000) == 0) printf("Loaded %d packets.\n", g_count);
        rte_memcpy(pkts, packet, header.caplen);
        pkts += 64;
    }
    g_pkts_end = pkts;
    g_pkt_ptr = g_pkts;
    pcap_close(handle);
}

void pcap_delete(void) {
    rte_free(g_pkts);
    g_pkts = 0;
}

int core_loop(void *ptr) {
    PortPtr port = (PortPtr)ptr;
    if (port_core_id(port) != rte_lcore_id())
        return 0;

    printf("Running function on lcore: %d\n", rte_lcore_id());
    port_loop(port, null_rx_ptr, null_ptr);
    return 0;
}

#define NUM_PKTS_TO_SEND 64

inline void 
null_ptr(PortPtr port) {
    if (unlikely(console_refresh(g_console))) {
        print_stats(port);
    }

    if (unlikely(g_pkts_end < g_pkt_ptr)) g_pkt_ptr = g_pkts;

    struct rte_mbuf **ptr = g_tx_ptr;
    rte_mempool_sc_get_bulk(g_tx_pkts, (void **)ptr, NUM_PKTS_TO_SEND);

    int i = 0;
    for (i = 0; i < NUM_PKTS_TO_SEND; ++i) {
        rte_memcpy((uint8_t*)(ptr[i]->buf_addr) + ptr[i]->data_off, g_pkt_ptr, 64);

        ptr[i]->data_len = 64;
        ptr[i]->pkt_len  = 64;

        port_send_packet(port, 0, ptr[i]);

        g_pkt_ptr += 64;
        if (unlikely(g_pkts_end < g_pkt_ptr)) g_pkt_ptr = g_pkts;
    }
}

void
initialize(void) {
    g_console = console_create(1000);
}

int
main(int argc, char **argv) {
    int ret = 0;
    ret = rte_eal_init(argc, argv);

    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments.");

    int nb_ports = 0;
    nb_ports = rte_eth_dev_count();

    int core_id = 1;
    int port_id = 0;
    int socket_id = rte_lcore_to_socket_id(core_id);

    printf("Total number of ports: %d\n", nb_ports);

    PortPtr port = port_create(port_id, core_id);
    if (!port) {
        port_delete(port);
        return EXIT_FAILURE;
    }

    g_tx_pkts = rte_pktmbuf_pool_create("testing", 2047, 15, 0, 2048, socket_id);
    if (!g_tx_pkts)
        return 0;

    pcap_load_file(argv[1]);

    initialize();
    port_start(port);
    port_print_mac(port);
    sleep(5);
    rte_eal_remote_launch(core_loop, port, 1);
    rte_eal_mp_wait_lcore();

    pcap_delete();
    // port_loop(port, null_rx_ptr, null_ptr);

    return 0;
}
