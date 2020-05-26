
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#include "nrf_gpio.h"
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "nordic_common.h"
#include "nrf_error.h"

#include "nrf_drv_clock.h"
#include "nrf.h"

#include "allspi.h"
#include "allradio.h"
#include "alltime.h"
#include "allcli.h"
#include "random.h"



void init()
{
  // clock and time
  uint32_t err_code = nrf_drv_clock_init();
  APP_ERROR_CHECK(err_code);
  nrf_drv_clock_lfclk_request(NULL);

  err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);

  // random generator
  NRF_RNG->TASKS_START = 1;

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
}

int main(void)
{
  // init all needed modules
  init();
  allspi_init(NULL);
  radio_init();

  // radio settings to default
  set_channel(DEFAULT_CHANNEL);
  set_power(DEFAULT_POWER);

  allcli_init();
  allcli_start();

  while (1)
  {
    allcli_process();
  }
}
