
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
  scheduler_init();

  bsp_board_init(BSP_INIT_LEDS);

  allcli_init();
}

void cli_process_callback(void* params)
{
  allcli_process();
}

void timer_test(void* params)
{
  //printf("Time worked! %s", (char*)params);
  bsp_board_led_invert(0);
  start_timer(500, 1, timer_test, NULL);
}

int main(void)
{
  // init all needed modules
  init();

  // radio settings to default
  set_channel(DEFAULT_CHANNEL);
  set_power(DEFAULT_POWER);

  //allcli_init();
  allcli_start();

  scheduler_add(timer_test, 1, NULL);

  while (1)
  {
    scheduler_add(cli_process_callback, CLI_PRIORITY, NULL);
    scheduler_process();
    __WFE();
    //allcli_process();
  }
}
