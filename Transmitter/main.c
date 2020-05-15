
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
#include "alluart.h"
#include "allradio.h"
#include "alltime.h"
#include "random.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static volatile bool done;

static uint8_t tx[IEEE_MAX_PAYLOAD_LEN];

void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    if (p_event->type == NRF_DRV_SPI_EVENT_DONE)
      done = true;
}


void bsp_evt_handler(bsp_event_t evt)
{
    tx[1] = 0;
    switch (evt)
    {
        case BSP_EVENT_KEY_0:
            /* Fall through. */
        case BSP_EVENT_KEY_1:
            /* Fall through. */
        case BSP_EVENT_KEY_2:
            /* Fall through. */
        case BSP_EVENT_KEY_3:
            /* Get actual button state. */
            for (int i = 0; i < BUTTONS_NUMBER; i++)
            {
                if(bsp_board_button_state_get(i))
		    bsp_board_led_invert(i);
                tx[1] |= (bsp_board_led_state_get(i) ? (1 << i) : 0);
            }
            break;
        default:
            /* No implementation needed. */
            break;
    }
    send_data(tx);
}

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
    //allspi_init();
    radio_init();

    uint32_t err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_evt_handler);
    APP_ERROR_CHECK(err_code);
    
    set_channel(DEFAULT_CHANNEL);
    set_power(DEFAULT_POWER);

    memset(tx, 0, IEEE_MAX_PAYLOAD_LEN);

    tx[0] = 1;
    tx[1] = 0;
    
    while (1)
    {
	nrf_delay_ms(1000);
        send_data(tx);
    }
}
