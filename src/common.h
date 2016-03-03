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

#ifndef _COMMON_H_
#define _COMMON_H_

#define HUGE_PAGE_SIZE ((2<<20) - 1)
#define PORT_PKTMBUF_POOL_SIZE ((2<<13)-1)

/* Advised to have a size where PORT_PKTMBUF_POOL_SIZE modulo num == 0 */
#define PORT_PKTMBUF_POOL_CACHE_SIZE ((250))  

#define MAX_NUM_SOCKETS 4    /* Total number of possible sockets in the system */
#define RX_DESC_DEFAULT 2048 /* Mempool size for the RX queue */
#define TX_DESC_DEFAULT 512  /* Mempool size for the TX queue */
#define MAX_PKT_BURST   32   /* Maximum number of packets received in a burst */
#define MAX_RX_BURST    64   /* The amount of packets to process at one time */
#define MAX_RX_WAIT     4    /* Number of RX wait cycles */
#define MAX_QUEUES      4    /* Maximum number of queues */

#include "rte_mbuf.h"
#define MBUF_SIZE (1600 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

#include "rte_log.h"
#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1

#define MAX_MODULE_NAME 32

#define HOPSCOTCH_SWEEP_SIZE 32
#define HASH_BUFFER_SIZE 32767

#endif // _COMMON_H_
