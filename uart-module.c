#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "contiki.h"
#include "contiki-net.h"
#include "dev/serial-line.h"
#include "uart-module.h"
#include "init_process.h"
#include "uip-debug.h"

#ifdef CC26XX_UART_CONF_ENABLE
#include "dev/cc26xx-uart.h"
#else
#include "dev/uart0.h"
#endif

#define COMPARE(string1, string2) (!strcmp(string1, string2))
#define TOKENIZE_START(string) strtok(string, " ")
#define TOKENIZE_RESUME strtok(NULL, " ")
#define TOKENIZE_REST strtok(NULL, "\0")

static uint8_t vsroot_id = 190;

PROCESS(uart_int_handler, "UART Interrupt Handler");

void change_root_id(uint8_t id){
	printf("Edge Routers Service ID is %u\n", id);
	vsroot_id=id;
}

static short int convertid(char *string, unsigned short int *target){
	unsigned int i;
	if(strlen(string) != 3){
		printf("ID must be a three digit number.\n");
l		printf("%s\n", string);
		return 0;
	}
	for(i=0; i<3; i++)
		if(string[i]<'0' || string[i]>'9'){
			printf("Please type a valid number as ID.\n");
			*target=255;
			return 0;
		}
	i = ((unsigned int)string[0]-'0')*100+((unsigned int)string[1]-'0')*10+((unsigned int)string[2]-'0');
	if(i>127 && i<256)
		*target=(unsigned short int) i;
	else{
		printf("ID is out of range. Must be from 128 to 255.\n");
		*target=0;
		return 0;
	}
	return 1;
}

static unsigned short int sttous(char string[2]){
	if(string[0]>='a' && string[0]<='f'){
		if(string[1]>='a' && string[1]<='f'){
			return ((unsigned short)(string[0]-'W'))*16+(unsigned short)(string[1]-'W');
		}
		else{
			return ((unsigned short)(string[0]-'W'))*16+(unsigned short)(string[1]-'0');
		}
	}
	else{
		if(string[1]>='a' && string[1]<='f'){
			return ((unsigned short)(string[0]-'0'))*16+(unsigned short)(string[1]-'W');
		}
		else{
			return ((unsigned short)(string[0]-'0'))*16+(unsigned short)(string[1]-'0');
		}
	}
}
/**
 * Function to count appearences of needle in string. Source: https://www.sanfoundry.com/c-program-count-occurence-substring/
 */
static uint amstrstr(char *string, char *needle){
    int i, j, l1, l2;
    uint count=0, count1=0;
    l1=strlen(string);
    l2=strlen(needle);
	for (i = 0; i < l1;)
    {
        j = 0;
        count = 0;
        while ((string[i] == needle[j]))
        {
            count++;
            i++;
            j++;
        }
        if (count == l2)
        {
            count1++;
            count = 0;
        }
        else
            i++;
    }
	return count1;
}
/**
 * Version of strtok which returns NULL when two delimiters appear in a row. Important for IPv6 addressing. Slightly modified based on: https://stackoverflow.com/questions/8705844/need-to-know-when-no-data-appears-between-two-token-separators-using-strtok
 */
char *strtok_single (char * str, char const * delims)
{
  static char  * src = NULL;
  char  *  p,  * ret = 0;

  if (str != NULL)
    src = str;

  if (src == NULL)
    return NULL;

  if ((p = strpbrk (src, delims)) != NULL) {
    *p  = 0;
    ret = src;
    src = ++p;

  } else if (*src) {
    ret = src;
    src = NULL;
  }
  return ret;
}

static uip_ip6addr_t strtoipv6(char *ip){
	char *ptr;
	char buf;
	char *string=&buf;
	uip_ip6addr_t addr;
	unsigned short int i;
	uint tokens = amstrstr(ip,":");
	if(ip[0]==':' || ip[15]==':')
		tokens--;
	for(i=0; i<16; i++){
		if(i==0)
			ptr = strtok_single(ip,":");
		else if(i<14)
			ptr=strtok_single(NULL,":");
		else
			ptr=strtok_single(NULL,"\0");
		if (strlen(ptr)==0){
			uint h;
			for(h=0; h<(8-tokens)*2; h++){
				addr.u8[i]=0;
				i++;
			}
			i--;
		}
		else if(strlen(ptr)==4){
			string=ptr;
			addr.u8[i]=sttous(string);
			string=&ptr[2];
			i++;
			addr.u8[i]=sttous(string);
		}
		else if(strlen(ptr)==3){
			string[0]='0';
			string[1]=ptr[0];
			addr.u8[i]=sttous(string);
			string=&ptr[1];
			i++;
			addr.u8[i]=sttous(string);
		}
		else if(strlen(ptr)==2){
			addr.u8[i]=0;
			string = ptr;
			i++;
			addr.u8[i]=sttous(string);
		}
		else if(strlen(ptr)==1){
			addr.u8[i]=0;
			string[0]='0';
			string[1]=ptr[0];
			i++;
			addr.u8[i]=sttous(string);
		}
	}
	return addr;
}

static uint strtoipv4(char *ip, uip_ip4addr_t *addr){
	char *ptr;
	unsigned long int buf;
	unsigned short int i;
	for(i=0; i<4; i++){
		if(i==0)
			ptr = strtok_single(ip,".");
		else if(i<3)
			ptr = strtok_single(NULL,".");
		else
			ptr = strtok_single(NULL,"\0");
		if(strlen(ptr)>3 || ptr==NULL)
			return 0;
		buf = strtoul(ptr, NULL, 0);
		if(buf > 255)
			return 0;
		addr->u8[i]=(unsigned short)buf;
	}
	return 1;
}

