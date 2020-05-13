
#include <stdbool.h>


#include "allspi.h"
#include "alluart.h"

static volatile bool done = 0;

static uint8_t rx;

void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    if (p_event->type == NRF_DRV_SPI_EVENT_DONE)
      done = true;
}


int main(void)
{
    //radio_init(TRANSMIT_PATTERN_11110000);

    bsp_board_init(BSP_INIT_LEDS);
    allspi_init();
    alluart_init();
    
    printf("\n\rUART session started\n\r");

    //tx = 0;
    rx = 0;
  
    while (1)
    {
	// start transfer
        allspi_transfer(NULL, 0, &rx, 1);
	
        while (!done)
        {
	    __WFE();
        }

        // transmission finished
        done = false;
	bsp_board_led_invert(BSP_BOARD_LED_0);
	
	//send to uart
	if (rx)
	    printf("%c", rx);
        nrf_delay_ms(200);
    }
}
