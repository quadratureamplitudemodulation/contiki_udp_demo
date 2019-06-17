#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "contiki.h"
#include "contiki-net.h"
#include "dev/serial-line.h"
#include "uart-module.h"
#include "init_process.h"
#include "uip-debug.h"
#include "configuration.h"

#if TARGET==CC1310
#include "dev/cc26xx-uart.h"
#elif	TARGET==Z1
#include "dev/uart0.h"
#endif

/*
 * @brief Compare two strings
 * @return 1 if equal, 0 if not
 */
#define COMPARE(string1, string2) (!strcmp(string1, string2))			

/*
 * @brief Start parsing a string for a space
 * @param string String to be parsed
 * @return First token
 */
#define TOKENIZE_START(string) strtok(string, " ")								
/*
 * @brief Continue parsing the last string for a space
 * @return Next token
 */
#define TOKENIZE_RESUME strtok(NULL, " ")
/*
 * @brief Continue parsing the last string until the end of the string
 * @return Rest of the string
 */
#define TOKENIZE_REST strtok(NULL, "\0")

#if RPLDEVICE==NODE
static uint8_t root_id = 0;																// Service ID of the root device. If 0, no root is known yet.
#endif

PROCESS(uart_int_handler, "UART Interrupt Handler");

#if RPLDEVICE==NODE || MODE==DEBUG
#if RPLDEVICE==NODE
/*
 * @brief Change the service ID of the root as stored in this library
 * @param id Service ID to be stored.
 */
void change_root_id(uint8_t id){
	printf("Edge Routers Service ID is %u\n", id);
	root_id=id;
}
#endif

/*
 * @brief Convert a service ID stored in a string into a uint8_t.
 * @param string String in which the service ID is stored. Length must be three.
 * @param target Pointer to the uint8_t that the result is supposed to be stored in.
 * @return 0 if not succesfull, 1 if successful
 */
