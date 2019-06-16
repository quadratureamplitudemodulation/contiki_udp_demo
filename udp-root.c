#include <string.h>
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

#if RPLDEVICE!=ROOT
ERROR: RPL Device must be ROOT							// Create compiler error
#endif

#define UDP_PORT_LOCAL 1234								// Port at which the UDP socket is opened
#define UDP_PORT_OUT 1234								// Port to which UDP packets will be sent

#if MODE==RELEASE
#define SERVICEID	190									// In release, a service will be initially be registered with this ID.
#endif

PROCESS(init_system_proc, "Init system process");
AUTOSTART_PROCESSES(&init_system_proc);

/**
 * @brief Int Handler for received UDP packages.
 *
 * In release, the edge router only has two UDP functions.
 * 		- when a PING arrives, it reacts to the ping. To keep the Int handler short, the reaction itself is implemented	in init_system_proc
 * 		- when anything else arrives, it assumes that a packet to be routed has arrived. It fills a route_packet very quick and leaves most of the processing to init_system_proc.
 * Please note: There's no description of the params since this function only gets called by the OS anyways.
 */
void cb_receive_udp(struct simple_udp_connection *c,
                    const uip_ipaddr_t *sender_addr,
                    uint16_t sender_port,
                    const uip_ipaddr_t *receiver_addr,
                    uint16_t receiver_port,
                    const uint8_t *data,
                    uint16_t datalen) {
#if MODE==DEBUG
	printf("Received data %s from IP ", (char *)data);
	uip_debug_ipaddr_print(sender_addr);
	printf(" on port %i\n", receiver_port);
#endif


	if(datalen == 1){																	// Datalength = 1 -> Packet is a PING
		static udp_packet packet;														// Packet that will be filled and sent to init_system_process
		if(data[0]==PING){
#if MODE==DEBUG
			printf("Service ID ping received.\n");
#endif
			packet.dest_addr=*sender_addr;
			process_post(&init_system_proc, CUSTOMER_EVENT_REACT_TO_PING, &packet);		// Poll init_system_process
			return;
		}
	}

	else{																				// Datalength != 1 -> Packet is supposed to be routed
		static route_packet packet;														// This packet will be filled with all necessary information for a route packet.

		packet.int_port = sender_port;													// Fill packet with sender port
		packet.int_addr = *sender_addr;													// Fill packet with sender address
		packet.data = (char *)data;																// Simply fill the packet with the data received. Processing will be done in init_system_proc
		packet.datalen = datalen;
		process_post(&init_system_proc, CUSTOMER_EVENT_ROUTE_TO_EXTERN, &packet);
		return;
	}
}

/**
 * @brief Create RPL dag
 *
 * A function to create a new RPL dag. Modified based on several Contiki examples due to bad documentation of rpl.h
 * @param ipaddr Pointer to the IP-address of this device. It will be the root of the RPL network.
 */
static void create_rpl_dag(uip_ipaddr_t *ipaddr)
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
#if MODE==DEBUG
    PRINTF("created a new RPL dag\n");
  } else {
    PRINTF("failed to create a new RPL DAG\n");
  }
#endif
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

        static short unsigned int serviceID = 0;								// If a service is registered, this will be used to store the ID
        static struct simple_udp_connection udp_connection;						// Handler for Simple-UDP Connection
		static uip_ipaddr_t *ip_addr;											// IP Address of device
		static uip_ipaddr_t *ip_dest_p;											// Can be used to store the IP Address of a destination
        static char *ptr;														// Can be used to store return value of strtoken

		ip_addr = set_global_address();
		create_rpl_dag(ip_addr);

		uart_init();
		process_start(&uart_int_handler, NULL);									// Refer to documentation of uart-module.h for these two lines

		servreg_hack_init();
#if MODE==RELEASE
		servreg_hack_register((servreg_hack_id_t)SERVICEID, ip_addr);
		serviceID=(servreg_hack_id_t)SERVICEID;
