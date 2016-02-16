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
#define PORT_PKTMBUF_POOL_SIZE ((1 << 8) - 1)

/* Advised to have a size where ME_PORT_PKTMBUF_POOL_SIZE modulo num == 0 */
#define PORT_PKTMBUF_POOL_CACHE_SIZE ((17))  

#define MAX_NUM_SOCKETS 4   /* Total number of possible sockets in the system */
#define TX_DESC_DEFAULT 512 /* Mempool size for the TX queue */
#define RX_DESC_DEFAULT 128 /* Mempool size for the RX queue */
#define MAX_PKT_BURST   32  /* Maximum number of packets received in a burst */
#define MAX_QUEUES      4   /* Maximum number of queues */

#endif // _COMMON_H_
