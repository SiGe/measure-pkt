#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rte_common.h"
#include "rte_eal.h"
#include "rte_ethdev.h"

#include "common.h"
#include "memory.h"
#include "net.h"

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

struct Size48 {
    char bytes[48];
};

int
main(int argc, char **argv) {
    int ret = 0;
    ret = rte_eal_init(argc, argv);

    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments.");

    int nb_ports = 0;
    nb_ports = rte_eth_dev_count();
    printf("Total number of ports: %d\n", nb_ports);

    PortPtr port = port_create(0);
    if (!port) {
        port_delete(port);
        return EXIT_FAILURE;
    }

    port_start(port);

    printf("Sleeping for 5 seconds ...\n");
    sleep(5);
    printf("Testing is good ...\n");
    return 0;
}
