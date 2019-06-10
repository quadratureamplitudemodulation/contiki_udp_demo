#include "contiki.h"
#include "contiki-net.h"
#include "net/ip/uip-debug.h"
#include "servreg-hack.h"
#include "simple-udp.h"
#include "dev/button-sensor.h"
#include "net/rpl/rpl.h"

#ifdef CC26XX_UART_CONF_ENABLE
#include "dev/cc26xx-uart.h"
#include "dev/serial-line.h"
#endif

/* Normally the software runs to be used on a CC1310. However if it's supposed to be debugged, on of the following
 * defines can be used.
 */

//#define DEBUG_CC1310	// CC1310 will not send UDP packets, but UART messages
#define DEBUG_Z1	// Node will not react to UART messages, but send "Hello World" via UDP when Button is pressed

#define UDP_PORT_CENTRAL 1234
#define UDP_PORT_OUT 1234

/* ID for Servrag-Hack. Must be the same as on sender side */
#define SERVICE_ID_ROOT 190

/* Handler for Simple-UDP Connection */
static struct simple_udp_connection udp_connection;

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
#ifdef DEBUG_COOJA
		printf("Received data via UDP: %s \n", data);
		printf("From Port: %i \n", sender_port);
		if(sender_addr == NULL)
			printf("No sender address");
		else{
			printf("From address: ");
			uip_debug_ipaddr_print(sender_addr);
		}
		simple_udp_sendto_port(c,								// Handler to identify connection
						data, 									// Buffer of bytes to be sent
						datalen,								// Length of buffer
						sender_addr,							// Destination IP-Address
						sender_port);
#elif DEBUG_CC1310
		printf("Received data via UDP: %s \n", data);
#else
		printf("%s", data);
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
    PRINTF("created a new RPL dag\n");
  } else {
    PRINTF("failed to create a new RPL DAG\n");
  }
}

/**********************************************************
 * Main process
 **********************************************************/
PROCESS_THREAD(init_system_proc, ev, data){
        PROCESS_BEGIN();

        /* IP Address of device */
        static uip_ipaddr_t *ip_addr;

        /* ID for Servrag-Hack. Must be the same as on sender side */
        static servreg_hack_id_t serviceID = SERVICE_ID_ROOT;

#ifdef CC26XX_UART_H_
		cc26xx_uart_init();
		cc26xx_uart_set_input(uart_handler);
#ifdef DEBUG_CC1310
		printf("CC130: Hello World");
#endif
#endif

		/* Set IP-Address and create a RPL DAG */
		ip_addr = set_global_address();
		create_rpl_dag(ip_addr);

#ifdef DEBUG_COOJA
		printf(" IP: ");
        uip_debug_ipaddr_print(ip_addr);
#elif DEBUG_CC1310
        printf(" IP: ");
        uip_debug_ipaddr_print(ip_addr);
#endif
        /* Provide a Servreg-Hack service to share the IP Address */
        servreg_hack_init();
		servreg_hack_register(serviceID, ip_addr);

        // Register with Simple-UDP
        simple_udp_register(&udp_connection,					// Handler to identify this connection
                            UDP_PORT_OUT,							// Port for outgoing packages
                            NULL,									// Destination-IP-Address for outgoing packages
                            UDP_PORT_CENTRAL,						// Port for incoming packages
                            cb_receive_udp);						// Int handler for incoming packages
#ifdef DEBUG_COOJA
        printf("\n Initialized. \n");
#elif DEBUF_CC1310
        printf("\n Initialized. \n");
#endif
        while (1) {
			PROCESS_YIELD();
        }
        PROCESS_END();
}
