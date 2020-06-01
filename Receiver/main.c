
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf_gpio.h"
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "nordic_common.h"
#include "nrf_error.h"

#include "nrf_drv_clock.h"
#include "nrf_drv_timer.h"
#include "nrf.h"

#include "allspis.h"
#include "allradio.h"
#include "hash.h"

#include "spi_protocol.h"

// header + payload + hash
static uint8_t spis_tx[sizeof(rx_spis_header_t) + IEEE_MAX_PAYLOAD_LEN + 1];

// header + hash
uint8_t spis_rx[sizeof(tx_spi_header_t) + 1];
/*
// type of packet buffer
typedef uint8_t[IEEE_MAX_PAYLOAD_LEN] radio_rx_t;

// all data buffers
radio_rx_t radio_rxs[RX_MAX_PACK_BUFFER];
*/
uint8_t radio_rxs[RX_MAX_PACK_BUFFER][IEEE_MAX_PAYLOAD_LEN];
// headers, received and sent
#define RX_HEADER ((rx_spis_header_t*)spis_tx)
#define TX_HEADER ((tx_spi_header_t*)spis_rx)

// pointer to the packet beginning in the spis_tx
uint8_t * const loopback = (spis_tx + sizeof(rx_spis_header_t));

// length of radio loopback in spi_rx
// length byte + data
#define LOOPBACK_LEN (1 + *loopback)
// overall spi_rx length
// header + payload
#define SPIS_TX_LEN_NO_HASH (sizeof(rx_spis_header_t) + LOOPBACK_LEN)

// Перенести пакет в область передачи данных по SPI
void prepare_pack(uint8_t pack_id)
{
  if (pack_id >= RX_MAX_PACK_BUFFER)
  {
    *loopback = 0;
    return;
  }

  memcpy(loopback, radio_rxs[pack_id], radio_rxs[pack_id][0] + 1);
  // forget the pack
  radio_rxs[pack_id][0] = 0;
}

// SPI session handler
void spis_event_handler(nrf_drv_spis_event_t event)
{
  if (event.evt_type != NRF_DRV_SPIS_XFER_DONE)
    return;

  // handling incoming message
  rx_status_t status_buf = RX_HEADER->status;
  RX_HEADER->status = RX_NOT_READY;
  RX_HEADER->pack_num = TX_HEADER->pack_num;
  uint8_t cmd = TX_HEADER->cmd;
  // do we need this? 
  //*loopback = 0;

  uint8_t sz = sizeof(spis_rx);
  size_t szz = sizeof(spis_rx);
  if (!check_hash(spis_rx, sizeof(spis_rx)))
  {
    // some data is lost, interrupt handling this message
    RX_HEADER->status = RX_LOSS;
  } 
  else if (cmd == TX_GET_STATUS)
  {
    RX_HEADER->status = status_buf;
  } 
  else if (cmd >= IEEE_MIN_CHANNEL && cmd <= IEEE_MAX_CHANNEL)
  {
    // change channel, update status
    set_channel(cmd);
    RX_HEADER->status = get_channel() == cmd ? RX_SUCCESS : RX_FAIL;
  }
  else if (cmd >= TX_REQUEST_PACK_FLOOR && cmd < (TX_REQUEST_PACK_FLOOR + RX_MAX_PACK_BUFFER))
  {
    // load pack to the buffers
    prepare_pack(cmd - TX_REQUEST_PACK_FLOOR);
  }
  else
  {
    // unknown command
    RX_HEADER->status = RX_FAIL;
  }

  hashify(spis_tx, SPIS_TX_LEN_NO_HASH);
  // re-establish buffers for next transfer
  allspis_transfer(spis_tx, sizeof(spis_tx), spis_rx, sizeof(spis_rx));
}


// common init
void init()
{
  // clock and time
  uint32_t err_code = nrf_drv_clock_init();
  APP_ERROR_CHECK(err_code);
  nrf_drv_clock_lfclk_request(NULL);

  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);

#ifdef NVMC_ICACHECNF_CACHEEN_Msk
  NRF_NVMC->ICACHECNF  = NVMC_ICACHECNF_CACHEEN_Enabled << NVMC_ICACHECNF_CACHEEN_Pos;
#endif // NVMC_ICACHECNF_CACHEEN_Msk

  // Start 64 MHz crystal oscillator.
  NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_HFCLKSTART    = 1;

  // Wait for the external oscillator to start up.
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
  {
  }
  
  err_code = bsp_init(BSP_INIT_LEDS, NULL);
  APP_ERROR_CHECK(err_code);

  allspis_init(spis_event_handler);

  // initing receiver

  // clearing packets data
  for (int i = 0; i < RX_MAX_PACK_BUFFER; ++i)
    radio_rxs[i][0] = 0;

  *loopback = 0;
  RX_HEADER->status = RX_SUCCESS;
  hashify(spis_tx, SPIS_TX_LEN_NO_HASH);
  allspis_transfer(spis_tx, sizeof(spis_tx), spis_rx, sizeof(spis_rx));
}

int main(void)
{
  init();
  radio_init();

  bsp_board_init(BSP_INIT_LEDS);

  set_channel(DEFAULT_CHANNEL);

  // when every module is inited, open SPI listener
  //set_status(NOT_READY);
  //allspis_transfer(spis_tx, spis_tx_len, spis_rx, 1);

  uint8_t radio_destination = 0;
  while(1)
  {
    // wait for packet and move to the next buffer
    read_data(radio_rxs[radio_destination], false);
    radio_destination = (radio_destination + 1) % RX_MAX_PACK_BUFFER;
  }
}
