#include "stm32f7xx_hal.h"
#include "uart_syscalls.h"
#include <string.h>


UART_HandleTypeDef *_handle_uart;

void _register_uart(UART_HandleTypeDef *uart) {
	_handle_uart = uart;
}


int _read(int file, char *ptr, int len) {
	memset(ptr, 0, len);
	for (;;) {
		if (HAL_UART_Receive(_handle_uart, (unsigned char*) ptr, len, 100) == HAL_OK)
			return strlen(ptr);
		if (strchr(ptr, '\n') != NULL || strchr(ptr, '\r'))
			return strlen(ptr);
		if (strlen(ptr))
			return strlen(ptr);
	}
	return -1;
}

int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(_handle_uart, (unsigned char *)ptr,
			len, 1000);
	return len;
}
