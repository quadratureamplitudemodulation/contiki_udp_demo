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
#define MODE 		RELEASE									// Select whether software shall be compiled in DEBUG or RELEASE mode
#define RPLDEVICE	ROOT									// Define the RPL position of the device. Can be either NODE or ROOT
#define TARGET		CC1310										// Define the target. Z1 and CC1310 are supported.

#define DELIMITER "."										// Delimiter for packets to be routed
#define PING 55												// Content of PING
#define BUFFER_PAYLOAD_UDP_TO_EXT 30								// Maximum payload for externally outgoing UDP packets in Bytes



#endif
