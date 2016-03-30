/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Omid Alipourfard <g@omid.io>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "rte_common.h"
#include "rte_cycles.h"
#include "rte_eal.h"
#include "rte_ethdev.h"
#include "rte_errno.h"
#include "rte_mempool.h"

#include "common.h"
#include "debug.h"
#include "memory.h"
#include "net.h"

static struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 0, /**< IP checksum offload disabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
        .mq_mode        = ETH_MQ_RX_RSS,
	},
    .rx_adv_conf = {
        .rss_conf = {
            .rss_hf = ETH_RSS_IP
        },
    },
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

struct mbuf_table {
    struct rte_mbuf     *tx_bufs[MAX_PKT_BURST];
    struct rte_mbuf     *rx_bufs[MAX_RX_BURST];
    uint32_t             len;
    uint32_t             rx_len;
};

struct Port {
    uint16_t    port_id;
    uint16_t    core_id;
    uint16_t    socket_id;

    uint16_t    nrxq; /* Num RX queues */
    uint16_t    ntxq; /* Num TX queues */

    uint16_t    rx_burst_size; /* Burst size of RX queue */
    uint16_t    tx_burst_size; /* Burst size of TX queue */

    struct rte_eth_conf *conf;  /* Ethernet config */
    struct rte_mempool  *pool;  /* DPDK pktmbuf pool */

    struct mbuf_table    rxtx_tables[MAX_QUEUES];
    struct rte_eth_stats port_stats;      /* Port statistics */
    uint64_t             port_stats_time; /* Time when the port stat snapshot was taken */
    struct ether_addr    port_mac;
};

static int
port_configure(PortPtr port) {
    RTE_LOG(INFO, APP,
        "Creating port with %d RX rings and %d TX rings\n", port->nrxq, port->ntxq);
    return rte_eth_dev_configure(port->port_id, port->nrxq,
            port->ntxq, port->conf);
}

static int
port_setup_mempool(PortPtr port) {
    char mempool_name[32] = {0};
    snprintf(mempool_name, 31, "port-%d", port->port_id);

    /* Automatically garbage collected by EAL (?) */
    port->pool = rte_pktmbuf_pool_create(
            mempool_name, PORT_PKTMBUF_POOL_SIZE,
            PORT_PKTMBUF_POOL_CACHE_SIZE, 0,
            RTE_MBUF_DEFAULT_BUF_SIZE, port->socket_id);

    if (!port->pool)
        return ENOMEM;

    return 0;
}

static int
port_setup_rx_queues(PortPtr port) {
    int ret = 0; uint32_t rxq;
    for (rxq = 0; rxq < port->nrxq; ++rxq ) {
        ret = rte_eth_rx_queue_setup(
                port->port_id, rxq, RX_DESC_DEFAULT,
                port->socket_id, NULL, port->pool);

        if (ret)
            return ret;
    }
    return 0;
}

static int
port_setup_tx_queues(PortPtr port) {
    int ret = 0; uint32_t txq;
    for (txq = 0; txq < port->ntxq; ++txq ) {
        ret = rte_eth_tx_queue_setup(
                port->port_id, txq, TX_DESC_DEFAULT,
                port->socket_id, NULL);

        if (ret)
            return ret;
    }
    return 0;
}

int
port_start(PortPtr port) {
    int ret = 0;
    ret = rte_eth_dev_start(port->port_id);
    if (ret < 0) return ret;

    rte_eth_promiscuous_enable(port->port_id);
    return 0;
}

PortPtr
port_create(uint32_t port_id, uint32_t core_id) {
    PortPtr port = NULL;

    int ret = mem_alloc(sizeof(struct Port), NULL, (void **)&port);
    if (ret) return 0;

    /* Set all the defaults to zero */
    memset(port, 0, sizeof(struct Port));

    port->port_id = port_id;
    port->socket_id = rte_eth_dev_socket_id(port_id);
    port->core_id = core_id;

    /* Warn if the core and port are not in the same NUMA domain */
    if (port->socket_id != rte_lcore_to_socket_id(core_id)) {
        RTE_LOG(WARNING, APP,
                "Port and socket not on the same core (port: %d, socket: %d)\n",
                port->port_id, port->core_id);
    }

    port->conf = &port_conf;
    port->nrxq = 1;
    port->ntxq = 1;

    port->rx_burst_size = MAX_PKT_BURST;
    port->tx_burst_size = MAX_PKT_BURST;

    ret = port_configure(port);
    if (ret < 0) return 0;

    ret = port_setup_mempool(port);
    if (ret < 0) return 0;

    ret = port_setup_rx_queues(port);
    if (ret < 0) return 0;

    ret = port_setup_tx_queues(port);
    if (ret < 0) return 0;

    /* Get the port MAC address */
    rte_eth_macaddr_get(port->port_id, &port->port_mac);

    return port;
}

