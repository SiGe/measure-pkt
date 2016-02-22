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

#ifndef _NET_H_
#define _NET_H_

typedef struct Port* PortPtr;
typedef struct PortStats* PortStatsPtr;
typedef void (*LoopRxFuncPtr)(PortPtr, uint32_t, struct rte_mbuf**, uint32_t);
typedef void (*LoopIdleFuncPtr)(PortPtr);

/* Create a port object for port_id on core_id */
PortPtr     port_create(uint32_t, uint32_t);

/* Configure and start the port in DPDK mode */
int         port_start(PortPtr);

/* Delete the port */
int         port_delete(PortPtr);

/* Send a single packet on port/queue_id */
int         port_send_packet(PortPtr, uint16_t, struct rte_mbuf *);

/* Run the loop function of the port:
 * Looping involves three steps:
 *      Receiving packets: LoopRxFuncPtr
 *      Transmiting pending packets
 *      Run the idle function of the user: LoopIdleFuncPtr
 */
void        port_loop(PortPtr, LoopRxFuncPtr, LoopIdleFuncPtr);

/* Returns the port id */
uint32_t    port_id(PortPtr);

/* Returns the core id that the port is running on  */
uint32_t    port_core_id(PortPtr);

/* Prints out the port MAC address */
void        port_print_mac(PortPtr);

/* Print port stats */
void        port_print_stats(PortPtr);

#endif // _NET_H_
