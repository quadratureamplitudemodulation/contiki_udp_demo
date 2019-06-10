#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "dev/serial-line.h"
#include "uart-module.h"
#include "init_process.h"

#ifdef CC26XX_UART_CONF_ENABLE
#include "dev/cc26xx-uart.h"
#else
#include "dev/uart0.h"
#endif

#define COMPARE(string1, string2) (!strcmp(string1, string2))
#define TOKENIZE_START(string) strtok(string, " ")
#define TOKENIZE_RESUME strtok(NULL, " ");

PROCESS(uart_int_handler, "UART Interrupt Handler");

static short int convertid(char *string, unsigned short int *target){
	unsigned int i;
	for(i=0; i<3; i++)
		if(string[i]<'0' || string[i]>'9'){
			*target=255;
			return 0;
		}
	i = ((unsigned int)string[0]-'0')*100+((unsigned int)string[1]-'0')*10+((unsigned int)string[2]-'0');
	if(i>127 && i<256)
		*target=(unsigned short int) i;
	else{
		*target=0;
		return 0;
	}
	return 1;
}

PROCESS_THREAD(uart_int_handler, ev, data){
	PROCESS_BEGIN();
	while(1){
		PROCESS_YIELD_UNTIL(ev==serial_line_event_message);
		static char *ptr;
		static short unsigned int id;

		ptr=TOKENIZE_START((char *)data);

		if(COMPARE(ptr, "service")){
			ptr=TOKENIZE_RESUME;
			if(COMPARE(ptr, "register")){
				ptr=TOKENIZE_RESUME;
				if(convertid(ptr, &id)){
					printf("Succesfull. ID: %i \n",id);
				}
				else{
					if(id)
						printf("Please type a valid number for ID.\n");
					else
						printf("ID is out of range. Must be from 128 to 255.\n");
				}
			}
			else
				printf("Command not recognised. Did you mean one of the following?\n'service register [ID]' : ID must be from 128 to 255\n");
		}
		else if(COMPARE(ptr, "udp")){
			ptr=TOKENIZE_RESUME;
			if(COMPARE(ptr, "send")){
				ptr=TOKENIZE_RESUME;
				if(COMPARE(ptr, "id")){
					ptr=TOKENIZE_RESUME;
					if(convertid(ptr, &id)){
						ptr=TOKENIZE_RESUME;
						printf("Sending to ID %i the data %s\n", id, ptr);
						process_post(&init_system_proc, PROCESS_EVENT_POLL, ptr);
					}
					else{
						if(id)
							printf("Please type a valid number for ID.\n");
						else
							printf("ID is out of range. Must be from 128 to 255.\n");
					}
				}
				else
					printf("Command not recognised. Did you mean one of the following?\n'udp send id [ID] [data]: Send data to ID. ID must be from 128 to 255'\n");
			}
			else
				printf("Command not recognised. Did you mean one of the following?\n'udp send id [ID] [data]: Send data to ID. ID must be from 128 to 255'\n");

		}

		else{
			printf("Command not recognised.\n");
		}
	}
	PROCESS_END();
}


void uart_init(void){
	serial_line_init();
#ifdef CC26XX_UART_H_
	cc26xx_uart_init();
	cc26xx_uart_set_input(serial_line_input_byte);
#else
	uart0_init(0);
	uart0_set_input(serial_line_input_byte);
#endif
}