int
port_delete(PortPtr __attribute__((unused)) port) {
    /* Don't really need to do anything because the mem_alloc takes care of it */
    return 0;
}

static void
port_send_burst(PortPtr port, uint16_t txq) {
    struct mbuf_table *m_table = &port->rxtx_tables[txq];
    struct rte_mbuf   **mbufs   = m_table->tx_bufs;
    int idx = 0, nb_tx = 0;
    int port_id = port->port_id;

    if (m_table->len > 0) {
        int len = m_table->len;
        while (len != 0) {
            nb_tx = rte_eth_tx_burst(port_id, txq, mbufs + idx, len);
            idx += nb_tx;
            len -= nb_tx;
        }

        m_table->len = 0;
    }
}

#define RX_HIST_SIZE 4096
#define BUCKET_SIZE  16
static uint64_t rx_time_sample = 0;
static uint64_t rx_packets = 0;
static uint64_t rx_time = 0;
static uint64_t rx_start_cycle = 0;
static uint64_t rx_end_cycle = 0;
static uint64_t rx_loop_count = 0;
static uint64_t rx_histogram[RX_HIST_SIZE] = {0};
static uint16_t rx_hist_bucket = 0;

void
port_loop(PortPtr port, LoopRxFuncPtr rx_func, LoopIdleFuncPtr idle_func) {
    uint8_t rxq = 0, nb_rx = 0, port_id = port->port_id, txq = 0;
    uint16_t burst_size = port->rx_burst_size;
    uint16_t half_burst_size = burst_size / 2;
    uint64_t rx_diff = 0;
    double   rx_per_packet = 0;

    (void)(rx_func);
    (void)(rx_time_sample);
    (void)(half_burst_size);

    while (1) {
        /* Consume Receive Queues */
        for (rxq = 0; rxq < port->nrxq; ++rxq) {
            struct mbuf_table *table = &port->rxtx_tables[rxq];

            do {
                struct rte_mbuf **pkts = table->rx_bufs + table->rx_len;
                nb_rx = rte_eth_rx_burst(port_id, rxq, pkts, burst_size);
                table->rx_len += nb_rx;

                if (unlikely(table->rx_len >= (MAX_RX_BURST)/2)) {
                    uint32_t rx_len = table->rx_len;

                    rx_start_cycle = rte_get_tsc_cycles();
                    rx_func(port, rxq, table->rx_bufs, rx_len); 
                    rx_end_cycle = rte_get_tsc_cycles();

                    table->rx_len = 0;

                    if (likely(rx_len > 0)) {
                        rx_packets += rx_len;
                        rx_diff = (rx_end_cycle - rx_start_cycle);
                        rx_time += rx_diff;
                        rx_per_packet = (double)(rx_diff)/(double)(rx_len);
                        rx_loop_count++;

                        rx_hist_bucket = (uint64_t)((rx_per_packet)/BUCKET_SIZE);
                        if (rx_hist_bucket > RX_HIST_SIZE)
                            rx_hist_bucket = RX_HIST_SIZE - 1;
                        rx_histogram[rx_hist_bucket] += rx_len;
                    }
                }
            } while (nb_rx == burst_size);
        }

        /* Drain Transmit Queues */
        for (txq = 0; txq < port->ntxq; ++txq) {
            port_send_burst(port, txq);
        }

        idle_func(port);
    }
}

int
port_send_packet(PortPtr port, uint16_t txq, struct rte_mbuf *pkt) {
    struct mbuf_table *m_table = &port->rxtx_tables[txq];
    struct rte_mbuf   **mbufs   = m_table->tx_bufs;

    int len = m_table->len;
    if (unlikely(len == MAX_PKT_BURST)) {
        // Send a burst of packets when the queue is full
        port_send_burst(port, txq);
        len = 0;
    }

    mbufs[len] = pkt;
    len++;
    m_table->len = len;
    return 0;
}

inline uint32_t
port_id(PortPtr port) { return port->port_id; };

inline uint32_t
port_core_id(PortPtr port) { return port->core_id; };

void
port_print_mac(PortPtr port) {
    struct ether_addr *mac = &port->port_mac;

    printf("Port (MAC: %d): %02X:%02X:%02X:%02X:%02X:%02X\n",
            port->port_id,
            mac->addr_bytes[0],
            mac->addr_bytes[1],
            mac->addr_bytes[2],
            mac->addr_bytes[3],
            mac->addr_bytes[4],
            mac->addr_bytes[5]);
}

