
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
#include "scheduler.h"


void init()
{
#ifdef NVMC_ICACHECNF_CACHEEN_Msk
  NRF_NVMC->ICACHECNF  = NVMC_ICACHECNF_CACHEEN_Enabled << NVMC_ICACHECNF_CACHEEN_Pos;
#endif // NVMC_ICACHECNF_CACHEEN_Msk
  
  alltime_init();
  random_init();
  allspi_init(NULL);
  radio_init();
  // radio settings to default
  set_channel(DEFAULT_CHANNEL);
  set_power(DEFAULT_POWER);

  bsp_board_init(BSP_INIT_LEDS);

  allcli_init();
  allcli_start();
}

void cli_process_callback(void* params)
{
  allcli_process();
}

int main(void)
{
  // init all needed modules
  init();

  while (1)
  {
    allcli_process();
  }
}
