#ifndef _UART_SYSCALLS_
#define _UART_SYSCALLS_

void _register_uart(UART_HandleTypeDef *uart);
int _read(int file, char *ptr, int len);
int _write(int file, char *ptr, int len);


#endif
