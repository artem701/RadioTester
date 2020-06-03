/*
 * Logic for transmitter subproject
 *
 * * */

#include <stdint.h>
#include <stdbool.h>

#include "allradio.h"


// interpretation of receiver's answers via SPI
typedef enum {
  SPI_SUCCESS,
  SPI_PROBES_OUT,
  SPI_DISCONNECTED,
  SPI_UNKNOWN
} spi_status_t;

// status of receiver to be monitored by client
extern spi_status_t spi_status;

// summary of a test
typedef struct {
  uint32_t packs_sent;
  uint32_t damaged_bits ;
  uint32_t damaged_bytes;
  uint32_t damaged_packs;
  uint32_t lost_packs;
  uint8_t  pattern;
  /*
  enum {
    TX_NO_ERROR,
    TX_SYNC_ERROR
  } error;*/
} transfer_result_t;

typedef enum
{
  TX_PATTERN_11111111 = 0xFF,
  TX_PATTERN_11110000 = 0xF0,
  TX_PATTERN_11001100 = 0xCC,
  TX_PATTERN_10101010 = 0xAA,
  TX_PATTERN_RANDOM
} pattern_t;

char* pattern_to_str(pattern_t pattern);

// Changes channel of radio, initiates the same for receiver, checks confirmation
void transmitter_set_channel(uint8_t channel);

// sets the length of a packet, including byte of payload len
void transmitter_set_pack_len(uint8_t    len    );
void transmitter_set_pattern (pattern_t  pattern);
void transmitter_set_delay   (uint32_t   delay  );

// Launches RX_MAX_PACK_BUFFER tests on the current channel and power,
// returns statistics
transfer_result_t transmitter_test();

// flags for the channel test result
#define CHANNEL_OK 0
#define CHANNEL_LOSS 1
typedef struct {
  transfer_result_t failed_transfer;
  uint8_t channel;
  uint8_t noise;
  uint8_t flag;
  // power, where losses start
  uint8_t loss_power;
} channel_info_t;

// tests current channel
channel_info_t transmitter_test_channel();