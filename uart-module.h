#ifndef UARTMODULE
#define UARTMODULE UARTMODULE
PROCESS_NAME(uart_int_handler);

void uart_init(void);
void change_root_id(uint8_t id);
#endif
