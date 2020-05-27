
#include "transmitter.h"
#include "spi_protocol.h"

#include "nrf_delay.h"

#include "allradio.h"
#include "allspi.h"
#include "random.h"

#define MAX_PROBES  16
#define PROBE_DELAY 1

//                                                            *** DATA DEFINITION ***

// buffer for packet to be sent
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
#define LOOPBACK_LEN (*loopback)
// overall spi_rx length
#define RX_LEN (sizeof(rx_spis_header_t) + LOOPBACK_LEN)

// number of the pack to be sent next (increments cyclically from 255 to 0)
static uint8_t pack_num = 0;

// test settings
uint8_t   radio_len;
pattern_t radio_pattern;
uint8_t   radio_delay;

// state of receiver to be monitored by client
spi_status_t spi_status = SPI_UNKNOWN;

//                                                            *** FUNC DEFINITION ***


// form a package to be sent via SPI
static inline void prepare_message(uint8_t cmd)
{
  TX_HEADER->pack_num = pack_num;
  TX_HEADER->cmd      = cmd;

  hashify(spi_tx, sizeof(tx_spi_header_t));
}

// initiate SPI transfer
static void push_message(void)
{
  allspi_transfer(spi_tx, sizeof(spi_tx), spi_rx, sizeof(spi_rx));
}

static uint8_t spi_cmd;
static uint8_t spi_probes_left;

// wait until timer is ready to be used
// actually won't enter the cycle, because only usage of a timer
// is delaying SPI messages.
// anyway guarantees safety
static inline void wait_for_timer()
{
  while (timer_is_busy())
    __WFE();
}

// macro for scheduling probe
#define SPI_DELAY(callback) \
  wait_for_timer();         \
  start_timer(PROBE_DELAY, SPI_MSG_PRIORITY, callback, NULL)

// probe phase 1
static void cmd_deliver_probe(void* params)
{
  //cmd_deliver_params_t* params = (cmd_deliver_params_t*)params;

  if (spi_probes_left == 0)
    // interrupt probes session
    spi_status = SPI_PROBES_OUT;
    return;

  ++pack_num;

  prepare_message(spi_cmd);
  push_message();
  // command message sent
  
  // give time to perform the command and check the result
  SPI_DELAY(cmd_deliver_check);
}

// probe phase 2
static void cmd_deliver_check(void* params)
{
  //cmd_deliver_params_t* params = (cmd_deliver_params_t*)params;
  prepare_message(TX_GET_STATUS);
  push_message();

  probes_left -= 1;

  // check if the response is not damaged
  if (!check_hash(spi_rx, RX_LEN))
  {
    // schedule a recheck
    SPI_DELAY(cmd_deliver_check);
    return;
  }

  if (RX_HEADER->pack_num != pack_num)
  {
    // got answer to wrong packet, resend the packet
    SPI_DELAY(cmd_deliver_probe);
    return;
  }

  switch(RX_HEADER->status)
  {
    case RX_SUCCESS:
      // successfully finish probes session
      spi_status = SPI_SUCCESS;
      break;
    case RX_NO_RESPONSE:
      // receiver is off, finish session
      spi_status = SPI_DISCONNECTED;
      break;
    case RX_NOT_READY:
      // receiver is in progress, wait and check again
      SPI_DELAY(cmd_deliver_check);
      break;
    default:
      // loss or fail
      // send packet again
      SPI_DELAY(cmd_deliver_probe);
      break;
  }
}

// safe deliverance of a command over SPI
// guarantees proper enough detection of mistakes
static void cmd_deliver(uint8_t cmd)
{
  spi_status      = SPI_UNKNOWN;
  spi_probes_left = MAX_PROBES;
  spi_cmd         = cmd;
  SPI_DELAY(cmd_deliver_probe);
}