static short int convertid(char *string, unsigned short int *target){
	unsigned int i;
	if(strlen(string) != 3){
		printf("ID must be a three digit number.\n");
		printf("%s\n", string);
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

/*
 * @brief Convert a string to a uint8_t. The digits of the string will be interpreted as hexadecimal values.
 * @param string[2] Char-array in which the string to be converted is stored
 * @return Converted uint8_t
 */
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
 * @brief Function to count appearances of needle in string. Copied from Internet.
 *
 *  Source: https://www.sanfoundry.com/c-program-count-occurence-substring/
 *  @param string String that the needle appears in.
 *  @param needle Needle that will be searched for in the string
 *  @return Amount of appearances of needle in string
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
 * @brief Version of strtok which returns NULL when two delimiters appear in a row.
 *
 * Important for IPv6 addressing. Copied from https://stackoverflow.com/questions/8705844/need-to-know-when-no-data-appears-between-two-token-separators-using-strtok
 * @param str String that will be tokenized. If NULL, the last string provided will be continued.
 * @param delims Delimiter to search for
 * @return Token which from beginning until delims appears. NULL if first symbol of str is delims.
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

/*
 * @brief Convert an IPv6 address provided in a string into a ui_ip6_addr_t
 *
 * The function deals with almost all conventions when it comes to writing IPv6 addresses. Only "::" at the very beginning or at the end and '.' instead of ':' cannot be interpreted and lead
 * to unusable results. It does not check for misspellings yet, so be careful to provide a correct address.
 *
 * @param ip String that contains the IPv6 address
 * @return IP address converted from string
 */
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

#if RPLDEVICE==NODE
/*
 * @brief Convert a string which contains a human readable IPv4 address into a uip_ip4addr_t address.
 * @param ip String that includes the IP address
 * @param addr Pointer to the address the IP address will be stored in
 * @return 1 if succesfull, 0 if not
 */
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
#endif
#endif

PROCESS_THREAD(uart_int_handler, ev, data){
	PROCESS_BEGIN();
	while(1){
		PROCESS_YIELD_UNTIL(ev==serial_line_event_message);
		static char *ptr;																			// Used for result of strtok
		static short unsigned int id;																// Can be used to store an service ID
#if RPLDEVICE==NODE || MODE==DEBUG
		static udp_packet packet;																	// Can be used to fill any UDP packet
		printf("Input: %s\n", (char*)data);															// Print the input
		ptr=TOKENIZE_START((char *)data);

		if(COMPARE(ptr, "service")){

			ptr=TOKENIZE_RESUME;
			if(COMPARE(ptr, "register")){

				ptr=TOKENIZE_REST;																	// Rest should be the ID to register a service
				if(convertid(ptr, &id)){															// If 1, a correct ID has been provided. If not, convertid will notify the user
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
#if RPLDEVICE==NODE
			else if(COMPARE(ptr, "edge")){
				ptr=TOKENIZE_REST;
				if(convertid(ptr, &id)){
					packet.dest_id = id;
					process_post(&init_system_proc, CUSTOMER_EVENT_CHECK_ROOT, &packet);
				}
			}
			else
				printf("Command not recognised. Did you mean one of the following?\n'service register [ID]'\n'service get ip [ID]'\n'service edge [ID]'\nNote: [ID] must be from 128 to 255\n");
#elif RPLDEVICE==ROOT
			else
				printf("Command not recognised. Did you mean one of the following?\n'service register [ID]'\n'service get ip [ID]'\nNote: [ID] must be from 128 to 255\n");
#endif
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
						packet.dataIsChar=1;
						packet.cData=ptr;
						process_post(&init_system_proc, CUSTOMER_EVENT_SEND_TO_ID, &packet);
					}
				}
				else if(COMPARE(ptr, "ip")){
					ptr=TOKENIZE_RESUME;
					uip_ipaddr_t dest_addr = strtoipv6(ptr);

					ptr=TOKENIZE_REST;
					printf("Sending via IP\n");
					packet.dest_addr=dest_addr;
					packet.dataIsChar=1;
					packet.cData=ptr;
					process_post(&init_system_proc, CUSTOMER_EVENT_SEND_TO_IP, &packet);
				}
#if RPLDEVICE==NODE
				else if(COMPARE(ptr, "extern")){
					if(root_id>0){
						ptr=TOKENIZE_RESUME;
						static uip_ip4addr_t addr;
						if(strtoipv4(ptr, &addr)){
							ptr=TOKENIZE_RESUME;
							if(strlen(ptr)<=5){
								uint32_t buf = strtoul(ptr, NULL, 0);
								if(buf < 65536){
									static uint8_t udpPort[2];
									int i,k;
									static uint8_t externPayload[MAX_PAYLOAD_EXTERN+9];
									udpPort[0] = (uint8_t)((uint16_t)buf>>8);
									udpPort[1] = (uint8_t)(buf&0xFF);
									ptr=TOKENIZE_REST;
									if(strlen(ptr)<=MAX_PAYLOAD_EXTERN){
										uint16_t datalen = sizeof(addr.u8)/sizeof(addr.u8[0])+sizeof(udpPort)/sizeof(udpPort[0])+strlen(ptr)+3;
										for(i=0; i<sizeof(addr.u8)/sizeof(addr.u8[0]);i++)
											externPayload[i] = addr.u8[i];
										externPayload[i] = (uint8_t)DELIMITER[0];
										i++;
										k=i;
										for(i=0; i<sizeof(udpPort)/sizeof(udpPort[0]);i++)
											externPayload[i+k] = udpPort[i];
										externPayload[i+k] = (uint8_t)DELIMITER[0];
										i++;
										k+=i;
										for(i=0; i<strlen(ptr); i++)
											externPayload[i+k]=(uint8_t)ptr[i];
										externPayload[i+k] = (uint8_t)DELIMITER[0];
										i++;
										datalen=i+k;
										packet.dest_id=root_id;
										packet.dataIsChar=0;
										packet.ui8Data=externPayload;
										packet.datalen=datalen;
										process_post(&init_system_proc, CUSTOMER_EVENT_SEND_TO_ID, &packet);
									}
									else
										printf("Payload is too long. Please compile with a highe max payload or provide a a smaller payload.\n");
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
					printf("Command not recognised. Did you mean one of the following?\n'udp send id [ID] [data]'\n'udp send ip [IP] [data]'\n'udp send extern [IP] [port] [data]'\n");
#elif RPLDEVICE==ROOT
				else
					printf("Command not recognised. Did you mean one of the following?\n'udp send id [ID] [data]'\n'udp send ip [IP] [data]'\n");
#endif
			}
#if RPLDEVICE==NODE
			else
				printf("Command not recognised. Did you mean one of the following?\n'udp send id [ID] [data]'\n'udp send ip [IP] [data]'\n'udp send extern [IP] [port] [data]'\n");
#elif RPLDEVICE==ROOT
			else
				printf("Command not recognised. Did you mean one of the following?\n'udp send id [ID] [data]'\n'udp send ip [IP] [data]'\n");
#endif
		}
		else if(COMPARE(ptr, "print")){
			ptr = TOKENIZE_RESUME;
			if(COMPARE(ptr, "id")){
				process_post(&init_system_proc, CUSTOMER_EVENT_GET_LOCAL_ID, NULL);
			}
			else if(COMPARE(ptr, "ip")){
				process_post(&init_system_proc, CUSTOMER_EVENT_GET_LOCAL_IP, NULL);
			}
			else
				printf("Command not recognised. Did you mean one of the following)\n'print id'\n'print id'\n");
		}
		else if(COMPARE(ptr, "hello")){
#if RPLDEVICE==NODE
			printf("Hello user! I am a node.\n");
#elif RPLDEVICE==ROOT
			printf("Hello user! I am a root.\n");
#endif
		}
		else{
#if RPLDEVICE==NODE
			printf("Command not recognised.\n");
#elif RPLDEVICE==ROOT
			printf("Input is not a command. It could still be a message to be routed.\n");
#endif
		}
#endif

#if RPLDEVICE==ROOT
		static route_packet packet_route;
		ptr=strtok((char *)data, DELIMITER);
		if(strlen(ptr)==2){
			packet_route.int_port=ptr[0]<<8 | (ptr[1]&0x00FF);
			ptr=strtok(NULL, DELIMITER);
			if(strlen(ptr)==2){
				packet_route.ext_port=ptr[0]<<8 | (ptr[1]&0x00FF);
				ptr=strtok(NULL, DELIMITER);
				if(strlen(ptr)==16){
					int i;
					for(i=0; i<16; i++)
						packet_route.int_addr.u8[i]=ptr[i]&0xFF;
					ptr=strtok(NULL, DELIMITER);
					if(strlen(ptr)==4){
						for(i=0; i<4; i++)
							packet_route.ext_addr.u8[i]=ptr[i]&0xFF;
						ptr=strtok(NULL, "\n");
						if(ptr!=NULL){
							packet_route.data=ptr;
							packet_route.datalen=strlen(ptr);
							process_post(&init_system_proc, CUSTOMER_EVENT_ROUTE_TO_INTERN, &packet_route);
						}
#if MODE==DEBUG
								else
									printf("No payload. Dropping packet.\n");
#endif
					}
#if MODE==DEBUG
							else
								printf("Source address is not 4 bytes long. Dropping packet.\n");
#endif
				}
#if MODE==DEBUG
						else
							printf("Destination is not 16 Bytes long. Dropping packet. \n");
#endif
			}
#if MODE==DEBUG
			else
				printf("External port is not 2 Bytes long. Dropping packet.\n");
#endif
		}
#endif
	}
	PROCESS_END();
}


void uart_init(void){
	serial_line_init();
#if TARGET==CC1310
	cc26xx_uart_init();
	cc26xx_uart_set_input(serial_line_input_byte);
#elif TARGET==Z1
	uart0_init(0);
	uart0_set_input(serial_line_input_byte);
#endif
}

