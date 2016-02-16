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
#include "rte_eal.h"
#include "rte_ethdev.h"
#include "rte_errno.h"
#include "rte_mempool.h"

#include "common.h"
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
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

struct PortStats {
    // ...
}; 

struct Port {
    int    port_id;
    int    socket_id;

    int    nrxq; /* Num RX queues */
    int    ntxq; /* Num TX queues */

    struct rte_mempool  *pool;  /* DPDK pktmbuf pool */
    struct PortStats    *stats; /* Port statistics */
    struct rte_eth_conf *conf;  /* Ethernet config */
};

static int
port_configure(PortPtr port) {
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
    int ret = 0, rxq;
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
    int ret = 0, txq;
    for (txq = 0; txq < port->ntxq; ++txq ) {
        ret = rte_eth_tx_queue_setup(
                port->port_id, txq, RX_DESC_DEFAULT,
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
port_create(int port_num) {
    PortPtr port = NULL;

    int ret = mem_alloc(sizeof(struct Port), NULL, (void **)&port);
    if (ret) return 0;

    port->port_id = port_num;
    port->socket_id = rte_eth_dev_socket_id(port_num);
    port->conf = &port_conf;
    port->nrxq = 1;
    port->ntxq = 1;

    ret = port_configure(port);
    if (ret < 0) return 0;

    ret = port_setup_mempool(port);
    if (ret < 0) return 0;

    ret = port_setup_rx_queues(port);
    if (ret < 0) return 0;

    ret = port_setup_tx_queues(port);
    if (ret < 0) return 0;

    return port;
}

int
port_delete(PortPtr __attribute__((unused)) port) {
    /* Don't really need to do anything because the mem_alloc takes care of it */
    return 0;
}
