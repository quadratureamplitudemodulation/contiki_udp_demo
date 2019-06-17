#ifndef configuration
#define configuration

/*
 * ***************************************************************************************************************
 * Please note: The following table only represents usable values. Configurations must be made below this section!
 */
#define DEBUG 	0
#define RELEASE 1
#define ROOT 	0
#define NODE 	1
#define Z1		0
#define CC1310	1
/*
 * ***************************************************************************************************************
 */






/*
 * Configure the project at compile time with these defines
 */
#define MODE 		DEBUG									// Select whether software shall be compiled in DEBUG or RELEASE mode
#define RPLDEVICE	ROOT									// Define the RPL position of the device. Can be either NODE or ROOT
#define TARGET		Z1										// Define the target. Z1 and CC1310 are supported.

#define DELIMITER "."										// Delimiter for packets to be routed
#define PING 55												// Content of PING
#define MAX_PAYLOAD_EXTERN 20								// Maximum Size of payload sent to an extern client in Bytes


#endif
