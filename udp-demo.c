#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "contiki.h"
#include "contiki-net.h"
#include "dev/serial-line.h"
#include "net/ipv6/uip-ds6.h"
#include "sys/etimer.h"
#include "simple-udp.h"
#include "net/ip/uip-debug.h"
#include "dev/uart0.h"

#define UDP_PORT_CENTRAL 7878
#define UDP_PORT_OUT 5555
#define CLOCK_REPORT CLOCK_SECOND*2
static struct simple_udp_connection broadcast_connection;
static uip_ipaddr_t server_addr;
static uint16_t central_addr[] = {0xaaaa, 0, 0, 0, 0, 0, 0, 0x1};

void connect_udp_server();

PROCESS(init_system_proc, "Init system process");
AUTOSTART_PROCESSES(&init_system_proc);

/**********************************************************
 * Int Handler for received packages
 **********************************************************/
void cb_receive_udp(struct simple_udp_connection *c,
                    const uip_ipaddr_t *sender_addr,
                    uint16_t sender_port,
                    const uip_ipaddr_t *receiver_addr,
                    uint16_t receiver_port,
                    const uint8_t *data,
                    uint16_t datalen) {
        printf("########## UDP #########");
        printf("\nReceived from UDP Server:%s\n",data);
}

/**********************************************************
 * Main process
 **********************************************************/
PROCESS_THREAD(init_system_proc, ev, data){
        PROCESS_BEGIN();
        static struct etimer periodic_timer;
        uint8_t buff_udp[50] = "Hello World!";						// Buffer for package which will later be sent
        uart0_init();

        // Init IPv6 network
        uip_ip6addr(&server_addr,									// Write human readable IP-Address into struct
                    central_addr[0],
                    central_addr[1],
                    central_addr[2],
                    central_addr[3],
                    central_addr[4],
                    central_addr[5],
                    central_addr[6],
                    central_addr[7]);

        // Register with Simple-UDP
        simple_udp_register(&broadcast_connection,					// Handler to identify this connection
                            UDP_PORT_OUT,							// Port for outgoing packages
                            &server_addr,							// Destination-IP-Address for outgoing packages
                            UDP_PORT_CENTRAL,						// Port for incoming packages
                            cb_receive_udp);						// Int handler for incoming packages
        etimer_set(&periodic_timer, CLOCK_REPORT);
        while (1) {
                PROCESS_YIELD();
                if (etimer_expired(&periodic_timer)) {
                        etimer_reset(&periodic_timer);
                        printf("Sending data to UDP Server at border router...");

                        // Send UDP package to server
                        simple_udp_sendto(&broadcast_connection,	// Handler to identify connection
                        		buff_udp, 							// Buffer of bytes to be sent
								strlen((const char *)buff_udp), 	// Length of buffer
								&server_addr);						// IP-Address of destination
                        uart0_writeb('H');
                }
        }
        PROCESS_END();
}
