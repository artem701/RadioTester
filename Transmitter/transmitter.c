
#include "transmitter.h"
#include "spi_protocol.h"

#include "nrf_delay.h"

#include "allradio.h"
#include "allspi.h"
#include "random.h"
#include "hash.h"

#include <stdlib.h>

#define MAX_PROBES  16
// delay in us
#define PROBE_DELAY 100

//                                                            *** DATA DEFINITION ***

// buffer for packet to be sent over radio
uint8_t radio_tx[IEEE_MAX_PAYLOAD_LEN];

// buffer to write to SPI
// header + hash
uint8_t spi_tx[sizeof(tx_spi_header_t) + 1];

// buffer to read from SPI
// header + payload + hash
uint8_t spi_rx[sizeof(rx_spis_header_t) + IEEE_MAX_PAYLOAD_LEN + 1];

// pointer to the packet beginning in the spi_rx
uint8_t * const loopback = (spi_rx + sizeof(rx_spis_header_t));

// headers, received and sent
#define RX_HEADER ((rx_spis_header_t*)spi_rx)
#define TX_HEADER ((tx_spi_header_t*)spi_tx)

// length of radio loopback in spi_rx
// length byte + data
#define LOOPBACK_LEN (1 + *loopback)
// overall spi_rx length
// header + payload + hash
#define RX_LEN (sizeof(rx_spis_header_t) + LOOPBACK_LEN + 1)

// number of the pack to be sent next (increments cyclically from 255 to 0)
static uint8_t pack_num = 0;

// test settings
static uint8_t   radio_len = 10;
static pattern_t radio_pattern = TX_PATTERN_11111111;
static uint8_t   radio_delay = 10;

// state of receiver to be monitored by client
spi_status_t spi_status = SPI_UNKNOWN;

//                                                            *** FUNC DEFINITION ***

// wait for default spi delay
static inline void delay_spi()
{
  nrf_delay_us(PROBE_DELAY);
}

// wait for currently set radio delay
static inline void delay_radio()
{
  nrf_delay_ms(radio_delay);
}

// form a package to be sent via SPI
static inline void prepare_message(uint8_t cmd)
{
  TX_HEADER->pack_num = pack_num;
  TX_HEADER->cmd      = cmd;

  hashify(spi_tx, sizeof(tx_spi_header_t));
}

// initiate SPI transfer
static inline void push_message(void)
{
  allspi_transfer(spi_tx, sizeof(spi_tx), spi_rx, sizeof(spi_rx));
}

static uint8_t spi_cmd;
static uint8_t spi_probes_left;

// pre-declaration
static void cmd_deliver_check(void);

// probe phase 1
static void cmd_deliver_probe(void)
{
  //cmd_deliver_params_t* params = (cmd_deliver_params_t*)params;

  if (spi_probes_left == 0)
  {
    // interrupt probes session
    spi_status = SPI_PROBES_OUT;
    return;
  }
  
  // if we don't increment the pack number each probe, that allows us
  // to recognize messages with same sense as equal
  //++pack_num; // ?

  prepare_message(spi_cmd);
  push_message();
  // command message sent
  
  // give time to perform the command and check the result
  delay_spi();
  cmd_deliver_check();
}

// probe phase 2
static void cmd_deliver_check(void)
{
  if (spi_probes_left == 0)
  {
    spi_status = SPI_PROBES_OUT;
    return;
  }
  
  prepare_message(TX_GET_STATUS);
  push_message();

  spi_probes_left -= 1;

  if (!check_hash(spi_rx, RX_LEN))
  {
    // schedule a recheck
    delay_spi();
    cmd_deliver_check();
    return;
  }

  if (RX_HEADER->pack_num != pack_num)
  {
    // got answer to wrong packet, resend the packet
    delay_spi();
    cmd_deliver_probe();
    return;
  }

  switch(RX_HEADER->status)
  {
    case RX_SUCCESS:
      // successfully finish probes session
      spi_status = SPI_SUCCESS;
      break;
    case RX_NO_RESPONSE:
      // receiver is off, check again
      spi_status = SPI_DISCONNECTED;
      delay_spi();
      cmd_deliver_check();
      break;
    case RX_NOT_READY:
      // receiver is in progress, wait and check again
      delay_spi();
      cmd_deliver_check();
      break;
    default:
      // loss or fail
      // send packet again
      delay_spi();
      cmd_deliver_probe();
      break;
  }
}

// safe deliverance of a command over SPI
// guarantees proper enough detection of mistakes
static inline void cmd_deliver(uint8_t cmd)
{
  pack_num += 1;
  spi_status      = SPI_UNKNOWN;
  spi_probes_left = MAX_PROBES;
  spi_cmd         = cmd;
  delay_spi();
  cmd_deliver_probe();
}

// Changes channel of radio, initiates the same for receiver, waits for confirmation
void transmitter_set_channel(uint8_t channel)
{
  set_channel(channel);
  channel = get_channel();

  // request channel switch
  cmd_deliver(channel);
}

void transmitter_set_pack_len(uint8_t len)
{
  radio_len = MIN(len, IEEE_MAX_PAYLOAD_LEN);
}

void transmitter_set_pattern(pattern_t pattern)
{
  radio_pattern = pattern;
}

void transmitter_set_delay(uint32_t delay)
{
  radio_delay = delay;
}

// fill payload with the given pattern
static inline void generate_pack()
{
  radio_tx[0] = radio_len - 1;

  if (radio_pattern == TX_PATTERN_RANDOM)
  {
    uint8_t pattern = rnd8();
    for (int i = radio_len - 1; i > 0; --i)
      radio_tx[i] = pattern;
  }
  else
  {
    for (int i = radio_len - 1; i > 0; --i)
      radio_tx[i] = radio_pattern;
  }
}

