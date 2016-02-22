#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rte_common.h"
#include "rte_cycles.h"
#include "rte_eal.h"
#include "rte_ethdev.h"
#include "rte_lcore.h"
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

static struct rte_mempool *tx_pkts = NULL;
static struct rte_mbuf *tx_ptr[64] = {0};
static uint8_t pkts_sent = 0;
static ConsolePtr g_console = 0;

void print_stats(PortPtr port);
void initialize(void);
int core_loop(void *);

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

inline void 
null_ptr(PortPtr port) {
    if (unlikely(console_refresh(g_console))) {
        print_stats(port);
    }

    if (likely(pkts_sent == 1))
        return;

    int i = 0;
    for (i = 0; i < 64; ++i)
        port_send_packet(port, 0, tx_ptr[i]);
    pkts_sent = 1;
}

void
initialize(void) {
    g_console = console_create(5000);
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

    tx_pkts = rte_pktmbuf_pool_create("testing", 2047, 15, 0, 2048, socket_id);
    if (!tx_pkts)
        return 0;

    struct rte_mbuf **ptr = tx_ptr;
    rte_mempool_sc_get_bulk(tx_pkts, (void **)ptr, 64);
    const char pkt[] = {
       0xc8, 0x1f, 0x66, 0xdd , 0x3e, 0xec, 0x68, 0x05, 0xca, 0x1e, 0xa5, 0xe4, 0x08, 0x00, 0x45, 0x00,
       0x00, 0x28, 0x00, 0x01 , 0x00, 0x00, 0x40, 0x06, 0x48, 0xd7, 0xa2, 0xae, 0xcc, 0x3c, 0x5e, 0x39,
       0x64, 0xd4, 0x00, 0x50 , 0x06, 0xd7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02,
       0x20, 0x00, 0x56, 0xc3 , 0x00, 0x00          
    };

    int i = 0;
    for (i = 0; i < 64; ++i) {
        memcpy((uint8_t*)(ptr[i]->buf_addr) + ptr[i]->data_off,
            pkt, sizeof(pkt));

        ptr[i]->data_len = sizeof(pkt);
        ptr[i]->pkt_len = sizeof(pkt);
    }
    

    initialize();
    port_start(port);
    port_print_mac(port);
    sleep(5);
    rte_eal_remote_launch(core_loop, port, 1);
    rte_eal_mp_wait_lcore();
    // port_loop(port, null_rx_ptr, null_ptr);

    return 0;
}
