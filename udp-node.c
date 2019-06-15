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

#define DEBUG 1
#define RELEASE 0
#define NODE 1
#define ROOT 0

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
	if(datalen == 2){
		if(data[0]==PING){
			printf("Edge router anounced his Service ID as %u\n", data[1]);
			change_root_id(data[1]);
			return;
		}
	}

	printf("Received data '%s' from IP ", data);
	uip_debug_ipaddr_print(sender_addr);
	printf(" on port %i\n", receiver_port);
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

		/* ID for servreg-hack */
		static servreg_hack_id_t serviceID = 0;

		ip_addr = set_global_address();

		uart_init();
		process_start(&uart_int_handler, NULL);

        /* IP Address of destination */
        uip_ipaddr_t *ip_dest_p;

        rpl_set_mode(RPL_MODE_LEAF);

		servreg_hack_init();

        simple_udp_register(&udp_connection,						// Handler to identify this connection
                            UDP_PORT_OUT,							// Port for outgoing packages
                            NULL,									// Destination-IP-Address for outgoing packages
                            UDP_PORT_CENTRAL,						// Port for incoming packages
                            cb_receive_udp);						// Int handler for incoming packages

        while (1) {
        	PROCESS_YIELD();
        	if(ev==CUSTOMER_EVENT_SEND_TO_ID || ev==CUSTOMER_EVENT_SEND_TO_IP){
        		udp_packet packet;
        		packet = *(udp_packet *)data;
        		if(ev==CUSTOMER_EVENT_SEND_TO_ID){
        			printf("Trying to reach ID: %i\n", packet.dest_id);
        			ip_dest_p = servreg_hack_lookup(packet.dest_id);				// Get receiver IP via Servreg-Hack
        		}
        		else
        			ip_dest_p = &packet.dest_addr;
    			if(ip_dest_p==NULL)
    				printf("Server not found \n");
    			else
    				printf("Server address ");
    				uip_debug_ipaddr_print(ip_dest_p);
    				printf("\n");
    				printf("Data: %s\n", packet.data);
    				simple_udp_sendto(&udp_connection,					// Handler to identify connection
    								packet.data, 						// Data to be sent
    								strlen(packet.data), 				// Length of data
    								ip_dest_p);							// Destination IP-Address*/
        	}
        	else if(ev==CUSTOMER_EVENT_REGISTER_ID){
        		if(servreg_hack_lookup(*(servreg_hack_id_t *)data)==NULL){
					printf("Register service with id %i\n", *(uint *)data);
					servreg_hack_register(*(servreg_hack_id_t *)data, ip_addr);
					serviceID = *(servreg_hack_id_t *)data;
        		}
        		else
        			printf("ID %i already in use. Try another one.\n",*(uint *)data);
        	}
        	else if(ev==CUSTOMER_EVENT_GET_IP_FROM_ID){
        		uip_ipaddr_t *ip = servreg_hack_lookup(*(servreg_hack_id_t *)data);
        		if(ip == NULL)
        			printf("Service with ID %i is not provided in the network\n", *(uint *)data);
        		else{
					printf("Service with ID %i is provided by IP ", *(uint *)data);
					uip_debug_ipaddr_print(ip);
					printf("\n");
        		}
        	}
        	else if(ev==CUSTOMER_EVENT_GET_LOCAL_ID){
        		if(serviceID == 0)
        			printf("No service registered yet.\n");
        		else
        			printf("Service registered with ID %i\n", serviceID);
        	}
        	else if(ev==CUSTOMER_EVENT_GET_LOCAL_IP){
        		printf("The IPv6 address of this device is ");
        		uip_debug_ipaddr_print(ip_addr);
				printf("\n");
        	}
        	else if(ev==CUSTOMER_EVENT_CHECK_ROOT){
        		udp_packet *packet = data;
        		uint8_t ping=PING;
        		printf("Checking if edge router is reachable at ID %u\n", packet->dest_id);
        		ip_dest_p = servreg_hack_lookup((servreg_hack_id_t)packet->dest_id);
        		if(ip_dest_p==NULL)
        			printf("Service with ID %i is not provided in the network\n", packet->dest_id);
        		else{
					simple_udp_sendto(&udp_connection,							// Handler to identify connection
										&ping, 									// Data to be sent
										sizeof(ping),							// Length of data
										ip_dest_p);								// Destination IP-Address
        		}
        	}
        }
        PROCESS_END();
}
