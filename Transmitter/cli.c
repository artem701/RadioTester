
#include "allcli.h"

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


// COMMANDS DEFINITION ZONE

static const uint32_t default_delay = 10;
static uint32_t delay = default_delay;

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

void report_test_result(nrf_cli_t const * p_cli, transfer_result_t result)
{
  switch(result.error)
  {
    case TX_WRONG_LEN:
    nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Error accured when generating packet: invalid packet length\r\n");
    break;
  }

  if (result.error != TX_NO_ERROR)
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Information, collected before error:\r\n");


  uint32_t 
    len   = result.packs_len,
    packs = result.packs_sent,
    bytes = packs * len,
    bits  = bytes * 8;

  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Packets sent:    %u\r\n", packs);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Packets lost:    %u (%u%%)\r\n", result.lost_packs, result.lost_packs * 100 / packs);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Damaged packets: %u (%u%%)\r\n", result.damaged_packs, result.damaged_packs * 100 / packs);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Damaged bytes:   %u (%u%%)\r\n", result.damaged_bytes, result.damaged_bytes * 100 / bytes);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Damaged bits:    %u (%u%%)\r\n", result.damaged_bits , result.damaged_bits  * 100 / bits);
}

void sync_channels(nrf_cli_t const * p_cli)
{
  nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Synchronizing channels...\r\n");
  transmitter_set_channel(get_channel());
  report_spi_status(p_cli);
}

#define CMD(NAME) static void NAME (nrf_cli_t const * p_cli, size_t argc, char ** argv)

CMD(cmd_help)
{
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL,
    "Commands:\r\n\
  set channel [auto | <channel> | <start> <end>]\r\n\
    sets radio channel for both transmitter and receiver\r\n\
    can be setted automatically and automatically in the given range\r\n\
  set delay <delay ms>\r\n\
    sets delay between two packets\r\n\
  set power <power option>\r\n\
    sets transmission power\r\n\
    \r\n\
  check transmitter\r\n\
    displays current transmisson settings\r\n\
  check receiver\r\n\
    shows the last status reported from receiver\r\n\
    \r\n\
  loopback <params ...>\r\n\
    transmits a line to receiver, checks response and prints statistics\r\n\
  test [<length> [<num>]]\r\n\
    transmits <num> packs with <length> bytes of payload and prints statistics\r\n"
    );
}

CMD(cmd_set_channel)
{   
  uint8_t channel;

  if (argc < 2 || argc > 2 || strcmp(argv[1], "auto") == 0)
  {
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Setting channel up in auto mode...\r\n");
    
    if (argc > 2)
      channel = best_channel_in_range(atoi(argv[1]), atoi(argv[2]));
    else
      channel = best_channel();
  } 
  else
  {
    channel = atoi(argv[1]);
  }

  transmitter_set_channel(channel);
  nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Transmitter's channel was set to %u\r\n", get_channel());
  report_spi_status(p_cli);
}

CMD(cmd_set_delay)
{
  if (argc < 2)
  {
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Setting delay to the default value...\r\n");
    delay = default_delay;
  }
  else
  {
    delay = atoi(argv[1]);
  }
  nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Delay between packs was set to %u ms\r\n", delay);
}

CMD(cmd_check_transmitter)
{
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current channel:     %u\r\n", get_channel());
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current power level: %s\r\n", power_to_str(get_power()));
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current delay:       %u ms\r\n", delay);
}

CMD(cmd_check_receiver)
{
  report_spi_status(p_cli);
}

CMD(cmd_loopback)
{
  if (argc < 2)
  {
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Nothing to loopback\r\n");
    return;
  }

  // make sure receiver listens the same channel
  sync_channels(p_cli);
  if (spi_status != OK)
    return;

  uint8_t l = 0, arg = 0;
  uint8_t maxlen = IEEE_MAX_PAYLOAD_LEN - 2; // byte for length and byte for '\0'

  // join all args with ' ' and write to radio_tx
  for (arg = 1; arg < argc; ++arg, ++l)
  {
    for (int i = 0; i < strlen(argv[arg]); ++i, ++l)
    {
      if (l >= maxlen)
        break;

      radio_tx[1 + l] = argv[arg][i];
    }

    if (l >= maxlen)
      break;

    if (arg < (argc - 1))
      radio_tx[1 + l] = ' ';
  }

  radio_tx[l] = '\0';
  radio_tx[0] = l;

  // transmit and display results
  transfer_result_t result;
  result = transmitter_transfer_pack(delay);

  // in case upper bound was lost
  loopback[IEEE_MAX_PAYLOAD_LEN - 1] = '\0';

  nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Received loopback: [%u]\"%s\"\r\n", loopback[0], (loopback + 1));
  report_test_result(p_cli, result);
}

CMD(cmd_test)
{
  uint8_t  len;
  uint32_t num;
  const uint32_t default_num = 64;

  // determine the length of every packet
  if (argc < 2 || atoi(argv[1]) > (IEEE_MAX_PAYLOAD_LEN - 1))
  {
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Setting packets length automatically to the max value...\r\n");
    len = IEEE_MAX_PAYLOAD_LEN - 1;
  }
  else
  {
    len = atoi(argv[1]);
    if (len == 0)
    {
      nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Setting packets length automatically to the least value...\r\n");
      len = 1;
    }
  }

  // determine the number of packets
  if (argc < 3)
  {
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Setting packets number automatically to the default value...\r\n");
    num = default_num;
  }
  else
  {
    num = atoi(argv[2]);
    if (num == 0)
    {
      nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Setting packets number automatically to the default value...\r\n");
      num = default_num;
    }
  }

  // make sure receiver listens the same channel
  sync_channels(p_cli);
  if (spi_status != OK)
    return;

  transfer_result_t result;
  result = transmitter_begin_test(len, delay, num);
  report_test_result(p_cli, result);
}

