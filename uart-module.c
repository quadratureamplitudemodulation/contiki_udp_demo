#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "contiki.h"
#include "contiki-net.h"
#include "dev/serial-line.h"
#include "uart-module.h"

#ifdef CC26XX_UART_CONF_ENABLE
#include "dev/cc26xx-uart.h"
#else
#include "dev/uart0.h"
#endif

#define COMPARE(string1, string2) (!strcmp(string1, string2))
#define TOKENIZE_START(string) strtok(string, " ")
#define TOKENIZE_RESUME strtok(NULL, " ");

PROCESS(uart_int_handler, "UART Interrupt Handler");

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
				unsigned long int buffer = strtoul(ptr, NULL, 0);
				if(buffer>127 && buffer<256){
					id=(unsigned short int)buffer;
					printf("ID: %i\n", id);
				}
				else{
					printf("ID is out of range. Must be from 128 to 255.\n");
				}
			}
			printf("Command not recognised. Did you mean one of the following?\nservice register [ID] : ID must be from 128 to 255\n");
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
