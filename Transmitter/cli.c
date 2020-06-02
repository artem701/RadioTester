
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


#define DEFAULT_DELAY   5
#define DEFAULT_LEN     16
#define DEFAULT_PATTERN TX_PATTERN_11111111
#define DEFAULT_POWER   RADIO_TXPOWER_TXPOWER_0dBm

// test settings
static uint8_t   radio_len     = DEFAULT_LEN;
static pattern_t radio_pattern = DEFAULT_PATTERN;
static uint8_t   radio_delay   = DEFAULT_DELAY;
static uint8_t   radio_power   = DEFAULT_POWER;

// provide all current settings to transmitter
static inline void config_transmitter()
{
  transmitter_set_pack_len(radio_len);
  transmitter_set_pattern (radio_pattern);
  transmitter_set_delay   (radio_delay);
}

// interpret spi_status
void report_spi_status(nrf_cli_t const * p_cli)
{
  switch (spi_status)
  {
    case SPI_SUCCESS:
      nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "Receiver reported success\r\n");
      break;
    case SPI_PROBES_OUT:
      nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Probes out. Check problems with receiver or set MAX_PROBES to a bigger value\r\n");
      break;
    case SPI_DISCONNECTED:
      nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "No connection with receiver. Check integrity of SPI connection and receiver's power\r\n");
      break;
    case SPI_UNKNOWN:
      nrf_cli_fprintf(p_cli, NRF_CLI_WARNING, "Receiver's status is unchecked\r\n");
      break;
  }
}

// casts one byte to string as a binary value
// destination must have size not less than 9
static void byte_to_str(uint8_t byte, char* destination)
{
  destination[8] = 0;
  for (int i = 7; i >= 0; --i)
  {
    destination[i] = (byte % 2) ? '1' : '0';
    byte /= 2;
  }
}

// interpret transfer_result_t
void report_test_result(nrf_cli_t const * p_cli, transfer_result_t result)
{
  uint32_t 
    packs = result.packs_sent,
    bytes = packs * radio_len,
    bits  = bytes * 8;

  char pattern[9];
  byte_to_str(result.pattern, pattern);

  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Packets sent:    %u\r\n", packs);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Packets pattern: %s\r\n", pattern);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Packets lost:    %u (%u%%)\r\n", result.lost_packs, result.lost_packs * 100 / packs);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Damaged packets: %u (%u%%)\r\n", result.damaged_packs, result.damaged_packs * 100 / packs);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Damaged bytes:   %u (%u%%)\r\n", result.damaged_bytes, result.damaged_bytes * 100 / bytes);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Damaged bits:    %u (%u%%)\r\n", result.damaged_bits , result.damaged_bits  * 100 / bits);
}

// interpret channel_info_t
void report_channel_info(nrf_cli_t const * p_cli, channel_info_t info)
{
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Channel: %u\r\n", info.channel);
  nrf_cli_fprintf(p_cli, info.noise == 0 ? NRF_CLI_INFO : NRF_CLI_WARNING, 
    "Noise on the channel: %u\r\n", info.noise);

  if (info.flag == CHANNEL_OK)
  {
    nrf_cli_fprintf(p_cli, NRF_CLI_INFO, "No mistakes on the channel\r\n", info.channel);
  }
  else
  {
    nrf_cli_fprintf(p_cli, NRF_CLI_WARNING, "Losses start on power %s and below. Failed test:\r\n", power_to_str(info.loss_power));
    report_test_result(p_cli, info.failed_transfer);
  }
}

// sync channels with Receiver
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
  set delay [delay ms]\r\n\
    sets delay between two packets\r\n\
  set power <power option>\r\n\
    sets transmission power\r\n\
  set length [length option]\r\n\
    sets length of a radio packet\r\n\
  set pattern <length option>\r\n\
    sets the transmission patter for generating packets\r\n\
    \r\n\
  info\r\n\
    displays current transmisson settings\r\n\
    \r\n\
  test\r\n\
    launches a series of tests on the current channel and power,\r\n\
    reporting statistics\r\n\
  test channel\r\n\
    tests current channel's characteristics\r\n\
  test all\r\n\
    tests all possible channels, picks the best one\r\n\
    \r\n"
    );
}

// commands to manipulate with settings

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
    radio_delay = DEFAULT_DELAY;
  }
  else
  {
    radio_delay = atoi(argv[1]);
  }
  nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Delay between packs was set to %u ms\r\n", radio_delay);
}

CMD(cmd_set_length)
{
  if (argc < 2)
  {
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Setting length to the default value...\r\n");
    radio_len = DEFAULT_LEN;
  }
  else
  {
    radio_len = MIN(MAX(atoi(argv[1]), 2), IEEE_MAX_PAYLOAD_LEN);
  }

  nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Length of each packet was set to %u\r\n", radio_len);
}

