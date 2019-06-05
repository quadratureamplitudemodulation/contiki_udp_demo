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
#include "dev/leds.h"

#ifdef CC26XX_UART_CONF_ENABLE 
#include "dev/cc26xx-uart.h"
#endif 

#define DEBUG_CC1310 1

#define UDP_PORT_CENTRAL 7878
#define UDP_PORT_OUT 5555
#define UART_BUFFER_SIZE 100
#define UART_END_LINE ';'
#define CLOCK_REPORT CLOCK_SECOND*2
static struct simple_udp_connection broadcast_connection;
static uip_ipaddr_t server_addr;
static uint16_t central_addr[] = {0xaaaa, 0, 0, 0, 0, 0, 0, 0x1};
static int sendBuffer = 0;
static char input_buffer[UART_BUFFER_SIZE];


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
        printf("########## UDP #########");
        printf("\nReceived from UDP Server:%s\n",data);
}

/**********************************************************
 * Int Handler for received UART bytes
 **********************************************************/
int uart_handler(unsigned char c){
	static int counter = UART_BUFFER_SIZE-1;
	if(c == UART_END_LINE){
		sendBuffer=1;
		printf("Int Handler.\n");
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
        static struct etimer periodic_timer;
	#ifdef CC26XX_UART_H_
	cc26xx_uart_init();
	cc26xx_uart_set_input(uart_handler);
	printf("CC130: Hello World");
	#endif
        

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

			PROCESS_WAIT_UNTIL(sendBuffer==1);
			#if DEBUG_CC1310
			printf("Main Thread.\n");
			#else
			simple_udp_sendto(&broadcast_connection,								// Handler to identify connection
											input_buffer, 							// Buffer of bytes to be sent
											strlen((const char *)input_buffer), 	// Length of buffer
											&server_addr);
			#endif
			sendBuffer=0;

        }
        PROCESS_END();
}
