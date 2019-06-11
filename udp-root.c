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

//#define RELEASE
#define DEBUG
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
#ifdef DEBUG
	printf("Received data %s from ip ", data);
	uip_debug_ipaddr_print(sender_addr);
	printf(" on port %i\n", receiver_port);
#elif RELEASE
	uip_debug_ipaddr_print(sender_addr);
	printf(" %i %s", sender_port, data);
#endif
}

/**********************************************************
 * Create RPL dag
 **********************************************************/
static void
create_rpl_dag(uip_ipaddr_t *ipaddr)
{
  struct uip_ds6_addr *root_if;

  root_if = uip_ds6_addr_lookup(ipaddr);
  if(root_if != NULL) {
    rpl_dag_t *dag;
    uip_ipaddr_t prefix;

    rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
    dag = rpl_get_any_dag();
    uip_ip6addr(&prefix, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &prefix, 64);
#ifdef DEBUG
    PRINTF("created a new RPL dag\n");
  } else {
    PRINTF("failed to create a new RPL DAG\n");
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
		create_rpl_dag(ip_addr);

		uart_init();
		process_start(&uart_int_handler, NULL);

        /* IP Address of destination */
        uip_ipaddr_t *ip_dest_p;

		servreg_hack_init();

        simple_udp_register(&udp_connection,						// Handler to identify this connection
                            UDP_PORT_OUT,							// Port for outgoing packages
                            NULL,									// Destination-IP-Address for outgoing packages
                            UDP_PORT_CENTRAL,						// Port for incoming packages
                            cb_receive_udp);						// Int handler for incoming packages

        while (1) {
        	PROCESS_YIELD();
        	if(ev==CUSTOMER_EVENT_SEND_TO_ID){
        		udp_packet packet;
        		packet = *(udp_packet *)data;
#ifdef DEBUG
			printf("Trying to reach ID: %i\n", packet.dest_id);
#endif
    			ip_dest_p = servreg_hack_lookup(packet.dest_id);				// Get receiver IP via Servreg-Hack
    			if(ip_dest_p==NULL)
#ifdef DEBUG
    				printf("Server not found \n");
#endif
    			else
#ifdef DEBUG
    				printf("Server address ");
    				uip_debug_ipaddr_print(ip_dest_p);
    				printf("\n");
    				printf("Data: %s\n", packet.data);
#endif
    				simple_udp_sendto(&udp_connection,					// Handler to identify connection
    								packet.data, 						// Data to be sent
    								strlen(packet.data), 				// Length of data
    								ip_dest_p);							// Destination IP-Address*/
        	}
        	else if(ev==CUSTOMER_EVENT_REGISTER_ID){
        		printf("Register service with id %i\n", *(servreg_hack_id_t *)data);
        		servreg_hack_register(*(servreg_hack_id_t *)data, ip_addr);
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
        }
        PROCESS_END();
}