#endif

        simple_udp_register(&udp_connection,									// Handler to identify this connection
                            UDP_PORT_OUT,										// Port for outgoing packages
                            NULL,												// Destination-IP-Address for outgoing packages->Individual for every packet
                            UDP_PORT_LOCAL,										// Port for incoming packages
                            cb_receive_udp);									// Int handler for incoming packages

        while (1) {
        	PROCESS_YIELD();

        	if(ev==CUSTOMER_EVENT_REACT_TO_PING){								// React with the PING followed by the ID under which a service is provided.
				if(serviceID>0){												// If service ID is still 0 (as in initialization), no service has been registered yet. Possible in DEBUG mode
        			udp_packet *packet = (udp_packet *)data;					// Cast data to a UDP packet
					uint8_t answer[2];											// The answer includes the PING and the service ID
					answer[0]=PING;
					answer[1]=serviceID;
#if MODE==DEBUG
					printf("Reacting to ping\n");
#endif
					simple_udp_sendto(&udp_connection,							// Handler to identify connection
										answer, 								// Send prepared answer
										sizeof(answer)/sizeof(answer[0]),		// Length of answer
										&packet->dest_addr);					// Destination IP-Address
				}
        	}
#if MODE==DEBUG
        	else if(ev==CUSTOMER_EVENT_REGISTER_ID){							// In DEBUG mode, a service registration can be commanded through UART
        		printf("Register service with id %i\n", *(servreg_hack_id_t *)data);
        		servreg_hack_register(*(servreg_hack_id_t *)data, ip_addr);
        		serviceID=*(servreg_hack_id_t *)data;
        	}

        	else if(ev==CUSTOMER_EVENT_SEND_TO_ID || ev==CUSTOMER_EVENT_SEND_TO_IP){	// Send a UDP packet. Data contains all relevant information as udp_packt
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
#endif
        	else if(ev==CUSTOMER_EVENT_ROUTE_TO_EXTERN){
        		/**
        				 * Please note: Upon convention the UDP packet which has arrived has the following structure:
        				 * data[0..3]	 	: IP address of external destination. This is an IPv4 address
        				 * data[4] 			: DELIMITER
        				 * data[5..6]		: UDP port of external destination. This is to be interpreted as uint16_t
        				 * data[7]			: DELIMITER
        				 * data[8..n]		: Payload. Length is variable.
        				 * data[n+1]		: DELIMITER
        				 * Any packet which violates this structure will be dropped with no notification (unless in DEBUG mode).
        				 */
        		route_packet packet;
        		packet = *(route_packet *)data;

				ptr=strtok((char *)packet.data,DELIMITER);												// Get first token -> external IP
				if(strlen(ptr)==4){																		// Only resume if external IP is an IPv4 address
        			int k;
					for(k=0; k<4; k++)
						packet.ext_addr.u8[k]=(uint8_t)ptr[k];											// Fill external address of packet with the IP address

					ptr=strtok(NULL, DELIMITER);														// Get second token -> external Port
        			if(strlen(ptr)==2){																	// Only resume if UDP port can be interpreted as uint16_t
         				packet.ext_port=ptr[0]<<8 | (ptr[1]&0x00FF);									// Fill external port of address with port

         				ptr=strtok(NULL, DELIMITER);													// The rest of the message is pure payload
        				if(ptr != NULL){																// strtok returns NULL, if no more DELIMITER was found. Only resume if this is not the case
#if MODE==DEBUG
						int k;
						printf("Route packet received.\n");
						printf("Received Data as UINT: \n");
						for(k=0; k<packet.datalen; k++)
							printf("Data[%i]: %u\n", k, ((uint8_t *)data)[k]);
#endif
        					packet.data=ptr;															// Fill packet data with ptr

							/**
							 * Note: Sending a packet through UDP has the following structure upon convention:
							 * Byte[0..1]		:	Destination Port
							 * Byte[2]			:	DELIMITER = '.'
							 * Byte[3..4]		:	Source Port
							 * Byte[5]			:	DELIMITER = '.'
							 * Byte[6..]		:	Destination IP-Address (can be 4 Bytes or 16 Bytes)
							 * Byte[n]			:	DELIMITER = '.'
							 * Byte[n+1..]		:	Source IP-Address (can be 4 Bytes or 16 Bytes)
							 * Byte[k]			:	DELIMITER = '.'
							 * Byte[k+1..]		:	Payload
							 * Last Byte		: 	SUPERFRAMEDELIMITER = '\n'
							 */
							printf("%u", packet.ext_port);
							printf(DELIMITER);
							printf("%u", packet.int_port);
							printf(DELIMITER);
							int i;
							for(i=0; i<4; i++)
								printf("%u", packet.ext_addr.u8[i]);
							printf(DELIMITER);
							for(i=0; i<16; i++)
								printf("%u", packet.int_addr.u8[i]);
							printf(DELIMITER);
							printf("%s", packet.data);
							printf("\n");
        				}
#if MODE==DEBUG
        				else
        					printf("No payload. Packet not a route packet. Received data: |%s|.\n", (char *)data);
#endif
        			}
#if MODE==DEBUG
        			else
        				printf("UDP Port doesn't have the right length. Packet not a route packet. Received data: |%s|.\n", (char *)data);
				}
#endif
#if MODE==DEBUG
				else
					printf("Destination IP has the wrong length. Packet not a route packet. Received data: |%s|.\n", (char *)data);
#endif
        	}
        }
        PROCESS_END();
}

