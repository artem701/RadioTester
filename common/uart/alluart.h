/*
 * UART driver wrapper
 *
 * * */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_delay.h"
#include "boards.h"
  
#include "app_uart.h"
#include "nrf.h"
#include "nrf_uart.h"
#include "nrf_uarte.h"


#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256                         /**< UART RX buffer size. */

#define UART_HWFC APP_UART_FLOW_CONTROL_DISABLED

void alluart_init();