/*
// buffer for packet to be sent
uint8_t radio_tx[IEEE_MAX_PAYLOAD_LEN];
// buffer to read from spi
uint8_t spi_rx[1 + IEEE_MAX_PAYLOAD_LEN];
// pointer to the packet beginning in the spi_rx
uint8_t * const loopback = (spi_rx + 1);


#define MAX_PROBES  16
#define PROBE_DELAY 1


spi_status_t spi_status = UNKNOWN;

static uint8_t check_status = STATUS_CHECK;

// safe deliverance of any command to receiver
static spi_status_t cmd_deliver(uint8_t command)
{
  uint8_t status;
  uint8_t iteration = 0;

  // Deliver our command and check response
  do {
    // if commands are being delivered consistently, 
    // need to ensure there is `probe delay` between them,
    nrf_delay_ms(PROBE_DELAY);  
    allspi_transfer(&command, 1, &status, 1);
    nrf_delay_ms(PROBE_DELAY);
    allspi_transfer(&check_status, 1, &status, 1);
    iteration += 2;
  } while (status == NO_RESPONSE && iteration < MAX_PROBES);

  // in case we haven't got any response
  if (status == NO_RESPONSE)
    return spi_status = DISCONNECTED;

  // when we got any response, wait until it's `success`
  while (status != SUCCESS && iteration < MAX_PROBES)
  {
    nrf_delay_ms(PROBE_DELAY);
    allspi_transfer(&check_status, 1, &status, 1);
    ++iteration;
  }

  // summarize
  if(status == SUCCESS)
    return spi_status = OK;
  else
    return spi_status = TIMEOUT;
}

// Changes channel of radio, initiates the same for receiver, waits for confirmation
void transmitter_set_channel(uint8_t channel)
{
  //spi_status = UNKNOWN;
  set_channel(channel);
  channel = get_channel();

  // ask to stop listening, we need only status information
  // not information about packs
  if (cmd_deliver(STOP_LISTENING) != OK)
    return;

  // request channel switch
  cmd_deliver(channel);
}

// initial state of a resulting structure
#define TX_EMPTY_RESULT \
  (transfer_result_t){  \
  .packs_sent    = 0, \
  .damaged_bits  = 0, \
  .damaged_bytes = 0, \
  .damaged_packs = 0, \
  .lost_packs    = 0, \
  .packs_len     = radio_tx[0] + 1, \
  .error = TX_NO_ERROR}

// number of unequal bits
static uint8_t diff_bits(uint8_t a, uint8_t b)
{
  uint8_t result = 0;
  for (; a != 0 || b != 0; a >>= 1, b >>= 1)
    if ((a & 1) != (b & 1))
      ++result;
    
  return result;
}

// transfers single pack and returns statistics
// dosn't do any preparations
static transfer_result_t single_pack(uint32_t delay)
{
  transfer_result_t result = TX_EMPTY_RESULT;
  result.packs_sent = 1;

  // send the pack and wait
  send_data(radio_tx, true);
  nrf_delay_ms(delay);

  // request loopback
  allspi_transfer(&check_status, 1, spi_rx, sizeof(spi_rx));

  if (spi_rx[0] == GOT_PACKET)
  {
    for (uint8_t i = 0; i < radio_tx[0]; ++i)
    {
      uint8_t bits = diff_bits(radio_tx[i], loopback[i]);
      if (bits > 0)
      {
        result.damaged_bits  += bits;
        result.damaged_bytes += 1;
      }
    }
    if (result.damaged_bits != 0)
      result.damaged_packs = 1;
  } 
  else // packet lost
  {
    result.damaged_bytes = radio_tx[0] + 1;
    result.damaged_bits  = (radio_tx[0] + 1) * 8;
    result.lost_packs    = 1;
  }

  return result;
}

// an interface for sending one pack, preparing the receiver
transfer_result_t transmitter_transfer_pack(uint32_t delay)
{
  transfer_result_t result = TX_EMPTY_RESULT;

  if (radio_tx[0] == 0 || radio_tx[0] > (IEEE_MAX_PAYLOAD_LEN - 1))
  {
    result.error = TX_WRONG_LEN;
    return result;
  }

  cmd_deliver(START_LISTENING);

  result = single_pack(delay);

  return result;
}

// fill radio_tx with random data
static void generate_tx(uint8_t length)
{
  // cut length, if too big
  length = MIN(length, IEEE_MAX_PAYLOAD_LEN - 1);

  radio_tx[0] = length;
  for (uint8_t i = 1; i < (length + 1); ++i)
    radio_tx[i] = rnd8();
}

// starts testing session with output to cli
transfer_result_t transmitter_begin_test(uint8_t packs_len, uint32_t delay, uint32_t packs_number)
{
  transfer_result_t summary = TX_EMPTY_RESULT;
  transfer_result_t single_result;

  generate_tx(packs_len);
  cmd_deliver(START_LISTENING);

  for (uint32_t i = 0; i < packs_number; ++i)
  {
    //generate_tx(packs_len);
    single_result = single_pack(delay);
    summary.packs_sent    += 1;
    summary.damaged_bits  += single_result.damaged_bits;
    summary.damaged_bytes += single_result.damaged_bytes;
    summary.damaged_packs += single_result.damaged_packs;
    summary.lost_packs    += single_result.lost_packs;
    summary.error          = single_result.error;
    if (summary.error != TX_NO_ERROR)
      break;
  }

  return summary;
}*/