// initial state of a resulting structure
#define TX_EMPTY_RESULT \
  (transfer_result_t){  \
  .packs_sent    = 0,   \
  .damaged_bits  = 0,   \
  .damaged_bytes = 0,   \
  .damaged_packs = 0,   \
  .lost_packs    = 0,   \
  .pattern       = 0}

// number of unequal bits
static uint8_t diff_bits(uint8_t a, uint8_t b)
{
  uint8_t result = 0;
  for (; a != 0 || b != 0; a >>= 1, b >>= 1)
    if ((a & 1) != (b & 1))
      ++result;
    
  return result;
}

// count mistakes, pack already was sent and received
static transfer_result_t check_rx()
{
  transfer_result_t result = TX_EMPTY_RESULT;
  result.packs_sent = 1;

  // if empty packet received via SPI
  if (loopback[0] == 0)
  {
    result.lost_packs = 1;
    return result;
  }

  // count all lost bytes, caused by damage of the first byte
  uint8_t len_dif = abs(radio_tx[0] - loopback[0]);
  result.damaged_bytes = len_dif;
  result.damaged_bits  = len_dif * 8;

  result.damaged_bytes += (len_dif != 0) ? 1 : 0;
  result.damaged_bits  += diff_bits(radio_tx[0], loopback[0]);

  // count mistakes in received pattern
  uint8_t len = MIN(radio_tx[0], loopback[0]);
  for (int i = 1; i <= len; ++i)
  {
    uint8_t bits = diff_bits(radio_tx[i], loopback[i]);
    if (bits)
    {
      result.damaged_bytes += 1;
      result.damaged_bits  += bits;
    }
  }

  if (result.damaged_bits != 0)
    result.damaged_packs = 1;
  
  return result;
}

// sums two statistics, writing to accumulator
static void tx_result_sum(transfer_result_t* accumulator, const transfer_result_t* term)
{
  accumulator->packs_sent    += term->packs_sent;
  accumulator->damaged_bits  += term->damaged_bits;
  accumulator->damaged_bytes += term->damaged_bytes;
  accumulator->damaged_packs += term->damaged_packs;
  accumulator->lost_packs    += term->lost_packs;
}

// tests series of packs, collecting statistics
// does no preparations
static transfer_result_t test_series_raw()
{
  transfer_result_t result = TX_EMPTY_RESULT;
  result.pattern = radio_tx[1];
  for (int i = 0; i < RX_MAX_PACK_BUFFER; ++i)
  {
    send_data(radio_tx, true);
    nrf_delay_ms(radio_delay);
  }

  for (int i = 0; i < RX_MAX_PACK_BUFFER; ++i)
  {
    cmd_deliver(TX_REQUEST_PACK(i));
    transfer_result_t single_result = check_rx();
    tx_result_sum(&result, &single_result);
  }

  return result;
}

// tests a series of packs on current channel and power
transfer_result_t transmitter_test()
{
  generate_pack();
  return test_series_raw();
}

// available values for power
// from max to min
static const uint8_t powers[] = 
  {
    RADIO_TXPOWER_TXPOWER_Pos8dBm,
    RADIO_TXPOWER_TXPOWER_Pos7dBm,
    RADIO_TXPOWER_TXPOWER_Pos6dBm,
    RADIO_TXPOWER_TXPOWER_Pos5dBm,
    RADIO_TXPOWER_TXPOWER_Pos4dBm,
    RADIO_TXPOWER_TXPOWER_Pos3dBm,
    RADIO_TXPOWER_TXPOWER_Pos2dBm,
    RADIO_TXPOWER_TXPOWER_0dBm,
    RADIO_TXPOWER_TXPOWER_Neg4dBm,
    RADIO_TXPOWER_TXPOWER_Neg8dBm,
    RADIO_TXPOWER_TXPOWER_Neg12dBm,
    RADIO_TXPOWER_TXPOWER_Neg16dBm,
    RADIO_TXPOWER_TXPOWER_Neg20dBm,
    RADIO_TXPOWER_TXPOWER_Neg30dBm,
    RADIO_TXPOWER_TXPOWER_Neg40dBm
  };

channel_info_t transmitter_test_channel()
{
  // remember current value of power
  uint8_t power = get_power();

  channel_info_t result;
  result.channel = get_channel();
  result.noise = check_power(get_channel());
  result.flags = CHANNEL_OK;

  generate_pack();
  for (int i = 0; i < sizeof(powers); ++i)
  {
    set_power(powers[i]);

    transfer_result_t transfer_result;
    transfer_result = test_series_raw();
    // if there is damage, and no `mistake` flag yet
    if (transfer_result.damaged_bits != 0 && !(result.flags & CHANNEL_MISTAKE_FLAG))
    {
      result.flags |= CHANNEL_MISTAKE_FLAG;
      result.mistake_power = powers[i];
    }
    // if there is packet loss, and no `loss` flag yet
    if (transfer_result.lost_packs != 0 && !(result.flags & CHANNEL_LOSS_FLAG))
    {
      result.flags |= CHANNEL_LOSS_FLAG;
      result.loss_power = powers[i];
    }
    // if both flags are active, no sense in testing more
    if ((result.flags & CHANNEL_LOSS_FLAG) && (result.flags & CHANNEL_MISTAKE_FLAG))
      break;
  }

  set_power(power);
  return result;
}

char* pattern_to_str(pattern_t pattern)
{
  switch(pattern)
  {
    case TX_PATTERN_11111111:
      return "11111111";
    case TX_PATTERN_11110000:
      return "11110000";
    case TX_PATTERN_11001100:
      return "11001100";
    case TX_PATTERN_10101010:
      return "10101010";
    case TX_PATTERN_RANDOM:
      return "random";
  }
}