// prints current settings
CMD(cmd_info)
{
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current channel:     %u\r\n", get_channel());
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current power level: %s\r\n", power_to_str(get_power()));
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current delay:       %u ms\r\n", radio_delay);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current pattern:     %s\r\n", pattern_to_str(radio_pattern));
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Current pack length: %u bytes\r\n", radio_len);
}

// performs a series of tests on current channel with current power
CMD(cmd_test)
{
  nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, 
    "Testing on channel %u with power %s\r\n", get_channel(), power_to_str(get_power()));
  config_transmitter();
  sync_channels(p_cli);
  transfer_result_t result = transmitter_test();
  report_test_result(p_cli, result);
}

// for current channel, finds the minimum safe power for transmission
CMD(cmd_test_channel)
{
  config_transmitter();
  sync_channels(p_cli);
  channel_info_t channel_info;
  channel_info = transmitter_test_channel();
  report_channel_info(p_cli, channel_info);
}

// does channel test for every channel, finds the best one
CMD(cmd_test_all)
{
  uint8_t old_channel = get_channel();
  config_transmitter();
  sync_channels(p_cli);

  uint8_t best_channel = IEEE_MIN_CHANNEL;
  channel_info_t best_info;

  // test first available channel to init best values
  transmitter_set_channel(best_channel);
  best_info = transmitter_test_channel();
  report_channel_info(p_cli, best_info);
  nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\r\n");

  for (uint8_t ch = (IEEE_MIN_CHANNEL + 1); ch <= IEEE_MAX_CHANNEL; ++ch)
  {
    channel_info_t info;
    transmitter_set_channel(ch);
    info = transmitter_test_channel();
    report_channel_info(p_cli, info);
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\r\n");

    // cases, when we have to change the best channel
    // 1) mistakes on old channel and no mistakes on current
    // 2) mistakes appear on less power, than on the best channel
    // 3) mistake power is equal, but current channel has less noise
    if (
      (best_info.flag != CHANNEL_OK && info.flag == CHANNEL_OK) ||
      (best_info.flag == info.flag) && (
        (info.loss_power < best_info.loss_power)   ||
        (info.loss_power == best_info.loss_power && info.noise < best_info.noise)
      ) 
    )
    {
      best_channel = ch;
      best_info = info;
    }
  }
  
  transmitter_set_channel(old_channel);
  nrf_cli_fprintf(p_cli, NRF_CLI_OPTION, "Optimal channel: %u\r\n\r\n", best_channel);
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

// transmission pattern options
CMD(cmd_pattern_11111111) { radio_pattern = TX_PATTERN_11111111; }
CMD(cmd_pattern_11110000) { radio_pattern = TX_PATTERN_11110000; }
CMD(cmd_pattern_11001100) { radio_pattern = TX_PATTERN_11001100; }
CMD(cmd_pattern_10101010) { radio_pattern = TX_PATTERN_10101010; }
CMD(cmd_pattern_random  ) { radio_pattern = TX_PATTERN_RANDOM  ; }

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

NRF_CLI_CREATE_STATIC_SUBCMD_SET(sub_pattern)
{
  NRF_CLI_CMD(11111111,  NULL,  "set pattern 11111111",  cmd_pattern_11111111),
  NRF_CLI_CMD(11110000,  NULL,  "set pattern 11110000",  cmd_pattern_11110000),
  NRF_CLI_CMD(11001100,  NULL,  "set pattern 11001100",  cmd_pattern_11001100),
  NRF_CLI_CMD(10101010,  NULL,  "set pattern 10101010",  cmd_pattern_10101010),
  NRF_CLI_CMD(random,    NULL,  "set pattern random",    cmd_pattern_random  ),
  NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CREATE_STATIC_SUBCMD_SET(sub_set)
{
  NRF_CLI_CMD(channel, NULL, "set channel [auto | <channel> | <start> <end>]", cmd_set_channel),
  NRF_CLI_CMD(delay, NULL, "set delay [delay ms]", cmd_set_delay),
  NRF_CLI_CMD(power, &sub_power, "set power <power option>", unfinished),
  NRF_CLI_CMD(length, NULL, "set length [bytes count]", cmd_set_length),
  NRF_CLI_CMD(pattern, &sub_pattern, "set pattern <pattern option>", unfinished),
  NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(set, &sub_set, "set (channel | power | delay | length | pattern)", unfinished);

NRF_CLI_CMD_REGISTER(info, NULL, "info", cmd_info);

NRF_CLI_CREATE_STATIC_SUBCMD_SET(sub_test)
{
  NRF_CLI_CMD(channel, NULL, "test channel", cmd_test_channel),
  NRF_CLI_CMD(all, NULL, "test all", cmd_test_all),
  NRF_CLI_SUBCMD_SET_END
};

NRF_CLI_CMD_REGISTER(test, &sub_test, "test [channel]", cmd_test);