#include "contiki.h"
#include "contiki-net.h"
#include "servreg-hack.h"
#include "simple-udp.h"
#include "net/ip/uip-debug.h"
#include "dev/button-sensor.h"
#include "net/rpl/rpl.h"
#include "uart-module.h"
#include "init_process.h"
#include "configuration.h"

#if RPLDEVICE != NODE
Error: RPLDEVICE must be NODE
#endif

#define UDP_PORT_LOCAL 1234										// Port at which the UDP socket is opened
#define UDP_PORT_OUT 1234										// Port to which UDP packets will be sent

PROCESS(init_system_proc, "Init system process");
AUTOSTART_PROCESSES(&init_system_proc);

/**
 * @brief Int Handler for received UDP packages.
 *
 * The Int handler has two functions:
 * 		- if the data is of length 2, it checks whether it's a response to a ping. If so, it changes the root service ID
 * 		- in any other case, it passes the data to init_system_process to process it
 */
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
	static route_packet packet;
	packet.int_addr = *sender_addr;
	packet.int_port = sender_port;
	packet.data = (char *)data;
	packet.datalen = datalen;
	process_post(&init_system_proc, CUSTOMER_EVENT_PROCESS_INCOMING_UDP, &packet);
	return;
}

/**
 *@brief Set global IPv6 Address
 *
 *Create an IPv6 address based on the MAC address of the device. Modified based on several Contiki examples.
 *@return Global IPv6 address as calculated by the function.
 */
