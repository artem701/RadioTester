
#include <stdint.h>
#include <stdbool.h>

#include "nrf_gpio.h"
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "nordic_common.h"
#include "nrf_error.h"

#include "nrf_drv_clock.h"
#include "nrf.h"

#include "allspis.h"
#include "allradio.h"
#include "alluart.h"


uint8_t rx[IEEE_MAX_PAYLOAD_LEN];

void init()
{
    // log init
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    // clock and time
    uint32_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);

    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

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
        // Do nothing.
    }

    err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}


int main(void)
{
    init();
    radio_init();
    alluart_init();

    set_channel(DEFAULT_CHANNEL);
    set_power(DEFAULT_POWER);

    memset(rx, 0, IEEE_MAX_PAYLOAD_LEN);

    uint8_t prev = 0;

    while(1)
    {
	read_data(rx);
        uint8_t p = rx[1];
	printf("\n\r%d\n\r", (int)p);

        if (p != prev)
        {
	  for (int i = 0; i < LEDS_NUMBER; ++i)
	  {
	      if (((p >> i) & 1)/* && !bsp_board_led_state_get(i)*/)
	      {
		  bsp_board_led_on(i);
	      }
	      else /*if (bsp_board_led_state_get(i))*/
		  bsp_board_led_off(i);
	  }
	}
	prev = p;
    }
}
