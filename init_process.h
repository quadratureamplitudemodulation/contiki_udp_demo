/**
 * @file init_process.h
 * @brief Includes important definitions for the project.
 */
#ifndef INITPROCESS
#define INITPROCESS	INITPROCESS
#include "uip.h"											// Needed for some typedefs
#include "configuration.h"
															// Customer events that will be used for polling processes
#define CUSTOMER_EVENT_SEND_TO_IP				9
#define CUSTOMER_EVENT_SEND_TO_ID				10
#define CUSTOMER_EVENT_REGISTER_ID				11
#define CUSTOMER_EVENT_GET_IP_FROM_ID			12
#define CUSTOMER_EVENT_GET_LOCAL_ID				13
#define CUSTOMER_EVENT_GET_LOCAL_IP				14
#define CUSTOMER_EVENT_PROCESS_INCOMING_UDP		15

#if RPLDEVICE == NODE
#define CUSTOMER_EVENT_CHECK_ROOT				16			// Only nodes can check for the root
#endif
#if RPLDEVICE == ROOT										// Only roots can react to pings or route packets
#define CUSTOMER_EVENT_REACT_TO_PING			17
#define CUSTOMER_EVENT_ROUTE_TO_EXTERN 			18
#define CUSTOMER_EVENT_ROUTE_TO_INTERN			19
#endif

// Note: The payload can be stored in a string or a uint8_t array. The only difference is that a string is expected to end with \0 and for a uint8_t* the length of the array must be provided with datalen.
typedef struct{
	unsigned short int dest_id;
	uip_ipaddr_t dest_addr;
	uint8_t dataIsChar;
	char *cData;
	uint8_t *ui8Data;
	uint16_t datalen;
} udp_packet;

typedef struct{
	uip_ip4addr_t ext_addr;
	uip_ip6addr_t int_addr;
	uint16_t ext_port;
	uint16_t int_port;
	char* data;
	uint16_t datalen;
} route_packet;

PROCESS_NAME(init_system_proc);								// Make init_system_proc be visible for all files. Needed to poll it from outside its own file
#endif

#if MODE!=DEBUG && MODE!=RELEASE
Error: Mode must be either DEBUG or RELEASE
#endif
#if RPLDEVICE!=ROOT && RPLDEVICE!=NODE
Error: RPLDEVICE must be either ROOT or NODE
#endif
#if TARGET!=Z1 && TARGET!=CC1310
Error: TARGET must be either Z1 or CC1310
#endif
