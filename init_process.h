#ifndef INITPROCESS
#define INITPROCESS	INITPROCESS

#include "uip.h"

#define CUSTOMER_EVENT_SEND_TO_IP		9
#define CUSTOMER_EVENT_SEND_TO_ID		10
#define CUSTOMER_EVENT_REGISTER_ID		11
#define CUSTOMER_EVENT_GET_IP_FROM_ID	12
#define CUSTOMER_EVENT_GET_LOCAL_ID		13
#define CUSTOMER_EVENT_GET_LOCAL_IP		14
#define CUSTOMER_EVENT_CHECK_ROOT		15
#define CUSTOMER_EVENT_REACT_TO_POLL	16

#define DELIMITER "."
#define PING "1"

typedef struct{
	unsigned short int dest_id;
	uip_ipaddr_t dest_addr;
	char *data;
} udp_packet;

PROCESS_NAME(init_system_proc);
#endif

