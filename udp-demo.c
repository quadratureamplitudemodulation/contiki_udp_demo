#include "contiki.h"
#include "contiki-net.h"
#include "servreg-hack.h"
#include "simple-udp.h"
#include "net/ip/uip-debug.h"
#include "dev/button-sensor.h"


#ifdef CC26XX_UART_CONF_ENABLE 
#include "dev/cc26xx-uart.h"
#include "dev/serial-line.h"
#endif 

/* Normally the software runs to be used on a CC1310. However if it's supposed to be debugged, on of the following
 * defines can be used.
 */

//#define DEBUG_CC1310	// CC1310 will not send UDP packets, but UART messages
#define DEBUG_COOJA	// Node will not react to UART messages, but send "Hello World" via UDP when Button is pressed

#define UDP_PORT_CENTRAL 1234
#define UDP_PORT_OUT 1234

#define UART_BUFFER_SIZE 100
#define UART_END_LINE ';'

static char input_buffer[UART_BUFFER_SIZE] ={"Hello World"};	// Message that was received via UART and gets send via UDP

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
 }

/**********************************************************
 * Int Handler for received UART bytes
 **********************************************************/
int uart_handler(unsigned char c){
	static int counter = UART_BUFFER_SIZE-1;
	if(c == UART_END_LINE){
		process_poll(&init_system_proc);
		counter=UART_BUFFER_SIZE-1;
	}
	else {
		input_buffer[counter]=c;
		counter--;
		if(counter==0)
			counter=UART_BUFFER_SIZE-1;
	}
	return 1;
}
/**********************************************************
 * Main process
 **********************************************************/
PROCESS_THREAD(init_system_proc, ev, data){
        PROCESS_BEGIN();

        /* Handler for Simple-UDP Connection */
        static struct simple_udp_connection udp_connection;

        /* IP Address of destination */
        uip_ipaddr_t *ip_dest_p;

        /* ID for Servrag-Hack. Must be the same as on receiver side */
        static servreg_hack_id_t serviceID = 190;

        SENSORS_ACTIVATE(button_sensor);

        #ifdef CC26XX_UART_H_
		cc26xx_uart_init();
		cc26xx_uart_set_input(uart_handler);
		printf("CC130: Hello World");
		#endif

		servreg_hack_init();

        simple_udp_register(&udp_connection,						// Handler to identify this connection
                            UDP_PORT_OUT,							// Port for outgoing packages
                            NULL,									// Destination-IP-Address for outgoing packages
                            UDP_PORT_CENTRAL,						// Port for incoming packages
                            cb_receive_udp);						// Int handler for incoming packages
        printf("Initialized\n");
        while (1) {
			#ifdef DEBUG_COOJA
        	PROCESS_YIELD_UNTIL(ev==sensors_event);
			#else
        	PROCESS_YIELD_UNTIL(ev==PROCESS_EVENT_POLL);
			#endif

        	#ifdef DEBUG_CC1310
			printf(input_buffer);
			#else
			ip_dest_p = servreg_hack_lookup(serviceID);				// Get receiver IP via Servreg-Hack
			if(ip_dest_p==NULL)
				printf("\n Server not found \n");
			else
				printf("\n Server address ");
				uip_debug_ipaddr_print(ip_dest_p);
				simple_udp_sendto(&udp_connection,					// Handler to identify connection
								input_buffer, 						// Buffer of bytes to be sent
								strlen((const char *)input_buffer), // Length of buffer
								ip_dest_p);							// Destination IP-Address
			#endif

        }
        PROCESS_END();
}
