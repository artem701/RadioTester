
#include "transmitter.h"
#include "spi_protocol.h"

#include "nrf_delay.h"

#include "allradio.h"
#include "allspi.h"
//#include "allradio.h"

#define MAX_PROBES  10
#define PROBE_DELAY 10

spi_status_t spi_status = UNKNOWN;

// safe deliverance of any command to receiver
static spi_status_t cmd_deliver(uint8_t command)
{
    uint8_t check_status = STATUS_CHECK;
    uint8_t status;
    uint8_t iteration = 0;
  
    // Deliver our command and check response
    do {
	allspi_transfer(&command, 1, &status, 1);
        nrf_delay_ms(PROBE_DELAY);
        allspi_transfer(&check_status, 1, &status, 1);
        ++iteration;
    } while (status == NO_RESPONSE && iteration < MAX_PROBES);

    // in case we haven't got any response
    if (status == NO_RESPONSE)
	return spi_status = DISCONNECTED;

    // when we got any response, wait until it's `success`
    while (status != SUCCESS && iteration < MAX_PROBES)
    {
	nrf_delay_ms(PROBE_DELAY);
        allspi_transfer(&check_status, 1, &status, 1);
        ++iteration;
    }

    // summarize
    if(status == SUCCESS)
	return spi_status = OK;
    else
	return spi_status = TIMEOUT;
}

// Changes channel of radio, initiates the same for receiver, waits for confirmation
void transmitter_set_channel(uint8_t channel)
{
    spi_status = UNKNOWN;
    set_channel(channel);
    channel = get_channel();

    // ask to stop listening, we need only status information
    // not information about packs
    if (cmd_deliver(STOP_LISTENING) != OK)
	return;

    // request channel switch
    cmd_deliver(channel);
}