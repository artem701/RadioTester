
#include "allcli.h"
#include "alluart.h"
/*
#include "allradio.h"
#include "transmitter.h"*/

#include "nrf.h"
#include "app_uart.h"
#include "app_error.h"
#include "nordic_common.h"
#include "nrf_cli.h"
#include "nrf_cli_uart.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

NRF_CLI_UART_DEF(m_cli_uart_transport, 0, 64, 16);
NRF_CLI_DEF(m_cli_uart,
  "transmitter:~$ ",
  &m_cli_uart_transport.transport,
  '\r',
  CLI_EXAMPLE_LOG_QUEUE_SIZE);

void allcli_init()
{
  ret_code_t ret;

  nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;

  uart_config.pseltxd = TX_PIN_NUMBER;
  uart_config.pselrxd = RX_PIN_NUMBER;
  uart_config.hwfc    = NRF_UART_HWFC_DISABLED;
  ret                 = nrf_cli_init(&m_cli_uart, &uart_config, true, true, NRF_LOG_SEVERITY_INFO);
  APP_ERROR_CHECK(ret);
}

void allcli_start()
{
  ret_code_t ret;

  ret = nrf_cli_start(&m_cli_uart);
  APP_ERROR_CHECK(ret);
}

void allcli_process()
{
  nrf_cli_process(&m_cli_uart);
}

