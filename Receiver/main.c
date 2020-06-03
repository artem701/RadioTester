
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

// header + hash
uint8_t spis_rx[sizeof(tx_spi_header_t) + 1];

uint8_t spis_txs[RX_MAX_PACK_BUFFER + 1][sizeof(rx_spis_header_t) + IEEE_MAX_PAYLOAD_LEN + 1];
// index of spis tx, used to transfer only short messages without payload
#define SPIS_TX_NO_LOOPBACK RX_MAX_PACK_BUFFER

// uint8_t radio_rxs[RX_MAX_PACK_BUFFER][IEEE_MAX_PAYLOAD_LEN];
// headers, received and sent
#define RX_HEADER(i) ((rx_spis_header_t*)spis_txs[i])
#define TX_HEADER ((tx_spi_header_t*)spis_rx)

// pointer to the packet beginning in the spis_tx
#define LOOPBACK(i) (spis_txs[i] + sizeof(rx_spis_header_t))
// length of radio loopback in spi_rx
// length byte + data
#define LOOPBACK_LEN(i) (1 + *LOOPBACK(i))
// overall spi_tx length
// header + payload + hash
#define SPIS_TX_LEN(i) (sizeof(rx_spis_header_t) + LOOPBACK_LEN(i) + 1)
#define SPIS_TX_LEN_NO_HASH(i) (sizeof(rx_spis_header_t) + LOOPBACK_LEN(i))

// current status of the receiver
static rx_status_t status = RX_WAITING;

// index of buffer, where to write radio rx
static uint8_t radio_destination = 0;

static void reset_packs()
{
  // clearing packets data (and no-loopback tx)
  for (int i = 0; i <= RX_MAX_PACK_BUFFER; ++i)
  {
    LOOPBACK(i)[0] = 0;
    RX_HEADER(i)->status = RX_WAITING;
  }

  radio_destination = 0;
}

// SPI session handler
void spis_event_handler(nrf_drv_spis_event_t event)
{
  if (event.evt_type != NRF_DRV_SPIS_XFER_DONE)
    return;

  // handling incoming message

  // forming next header in a special variable
  rx_spis_header_t header;
  header.status = RX_NOT_READY;
  header.pack_num = TX_HEADER->pack_num;

  uint8_t spis_tx_id = SPIS_TX_NO_LOOPBACK;
  uint8_t cmd = TX_HEADER->cmd;

  if (!check_hash(spis_rx, sizeof(spis_rx)))
  {
    // some data is lost, interrupt handling this message
    header.status = RX_LOSS;
  } 
  else if (cmd == TX_GET_STATUS)
  {
    header.status = status;
  }
  // commad to switch channel
  else if (cmd >= IEEE_MIN_CHANNEL && cmd <= IEEE_MAX_CHANNEL)
  {
    spis_tx_id = SPIS_TX_NO_LOOPBACK;
    // reconfigure radio settings, update status
    set_channel(cmd);
    read_data(LOOPBACK(radio_destination), true);
    header.status = get_channel() == cmd ? RX_SUCCESS : RX_FAIL;
  }
  // command to loopback one packet
  else if (cmd >= TX_REQUEST_PACK_FLOOR && cmd < (TX_REQUEST_PACK_FLOOR + RX_MAX_PACK_BUFFER))
  {
    // re-establish buffers for next transfer
    spis_tx_id = cmd - TX_REQUEST_PACK_FLOOR;
    header.status = RX_SUCCESS;
  } 
  else if (cmd == TX_RESET_RECEIVER)
  {
    reset_packs();
    read_data(LOOPBACK(radio_destination), true);
    header.status = RX_SUCCESS;
  }
  else
  {
    // unknown command
    header.status = RX_FAIL;
  }

  // saving current status
  status = header.status;
  // preparing transfer
  *RX_HEADER(spis_tx_id) = header;
  hashify(spis_txs[spis_tx_id], SPIS_TX_LEN_NO_HASH(spis_tx_id));
  allspis_transfer(spis_txs[spis_tx_id], SPIS_TX_LEN(spis_tx_id), spis_rx, sizeof(spis_rx));
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

  reset_packs();

  allspis_transfer(spis_txs[SPIS_TX_NO_LOOPBACK], SPIS_TX_LEN(SPIS_TX_NO_LOOPBACK), 
    spis_rx, sizeof(spis_rx));
  
  radio_init();
  set_channel(DEFAULT_CHANNEL);
}

int main(void)
{
  init();

  while(1)
  {
    // wait for packet and move to the next buffer
    read_data(LOOPBACK(radio_destination), false);
    radio_destination = (radio_destination + 1) % RX_MAX_PACK_BUFFER;
  }
}
