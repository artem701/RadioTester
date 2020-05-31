/*
 * Logic for transmitter subproject
 *
 * * */

#include <stdint.h>
#include <stdbool.h>

#include "allradio.h"

/*
// buffer for packet to be sent
extern uint8_t radio_tx[IEEE_MAX_PAYLOAD_LEN];
// buffer to read from spi
extern uint8_t spi_rx[1 + IEEE_MAX_PAYLOAD_LEN];
// pointer to the packet beginning in the spi_rx
extern uint8_t * const loopback;
*/

// interpretation of receiver's answers via SPI
typedef enum {
  SPI_SUCCESS,
  SPI_PROBES_OUT,
  SPI_DISCONNECTED,
  SPI_UNKNOWN
} spi_status_t;
//extern spi_status_t spi_status;
extern spi_status_t spi_status;

// summary of a test
typedef struct {
  uint32_t packs_sent;
  uint32_t damaged_bits ;
  uint32_t damaged_bytes;
  uint32_t damaged_packs;
  uint32_t lost_packs;
  uint8_t  packs_len;
  enum {
    TX_NO_ERROR,
    TX_WRONG_LEN
  } error;
} transfer_result_t;

typedef enum
{
  TX_PATTERN_11111111 = 0xFF,
  TX_PATTERN_11110000 = 0xF0,
  TX_PATTERN_11001100 = 0xCC,
  TX_PATTERN_10101010 = 0xAA,
  TX_PATTERN_RANDOM
} pattern_t;

extern uint8_t   radio_len;
extern pattern_t radio_pattern;
extern uint8_t   radio_delay;

// Changes channel of radio, initiates the same for receiver, checks confirmation
void transmitter_set_channel(uint8_t channel);

void transmitter_set_pack_len(uint8_t    len    );
void transmitter_set_pattern (pattern_t  pattern);
void transmitter_set_delay   (uint32_t   delay  );

// Tests given channel with given power
// uses current values
transfer_result_t transmitter_test_channel_power(/*uint8_t channel, uint8_t power*/);

// Tests given channel, gradually decreasing power from max value to min
transfer_result_t transmitter_test_channel(uint8_t channel);

// Tests all availble channels
transfer_result_t transmitter_test_all(uint8_t channel);

/*
// Transfers a single packet to receiver, returns status of the transmission (see spi_protocol.h)
// delay is time after which pack is considered to be lost
// radio_tx must be filled already
transfer_result_t transmitter_transfer_pack(uint32_t delay);

// Starts testing session with output to cli
transfer_result_t transmitter_begin_test(uint8_t packs_len, uint32_t delay, uint32_t packs_number);
*/
