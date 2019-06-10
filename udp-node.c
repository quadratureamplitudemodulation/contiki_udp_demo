#include "contiki.h"
#include "contiki-net.h"
#include "servreg-hack.h"
#include "simple-udp.h"
#include "net/ip/uip-debug.h"
#include "dev/button-sensor.h"
#include "net/rpl/rpl.h"
#include "uart-module.h"
#include "init_process.h"

/* Normally the software runs to be used on a CC1310. However if it's supposed to be debugged, on of the following
 * defines can be used.
 */

//#define DEBUG_CC1310	// CC1310 will not send UDP packets, but UART messages
#define DEBUG_Z1	// Node will not react to UART messages, but send "Hello World" via UDP when Button is pressed
#ifdef DEBUG_Z1
#include "dev/leds.h"
#endif
#define UDP_PORT_CENTRAL 1234
#define UDP_PORT_OUT 1234

PROCESS(init_system_proc, "Init system process");
AUTOSTART_PROCESSES(&init_system_proc);

/**********************************************************
 * Int Handler for received UDP packages
 **********************************************************/
void cb_receive_udp(struct simple_udp_connection *c,
                    const uip_ipaddr_t *sender_addr,
                    uint16_t sender_port,
                    const uip_ipaddr_t *receiver_addr,
                    uint16_t receiver_port,
                    const uint8_t *data,
                    uint16_t datalen) {
#ifdef DEBUG_Z1
	leds_toggle(LEDS_ALL);
	printf("Received data");
#elif DEBUG_CC1310
	printf("Received data");
 }
#endif
}
/**********************************************************
 * Set global IPv6 Address
 **********************************************************/
static uip_ipaddr_t *
set_global_address(void)
{
  static uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  return &ipaddr;
}
/**********************************************************
 * Main process
 **********************************************************/
PROCESS_THREAD(init_system_proc, ev, data){
        PROCESS_BEGIN();

        /* Handler for Simple-UDP Connection */
        static struct simple_udp_connection udp_connection;

        /* IP Address of device */
		static uip_ipaddr_t *ip_addr;

		ip_addr = set_global_address();

		uart_init();
		process_start(&uart_int_handler, NULL);

        /* IP Address of destination */
        uip_ipaddr_t *ip_dest_p;

        rpl_set_mode(RPL_MODE_LEAF);

        SENSORS_ACTIVATE(button_sensor);

		servreg_hack_init();

        simple_udp_register(&udp_connection,						// Handler to identify this connection
                            UDP_PORT_OUT,							// Port for outgoing packages
                            NULL,									// Destination-IP-Address for outgoing packages
                            UDP_PORT_CENTRAL,						// Port for incoming packages
                            cb_receive_udp);						// Int handler for incoming packages
#ifdef DEBUG_Z1
        printf("Initialized\n");
#elif DEBUG_CC1310
        printf("Initialized\n");
#endif

        while (1) {
#ifdef DEBUG_Z1
        	PROCESS_YIELD();
        	if(ev==CUSTOMER_EVENT_SEND_TO_ID){
        		udp_packet *packet;
        		packet = (udp_packet *)data;
			printf("Trying to reach ID: %i\n", (*packet).dest_id);
    			ip_dest_p = servreg_hack_lookup(packet->dest_id);				// Get receiver IP via Servreg-Hack
    			if(ip_dest_p==NULL)
    				printf("Server not found \n");
    			else
    				printf("Server address ");
    				uip_debug_ipaddr_print(ip_dest_p);
    				printf("\n");
    				printf("Data: %s\n", packet->data);
    				simple_udp_sendto(&udp_connection,					// Handler to identify connection
    								packet->data, 						// Buffer of bytes to be sent
    								strlen((const char *)packet->data), // Length of buffer
    								ip_dest_p);							// Destination IP-Address*/
        	}
        	else if(ev==CUSTOMER_EVENT_REGISTER_ID){
        		printf("Register service with id %i\n", *(servreg_hack_id_t *)data);
        		servreg_hack_register(*(servreg_hack_id_t *)data, ip_addr);
        	}
#else
        	PROCESS_YIELD_UNTIL(ev==PROCESS_EVENT_POLL);

#endif

#ifdef DEBUG_CC1310
			printf(input_buffer);
#else

#endif

        }
        PROCESS_END();
}