PROCESS_THREAD(uart_int_handler, ev, data){
	PROCESS_BEGIN();
	while(1){
		PROCESS_YIELD_UNTIL(ev==serial_line_event_message);
		static char *ptr;
		static short unsigned int id;
		static udp_packet packet;
		printf("Input: %s\n", (char*)data);
		ptr=TOKENIZE_START((char *)data);

		if(COMPARE(ptr, "service")){
			ptr=TOKENIZE_RESUME;
			if(COMPARE(ptr, "register")){
				ptr=TOKENIZE_REST;
				if(convertid(ptr, &id)){
					process_post(&init_system_proc, CUSTOMER_EVENT_REGISTER_ID, &id);
				}
			}
			else if(COMPARE(ptr, "get")){
				ptr=TOKENIZE_RESUME;
				if(COMPARE(ptr, "ip")){
					ptr=TOKENIZE_REST;
					if(convertid(ptr, &id)){
						process_post(&init_system_proc, CUSTOMER_EVENT_GET_IP_FROM_ID, &id);
					}
				}
				else
					printf("Command not recognised. Did you mean one of the following?\n'service get ip [ID]'\nNote: [ID] must be from 128 to 255\n");
			}
			else if(COMPARE(ptr, "edge")){
				ptr=TOKENIZE_REST;
				if(convertid(ptr, &id)){
					packet.dest_id = id;
					process_post(&init_system_proc, CUSTOMER_EVENT_CHECK_ROOT, &packet);
				}
			}
			else
				printf("Command not recognised. Did you mean one of the following?\n'service register [ID]'\n'service get ip [ID]'\nNote: [ID] must be from 128 to 255\n");
		}
		else if(COMPARE(ptr, "udp")){
			ptr=TOKENIZE_RESUME;
			if(COMPARE(ptr, "send")){
				ptr=TOKENIZE_RESUME;
				if(COMPARE(ptr, "id")){
					ptr=TOKENIZE_RESUME;
					if(convertid(ptr, &id)){
						ptr=TOKENIZE_REST;
						printf("Sending via ID\n");
						packet.dest_id=id;
						packet.data=ptr;
						process_post(&init_system_proc, CUSTOMER_EVENT_SEND_TO_ID, &packet);
					}
				}
				else if(COMPARE(ptr, "ip")){
					ptr=TOKENIZE_RESUME;
					packet.dest_addr = strtoipv6(ptr);
					ptr=TOKENIZE_REST;
					printf("Sending via IP\n");
					packet.data=ptr;
					process_post(&init_system_proc, CUSTOMER_EVENT_SEND_TO_IP, &packet);
				}
				else if(COMPARE(ptr, "extern")){
					if(vsroot_id>0){
						ptr=TOKENIZE_RESUME;
						static uip_ip4addr_t addr;
						if(strtoipv4(ptr, &addr)){
							ptr=TOKENIZE_RESUME;
							if(strlen(ptr)<=5){
								uint32_t buf = strtoul(ptr, NULL, 0);
								if(buf < 65536){
									static uint8_t udpPort[2];
									udpPort[0] = (uint8_t)((uint16_t)buf>>8);
									udpPort[1] = (uint8_t)(buf&0xFF);
									//udpPort[0]=192;
									//udpPort[1]=200;
									printf("Port[0]: %u\n", udpPort[0]);
									printf("Port[1]: %u\n", udpPort[1]);
									//printf("Port[2]: %u\n", port[2]);
									ptr=TOKENIZE_REST;
									packet.dest_id=vsroot_id;
									strcat(ptr, DELIMITER);
									strcat(ptr, (char *)udpPort);
									strcat(ptr, DELIMITER);
									strcat(ptr, (char *)addr.u8);
									strcat(ptr, DELIMITER);
									packet.data=ptr;
									process_post(&init_system_proc, CUSTOMER_EVENT_SEND_TO_ID, &packet);

								}
								else
									printf("Invalid port number");
							}
							else
								printf("Invalid port number");
						}
						else
							printf("Not a valid IPv4 address.\n");
					}
					else
						printf("Error: Please provide Service ID of Edge Router via service edge [ID] and then try again.\n");
				}
				else
					printf("Command not recognised. Did you mean one of the following?\n'udp send id [ID] [data]: Send data to ID. ID must be from 128 to 255'\n");
			}
			else
				printf("Command not recognised. Did you mean one of the following?\n'udp send id [ID] [data]: Send data to ID. ID must be from 128 to 255'\n");

		}
		else if(COMPARE(ptr, "print")){
			ptr = TOKENIZE_RESUME;
			if(COMPARE(ptr, "id")){
				process_post(&init_system_proc, CUSTOMER_EVENT_GET_LOCAL_ID, NULL);
			}
			if(COMPARE(ptr, "ip")){
				process_post(&init_system_proc, CUSTOMER_EVENT_GET_LOCAL_IP, NULL);
			}

		}

		else{
			printf("Command not recognised.\n");
		}
	}
	PROCESS_END();
}


void uart_init(void){
	serial_line_init();
#if RELEASE
	cc26xx_uart_init();
	cc26xx_uart_set_input(serial_line_input_byte);
#else
	uart0_init(0);
	uart0_set_input(serial_line_input_byte);
#endif
}

