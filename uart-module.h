#include "configuration.h"
/*
 * @file uart-module.h
 * @brief Contatins API for UART module
 *
 * The UART module automatically interprets UART commands and polls the init_system_process with the according event and data. It makes use of the serial-line.h library in order to receive full commands.
 * If the device is a node, this library stores a copy of the root's service ID. It is important to keept that up to date at all times using change_root_id.
 */
#ifndef UARTMODULE
#define UARTMODULE UARTMODULE

/*
 * @brief Process which interprets the command
 *
 * This process gets polled by serial-line.h when a whole command has been sent. When using this library, please manually start this process.
 */
PROCESS_NAME(uart_int_handler);

/*
 * @brief Initialize the UART module
 *
 * Takes all necessary steps to initialize the UART module and the serial-line.h library
 */
void uart_init(void);

#if RPLDEVICE==NODE
/*
 * @brief Change the service ID of the root as stored in this library
 * @param id Service ID to be stored.
 */
void change_root_id(uint8_t id);
#endif

#endif