static uint32_t
rx_percentile(double percentile) {
    double packet_count = (rx_packets * percentile)/ 100.0f;
    uint32_t i = 0;
    uint64_t total_packet = 0;
    for (i = 0; i < RX_HIST_SIZE; ++i) {
        total_packet += rx_histogram[i];

        if (total_packet > packet_count)
            return (i) * BUCKET_SIZE;
    }

    return RX_HIST_SIZE * BUCKET_SIZE;
}

static double
rx_average(void) {
    uint32_t i = 0;
    double total_packet = 0;
    for (i = 0; i < RX_HIST_SIZE; ++i) {
        total_packet += (rx_histogram[i] * i * BUCKET_SIZE);
    }

    return total_packet/rx_packets + BUCKET_SIZE/2;
}


void
port_print_stats(PortPtr port) {
    struct rte_eth_stats *port_stats = &port->port_stats;

    uint64_t l_time     = port->port_stats_time;
    uint64_t l_opackets = port_stats->opackets;
    uint64_t l_ipackets = port_stats->ipackets;

    rte_eth_stats_get(port->port_id, port_stats);
    port->port_stats_time = rte_get_tsc_cycles();

    uint64_t l_hertz = rte_get_tsc_hz();
    double elapsed_time_s = (double)(port->port_stats_time - l_time) / (l_hertz);
    //double elapsed_time_ms = elapsed_time_s * 1000.0f;

    double o_rate = (port_stats->opackets - l_opackets) / (elapsed_time_s);
    double i_rate = (port_stats->ipackets - l_ipackets) / (elapsed_time_s);

    //static uint64_t last_ipackets = 0;
    //static uint64_t last_imissed  = 0;

    printf(
".------------------------------------------------------------------.\n\
| Port (%1d) - MAC [%02X:%02X:%02X:%02X:%2X:%02X]                               |\n\
|------------------------------------------------------------------|\n\
| Out: %15" PRIu64 " | Rate: %10.0f | Error: %15" PRIu64 " |\n\
|------------------------------------------------------------------|\n\
| In : %15" PRIu64 " | Rate: %10.0f | Error: %15" PRIu64 " | Loss: %.2f \n\
|------------------------------------------------------------------|\n\
| Avg burst size   : %15.4f                               |\n\
|------------------------------------------------------------------|\n\
| Cycles per packet : %15.0f                              |\n\
| 99.9999 Percentile: %15.4f                              |\n\
| 99.999  Percentile: %15.4f                              |\n\
| 99.99   Percentile: %15.4f                              |\n\
| 99.9    Percentile: %15.4f                              |\n\
| 99.0    Percentile: %15.4f                              |\n\
| 95.0    Percentile: %15.4f                              |\n\
| 90.0    Percentile: %15.4f                              |\n\
| 50.0    Percentile: %15.4f                              |\n\
| Average Cycles pp.: %15.4f                              |\n\
*------------------------------------------------------------------*\n",
            port->port_id,
            port->port_mac.addr_bytes[0], port->port_mac.addr_bytes[1],
            port->port_mac.addr_bytes[2], port->port_mac.addr_bytes[3],
            port->port_mac.addr_bytes[4], port->port_mac.addr_bytes[5],
            port_stats->opackets, o_rate, port_stats->oerrors,
            port_stats->ipackets, i_rate, port_stats->imissed, (double)((port_stats->imissed ) * 100.0f)/(port_stats->imissed + port_stats->ipackets + 1),
            (double)(rx_packets)/(double)(rx_loop_count),
            (double)(rx_time)/(double)(rx_packets),
            (double)rx_percentile(99.9999),
            (double)rx_percentile(99.999),
            (double)rx_percentile(99.99),
            (double)rx_percentile(99.9),
            (double)rx_percentile(99.0),
            (double)rx_percentile(95.0),
            (double)rx_percentile(90.0),
            (double)rx_percentile(50.0),
            (double)rx_average()
            );

    //last_imissed = port_stats->imissed;
    //last_ipackets = port_stats->ipackets;

#ifdef DEBUG
    static struct rte_eth_xstats g_stats[1024];
    int n = 0, i = 0;
    n = rte_eth_xstats_get(port->port_id, g_stats, 1024);
    for (i = 0; i < n; ++i) {
        printf("%s: %" PRIu64 "\n", g_stats[i].name, g_stats[i].value);
    }
#endif
}
