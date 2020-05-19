
#include "transmitter.h"
#include "spi_protocol.h"

#include "nrf_delay.h"

#include "allradio.h"
#include "allspi.h"

// buffer for packet to be sent
uint8_t radio_tx[    IEEE_MAX_PAYLOAD_LEN];
// buffer to read from spi
uint8_t   spi_rx[1 + IEEE_MAX_PAYLOAD_LEN];
// pointer to the packet beginning in the spi_rx
uint8_t * const loopback = (spi_rx + 1);


#define MAX_PROBES  16
#define PROBE_DELAY 1

spi_status_t spi_status = UNKNOWN;


static uint8_t check_status = STATUS_CHECK;

// safe deliverance of any command to receiver
static spi_status_t cmd_deliver(uint8_t command)
{
    uint8_t status;
    uint8_t iteration = 0;
  
    // Deliver our command and check response
    do {
	nrf_delay_ms(PROBE_DELAY); // ??
	allspi_transfer(&command, 1, &status, 1);
        nrf_delay_ms(PROBE_DELAY);
        allspi_transfer(&check_status, 1, &status, 1);
        iteration += 2;
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
    //spi_status = UNKNOWN;
    set_channel(channel);
    channel = get_channel();

    // ask to stop listening, we need only status information
    // not information about packs
    if (cmd_deliver(STOP_LISTENING) != OK)
	return;

    // request channel switch
    cmd_deliver(channel);
}

static uint8_t diff_bits(uint8_t a, uint8_t b)
{
    uint8_t result = 0;
    for (; a != 0 || b != 0; a >>= 1, b >>= 1)
	if ((a & 1) != (b & 1))
	    ++result;
}

transfer_result_t transmitter_transfer_pack(uint32_t delay)
{
    transfer_result_t result = (transfer_result_t){
	.packs_sent    = 1,
        .damaged_bits  = 0,
        .damaged_bytes = 0,
        .damaged_packs = 0,
        .lost_packs    = 0,
        .packs_len     = radio_tx[0] + 1, // count first byte too
        .error = TX_NO_ERROR
    };
    
    if (radio_tx[0] == 0 || radio_tx[0] > (IEEE_MAX_PAYLOAD_LEN - 1))
    {
	result.error = TX_WRONG_LEN;
        return result;
    }

    // turn the receiver in the listening mode
    cmd_deliver(START_LISTENING);

    // send the pack and wait
    send_data(radio_tx, true);
    nrf_delay_ms(delay);

    // request loopback
    allspi_transfer(&check_status, 1, spi_rx, sizeof(spi_rx));

    if (spi_rx[0] == GOT_PACKET)
    {
	for (uint8_t i = 0; i < radio_tx[0]; ++i)
        {
	    uint8_t bits = diff_bits(radio_tx[i], loopback[i]);
            if (bits > 0)
            {
		result.damaged_bits  += bits;
		result.damaged_bytes += 1;
            }
        }
        if (result.damaged_bits != 0)
	    result.damaged_packs = 1;
    } 
    else // packet lost
    {
	result.damaged_bytes = radio_tx[0] + 1;
	result.damaged_bits  = (radio_tx[0] + 1) * 8;
	result.lost_packs    = 1;
    }

    return result;
}