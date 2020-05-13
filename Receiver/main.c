
#include <stdint.h>
#include <stdbool.h>

#include "allspis.h"


#define TEST_STRING "There is no dark side of the Moon, really. In a matter of fact it is all dark."
static uint8_t       tx_buf[] = TEST_STRING;           /**< TX buffer. */
static const uint8_t len      = sizeof(tx_buf);        /**< Transfer length. */

static volatile bool done = 0;

void spis_event_handler(nrf_drv_spis_event_t event)
{
    if (event.evt_type == NRF_DRV_SPIS_XFER_DONE)
    {
	done = true;
    }
}


int main(void)
{
    NRF_POWER->TASKS_CONSTLAT = 1;

    bsp_board_init(BSP_INIT_LEDS);
    allspis_init();
    
    uint8_t i = 0;
    while (1)
    {
	done = false;
	// start transfer
        allspis_transfer(&tx_buf[i], 1, NULL, 0);
	
        while (!done)
        {
            __WFE();
        }
	i = (i + 1) % (len-1);
        // transmission finished
	bsp_board_led_invert(BSP_BOARD_LED_0);
    }
}
