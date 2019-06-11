#ifndef INITPROCESS
#define INITPROCESS	INITPROCESS

#define CUSTOMER_EVENT_SEND_TO_ID		10
#define CUSTOMER_EVENT_REGISTER_ID		11
#define CUSTOMER_EVENT_GET_IP_FROM_ID	12
#define CUSTOMER_EVENT_GET_LOCAL_ID		13
#define CUSTOMER_EVENT_GET_LOCAL_IP		14

typedef struct{
	unsigned short int dest_id;
	char *data;
} udp_packet;

PROCESS_NAME(init_system_proc);
#endif
