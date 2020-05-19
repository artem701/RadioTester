
#include <stdint.h>
#include <stdbool.h>

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

#include "spi_protocol.h"

// byte of status + received payload
#define spis_tx_len 1 + IEEE_MAX_PAYLOAD_LEN
static uint8_t spis_tx[spis_tx_len];

uint8_t spis_rx[1];
//#define SET_STATUS(value) spis_tx[0] = (value);

// write directly to spis buffer
uint8_t* radio_rx = (spis_tx + 1);

static inline void set_status(uint8_t status)
{
    spis_tx[0] = status;

    // indication:
    // 0 diods, when waiting for commands from transmitter
    // 1 diod, when receiver is setting up
    // 2 diods, when receiver is waiting for packet
    // 3 diods, when receiver has processed a command from transmitter
    // 4th diod is on when packet is received, and off after status check from transmitter
    bsp_board_leds_off();
    switch (status)
    {
	case GOT_PACKET:
	    bsp_board_led_on(3);
	case SUCCESS:
	    bsp_board_led_on(2);
	case NO_PACKET:
	    bsp_board_led_on(1);
	case NOT_READY:
	    bsp_board_led_on(0);

    }
}

uint8_t get_status()
{
    return spis_tx[0];
}

uint8_t get_command()
{
    return spis_rx[0];
}


// if information about packs should be reported
bool listening     = false;

// when listening state was requested, but not confirmed with a status check
bool pre_listening = false;

// common init
void init()
{
    // clock and time
    uint32_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);

    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // start random nums generator
    // NRF_RNG->TASKS_START = 1;

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

// SPI session handler
void spis_event_handler(nrf_drv_spis_event_t event)
{
    if (event.evt_type == NRF_DRV_SPIS_XFER_DONE)
    {
	uint8_t cmd = get_command();
	// set `in progress` state and drop the flag if it's a task, not status check
        if (cmd != STATUS_CHECK)
	{
	    set_status(NOT_READY);
	    pre_listening = false;
	}

        // reassign buffers for the next session
        allspis_transfer(spis_tx, spis_tx_len, spis_rx, 1);
        
        // perform requested actions
        switch(cmd)
        {
	    case STATUS_CHECK:
		 if (get_status() == SUCCESS) // ??
		 {
		    set_status(NO_RESPONSE);
		 }
		// if the receiver is in the listening state,
                // we should drop the packet flag every check
		if (listening || pre_listening)
		    set_status(NO_PACKET);

		// got confirmation to enter listening state
		if (pre_listening)
                {
		    listening = true;
		    pre_listening = false;
                }
            break;
	    case START_LISTENING:
		pre_listening = true;
		set_status(SUCCESS);
            break;
            case STOP_LISTENING:
		listening = false;
                set_status(SUCCESS);
                bsp_board_led_off(3);
            break;
	    default:
		// reconfigure radio settings
		set_channel(cmd);
		read_data(radio_rx, true);
                set_status ((get_channel() == cmd) ? SUCCESS : NOT_READY);
            break;
        }
    }
}

int main(void)
{
    init();
    radio_init();

    bsp_board_init(BSP_INIT_LEDS);

    // when every module is inited, open SPI listener
    set_status(NOT_READY);
    allspis_init(spis_event_handler);
    allspis_transfer(spis_tx, spis_tx_len, spis_rx, 1);

    while(1)
    {
	// wait until should start listening
	while(!listening) __WFE();
	
        // wait for incoming packet
	read_data(radio_rx, false);
	
        // if listening is still needed, confirm income
	if (listening)
	    set_status(GOT_PACKET);
    }

}