// tx power options
CMD(cmd_0dbm)    { set_power(RADIO_TXPOWER_TXPOWER_0dBm);     }
CMD(cmd_pos2dbm) { set_power(RADIO_TXPOWER_TXPOWER_Pos2dBm);  }
CMD(cmd_pos3dbm) { set_power(RADIO_TXPOWER_TXPOWER_Pos3dBm);  }
CMD(cmd_pos4dbm) { set_power(RADIO_TXPOWER_TXPOWER_Pos4dBm);  }
CMD(cmd_pos5dbm) { set_power(RADIO_TXPOWER_TXPOWER_Pos5dBm);  }
CMD(cmd_pos6dbm) { set_power(RADIO_TXPOWER_TXPOWER_Pos6dBm);  }
CMD(cmd_pos7dbm) { set_power(RADIO_TXPOWER_TXPOWER_Pos7dBm);  }
CMD(cmd_pos8dbm) { set_power(RADIO_TXPOWER_TXPOWER_Pos8dBm);  }
CMD(cmd_neg40dbm){ set_power(RADIO_TXPOWER_TXPOWER_Neg40dBm); }
CMD(cmd_neg30dbm){ set_power(RADIO_TXPOWER_TXPOWER_Neg30dBm); }
CMD(cmd_neg20dbm){ set_power(RADIO_TXPOWER_TXPOWER_Neg20dBm); }
CMD(cmd_neg16dbm){ set_power(RADIO_TXPOWER_TXPOWER_Neg16dBm); }
CMD(cmd_neg12dbm){ set_power(RADIO_TXPOWER_TXPOWER_Neg12dBm); }
CMD(cmd_neg8dbm) { set_power(RADIO_TXPOWER_TXPOWER_Neg8dBm);  }
CMD(cmd_neg4dbm) { set_power(RADIO_TXPOWER_TXPOWER_Neg4dBm);  }

// dead-end for command chain, which actual command was not specified
CMD(unfinished)
{
  nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unfinished command chain\r\n");
}


// REGISTERING COMMANDS

NRF_CLI_CMD_REGISTER(help, NULL, "help", cmd_help);

NRF_CLI_CREATE_STATIC_SUBCMD_SET(sub_power)
{
  NRF_CLI_CMD(0dBm,  NULL,  "set power 0dbm",  cmd_0dbm),
  NRF_CLI_CMD(+2dBm, NULL,  "set power +2dbm", cmd_pos2dbm),
  NRF_CLI_CMD(+3dBm, NULL,  "set power +3dbm", cmd_pos3dbm),
  NRF_CLI_CMD(+4dBm, NULL,  "set power +4dbm", cmd_pos4dbm),
  NRF_CLI_CMD(+5dBm, NULL,  "set power +5dbm", cmd_pos5dbm),
  NRF_CLI_CMD(+6dBm, NULL,  "set power +6dbm", cmd_pos6dbm),
  NRF_CLI_CMD(+7dBm, NULL,  "set power +7dbm", cmd_pos7dbm),
  NRF_CLI_CMD(+8dBm, NULL,  "set power +8dbm", cmd_pos8dbm),
  NRF_CLI_CMD(-40dBm, NULL, "set power -40dbm", cmd_neg40dbm),
  NRF_CLI_CMD(-30dBm, NULL, "set power -30dbm", cmd_neg30dbm),
  NRF_CLI_CMD(-20dBm, NULL, "set power -20dbm", cmd_neg20dbm),
  NRF_CLI_CMD(-16dBm, NULL, "set power -16dbm", cmd_neg16dbm),
  NRF_CLI_CMD(-12dBm, NULL, "set power -12dbm", cmd_neg12dbm),
  NRF_CLI_CMD(-8dBm,  NULL, "set power -8dbm",  cmd_neg8dbm),
  NRF_CLI_CMD(-4dBm,  NULL, "set power -4dbm",  cmd_neg40dbm),
  NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CREATE_STATIC_SUBCMD_SET(sub_set)
{
  NRF_CLI_CMD(channel, NULL, "set channel [auto | <channel> | <start> <end>]", cmd_set_channel),
  NRF_CLI_CMD(delay, NULL, "set delay <delay_ms>", cmd_set_delay),
  NRF_CLI_CMD(power, &sub_power, "set power <power option>", unfinished),
  NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(set, &sub_set, "set (channel | power | delay)", unfinished);

NRF_CLI_CREATE_STATIC_SUBCMD_SET(sub_check)
{
  NRF_CLI_CMD(transmitter, NULL, "check transmitter", cmd_check_transmitter),
  NRF_CLI_CMD(receiver, NULL, "check receiver", cmd_check_receiver),
  NRF_CLI_SUBCMD_SET_END
};
NRF_CLI_CMD_REGISTER(check, &sub_check, "check (transmitter | receiver)", unfinished);

NRF_CLI_CMD_REGISTER(loopback, NULL, "loopback <string>", cmd_loopback);

NRF_CLI_CMD_REGISTER(test, NULL, "test [<packets length>]", cmd_test);
