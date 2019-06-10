#ifndef INIT_PROCESS
#define INIT_PROCESS

#define CUSTOMER_EVENT_SEND_TO_ID	10
#define CUSTOMER_EVENT_REGISTER_ID		11

typedef struct{
	unsigned short int dest_id;
	char *data;
} udp_packet;

PROCESS_NAME(init_system_proc);
#endif