static uip_ipaddr_t *set_global_address(void)
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


        static struct simple_udp_connection udp_connection;						// Handler for Simple-UDP Connection
		static uip_ipaddr_t *ip_addr;											// IP Address of this device
		static servreg_hack_id_t serviceID = 0;									// ID for servreg-hack. If it's 0, no service has been registered yet.
        static uip_ipaddr_t *ip_dest_p;											// Can be used to store the IP Address of a destination

		ip_addr = set_global_address();

		uart_init();
		process_start(&uart_int_handler, NULL);									// Refer to documentation of uart-module.h for these two lines

        rpl_set_mode(RPL_MODE_LEAF);											// Leaf mode means that a routing table will be created for both directions (up and down)

		servreg_hack_init();

        simple_udp_register(&udp_connection,									// Handler to identify this connection
                            UDP_PORT_OUT,										// Port for outgoing packages
                            NULL,												// Destination-IP-Address for outgoing packages->will be set individually for every packet
                            UDP_PORT_LOCAL,										// Port for incoming packages
                            cb_receive_udp);									// Int handler for incoming packages

        while (1) {
        	PROCESS_YIELD();

        	if(ev==CUSTOMER_EVENT_SEND_TO_ID || ev==CUSTOMER_EVENT_SEND_TO_IP){	// Send a UDP packet. Data contains all relevant information as udp_packt
        		udp_packet packet;
        		packet = *(udp_packet *)data;									// Create a local packet and synch it with data.
        		if(ev==CUSTOMER_EVENT_SEND_TO_ID){								// Destination information is in the ID
        			printf("Trying to reach ID: %i\n", packet.dest_id);
        			ip_dest_p = servreg_hack_lookup(packet.dest_id);			// Get receiver IP via Servreg-Hack
        		}
        		else															// Destination information is in the IP
        			ip_dest_p = &packet.dest_addr;
    			if(ip_dest_p==NULL)
    				printf("Server not found \n");								// ID or IP does not exist in this network
    			else{
    				printf("Server address ");
    				uip_debug_ipaddr_print(ip_dest_p);							// Print IP address the packet will be sent to
    				printf("\n");
    				uint8_t *data;
    				uint16_t datalen;
    				if(packet.dataIsChar){										// Payload is provided as string. Array ends with \0
    					data=(uint8_t *)packet.cData;
    					datalen=strlen(packet.cData);
    				}
    				else{														// Payload is provided as uint8_t. Array length is described with datalen
    					data=packet.ui8Data;
    					datalen=packet.datalen;
    				}
						int i;
						for(i=0; i<datalen; i++)
							printf("Data[%i]: %u\n", i, 0x00FF & data[i]);		// Print the value of each Byte of the Payload. Note that this will not map the data to ASCII

						simple_udp_sendto(&udp_connection,						// Handler to identify connection
										data, 									// Data to be sent
										datalen, 								// Length of data
										ip_dest_p);								// Destination IP-Address
    			}
        	}

        	else if(ev==CUSTOMER_EVENT_REGISTER_ID){							// Register a Service. ID is provided in data.
        		if(servreg_hack_lookup(*(servreg_hack_id_t *)data)==NULL){		// Check the ID. If no node provides a service with this ID, this function will return NULL
					printf("Register service with id %i\n", *(uint *)data);
					servreg_hack_register(*(servreg_hack_id_t *)data, ip_addr);	// Register service with given ID and IP
					serviceID = *(servreg_hack_id_t *)data;						// Store ID to use it later on
        		}
        		else
        			printf("ID %i already in use. Try another one.\n",*(uint *)data);
        	}
        	else if(ev==CUSTOMER_EVENT_GET_IP_FROM_ID){							// Get the IP address of the node providing a service with an ID. ID is stored in data
        		uip_ipaddr_t *ip = servreg_hack_lookup(*(servreg_hack_id_t *)data);
        		if(ip == NULL)
        			printf("Service with ID %i is not provided in the network\n", *(uint *)data);
        		else{
					printf("Service with ID %i is provided by IP ", *(uint *)data);
					uip_debug_ipaddr_print(ip);
					printf("\n");
        		}
        	}

        	else if(ev==CUSTOMER_EVENT_GET_LOCAL_ID){							// Get the local ID. No data required
        		if(serviceID == 0)												// No service has been registered yet
        			printf("No service registered yet.\n");
        		else
        			printf("Service registered with ID %i\n", serviceID);
        	}

        	else if(ev==CUSTOMER_EVENT_GET_LOCAL_IP){							// Print the IP address of the device. No data required
        		printf("The IPv6 address of this device is ");
        		uip_debug_ipaddr_print(ip_addr);
				printf("\n");
        	}

        	else if(ev==CUSTOMER_EVENT_CHECK_ROOT){								// Send a PING to the root with the ID described in a udp_packet which is provided by data
        		udp_packet *packet = data;
        		uint8_t ping=PING;
        		printf("Checking if edge router is reachable at ID %u\n", packet->dest_id);
        		ip_dest_p = servreg_hack_lookup((servreg_hack_id_t)packet->dest_id);
        		if(ip_dest_p==NULL)
        			printf("Service with ID %i is not provided in the network\n", packet->dest_id);
        		else{
					simple_udp_sendto(&udp_connection,							// Handler to identify connection
										&ping, 									// Data to be sent->PING
										sizeof(ping),							// Length of data
										ip_dest_p);								// Destination IP-Address
        		}
        	}
        	else if(ev==CUSTOMER_EVENT_PROCESS_INCOMING_UDP){					// Interpret the packet which has arrived through UDP. All necessary data is stored in data as a route_packet
			/**
			 * Please note: Upon convention the UDP packet which has arrived has the following structure if it's been routed from an external source:
			 * data[0..3]	 	: IP address of external source. This is an IPv4 address
			 * data[4] 			: DELIMITER
			 * data[5..6]		: UDP port of external source. This is to be interpreted as uint16_t
			 * data[7]			: DELIMITER
			 * data[8..n]		: Payload. Length is variable.
			 * data[n+1]		: DELIMITER
			 * Any packet which violates this structure must be from an internal source.
			 */
        		route_packet packet = *(route_packet *)data;
        		char *ptr;														// Used to parse the payload
        		int packet_is_from_external_source = 0;
        		int k;
        		ptr = strtok(packet.data, DELIMITER);
        		if(strlen(ptr)==4){												// Possible that the packet has arrived from an external sender
        			for(k=0; k<4; k++)
        				packet.ext_addr.u8[k] = (uint8_t)ptr[k];
        			ptr = strtok(NULL, DELIMITER);
        			if(strlen(ptr)==2){											// Still possible that the packet has arrived from an external sender
        				packet.ext_port=ptr[0]<<8 | (ptr[1]&0x00FF);
        				ptr = strtok(NULL, DELIMITER);
        				if(ptr != NULL){										// Still possible that the packet has arrived from an external sender
        					packet.data=ptr;
        					packet_is_from_external_source=1;
        				}

        			}
        		}
        		if(packet_is_from_external_source){
        			printf("A packet from an external source has arrived. \n");
        			printf("IP address of external source: ");
        			for(k=0; k<4; k++)
        				printf("%u.", packet.ext_addr.u8[k]);
        			printf("\n");
        			printf("Port of external source: %u\n", packet.ext_port);
        			printf("Payload: |%s|", packet.data);
        		}
        		else{
        			printf("A packet has arrived from IP ");
        			uip_debug_ipaddr_print(&packet.int_addr);
        			printf("\n");
        			printf("at port %u\n", packet.int_port);
        			printf("with the payload: |%s|", packet.data);
        		}

        	}
        }
        PROCESS_END();
}
