
#include "allcli.h"
#include "alluart.h"

#include "allradio.h"
#include "transmitter.h"

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
            "transmitter-cli:~$ ",
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


// COMMANDS DEFINITION ZONE

void report_spi_status(nrf_cli_t const * p_cli)
{
    switch (spi_status)
    {
	case OK:
	    nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "Receiver reported success\r\n");
	    break;
	case TIMEOUT:
	    nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Connection to receiver timeout. Check problems with receiver or set MAX_PROBES to a bigger value\r\n");
	    break;
	case DISCONNECTED:
	    nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "No connection with receiver. Check integrity of SPI connection and receiver's power\r\n");
	    break;
	case UNKNOWN:
	    nrf_cli_fprintf(p_cli, NRF_CLI_WARNING, "Receiver's status is unchecked\r\n");
	    break;
    }
}

#define CMD(NAME) static void NAME (nrf_cli_t const * p_cli, size_t argc, char ** argv)

CMD(cmd_help)
{
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL,
    "Commands:\r\n\
     set channel [auto | <channel> | <start> <end>]\r\n\
         sets radio channel for both transmitter and receiver. Can be setted automatically and automatically in the given range\r\n\
     check transmitter\r\n\
         displays current transmisson settings\r\n\
     check receiver\r\n\
         shows the last status reported from receiver\r\n"
    );
}

CMD(cmd_set_channel)
{
    
    uint8_t channel;

    if (argc < 2 || argc > 2 || strcmp(argv[1], "auto") == 0)
    {
	nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Setting channel up in auto mode...\r\n");
        
	if (argc > 1)
	    channel = best_channel_in_range(atoi(argv[1]), atoi(argv[2]));
	else
	    channel = best_channel();
    } 
    else
    {
	channel = atoi(argv[1]);
    }

    transmitter_set_channel(channel);
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Transmitter's channel was set to %d\r\n", (int)get_channel());
    report_spi_status(p_cli);
}

CMD(cmd_check_transmitter)
{
    nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current channel:     %d\r\n", (int)get_channel());
    nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current power level: %s\r\n", power_to_str(get_power()));
}

CMD(cmd_check_receiver)
{
    report_spi_status(p_cli);
}

CMD(unfinished)
{
    nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unfinished command chain\r\n");
}


// REGISTERING COMMANDS

NRF_CLI_CMD_REGISTER(help,
                     NULL,
                     "help",
                     cmd_help);

NRF_CLI_CREATE_STATIC_SUBCMD_SET(sub_set)
{
    NRF_CLI_CMD(channel, NULL, "set channel [auto | <channel> | <start> <end>]", cmd_set_channel),
    //NRF_CLI_CMD(power, sub_power, "set power <power option>", unfinished),
    NRF_CLI_SUBCMD_SET_END
};
NRF_CLI_CMD_REGISTER(set,
                     &sub_set,
                     "set (channel | power)",
                     unfinished);

NRF_CLI_CREATE_STATIC_SUBCMD_SET(sub_check)
{
    NRF_CLI_CMD(transmitter, NULL, "check transmitter", cmd_check_transmitter),
    NRF_CLI_CMD(receiver, NULL, "check receiver", cmd_check_receiver),
    NRF_CLI_SUBCMD_SET_END
};
NRF_CLI_CMD_REGISTER(check,
                     &sub_check,
                     "check (transmitter | receiver)",
                     unfinished);