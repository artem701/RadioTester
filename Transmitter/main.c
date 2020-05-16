
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


static volatile bool done;

static uint8_t tx[IEEE_MAX_PAYLOAD_LEN];

void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    if (p_event->type == NRF_DRV_SPI_EVENT_DONE)
      done = true;
}

uint8_t pwr[] = {
    RADIO_TXPOWER_TXPOWER_Neg40dBm,
    RADIO_TXPOWER_TXPOWER_Neg20dBm,
    RADIO_TXPOWER_TXPOWER_Neg16dBm,
    RADIO_TXPOWER_TXPOWER_Neg8dBm,
    RADIO_TXPOWER_TXPOWER_Neg4dBm,
    RADIO_TXPOWER_TXPOWER_0dBm,
    RADIO_TXPOWER_TXPOWER_Pos2dBm,
    RADIO_TXPOWER_TXPOWER_Pos4dBm,
    RADIO_TXPOWER_TXPOWER_Pos6dBm,
    RADIO_TXPOWER_TXPOWER_Pos8dBm,
};

uint32_t delays[] = {600, 540,480,420,360,300,240,180,120,60};

/* button 1: pwr+
 * button 2: pwr-
 * button 3: pack+
 * button 4: pack send
 * 
 * led 1: indicates power
 * led 2: invert on pack sent
 * led 3-4: last 2 bits of pack
 */

static volatile uint8_t pwr_id = 5;
static volatile uint8_t pack = 0;

void bsp_evt_handler(bsp_event_t evt)
{
    switch (evt)
    {
        case BSP_EVENT_KEY_0:
            if (pwr_id < (sizeof(pwr) - 1))
		++pwr_id;
	    set_power(pwr[pwr_id]);
	    break;
        case BSP_EVENT_KEY_1:
	    if (pwr_id > 0)
		--pwr_id;
	    set_power(pwr[pwr_id]);
            break;
        case BSP_EVENT_KEY_2:
	    ++pack;
            bsp_board_led_off(2);
            bsp_board_led_off(3);
            if (pack & 0b01) bsp_board_led_on(2);
            if (pack & 0b10) bsp_board_led_on(3);
	    tx[0] = 1;
	    tx[1] = pack;
            break;
        case BSP_EVENT_KEY_3:
	    tx[0] = 1;
	    tx[1] = pack;
            send_data(tx, false);
            bsp_board_led_invert(1);
            break;
        default:
            /* No implementation needed. */
            break;
    }
}

void init()
{
    //alluart_init();

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
	nrf_delay_ms(delays[pwr_id]);
        bsp_board_led_invert(0);
        bsp_board_led_off(1);
    /*
	nrf_delay_ms(500);
        send_data(tx, false);*/
    }
}
