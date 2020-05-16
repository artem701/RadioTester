
#include <string.h>
#include <stdbool.h>

#include "allradio.h"
#include "radio_test.h"

// Frequency calculation for a given channel in the IEEE 802.15.4 radio mode.
#define IEEE_FREQ_CALC(_channel) (IEEE_DEFAULT_FREQ + \
                                  (IEEE_DEFAULT_FREQ * ((_channel) - IEEE_MIN_CHANNEL))) 

#define IEEE_DEFAULT_FREQ         (5)   /**< IEEE 802.15.4 default frequency. */
#define RADIO_LENGTH_LENGTH_FIELD (8UL) /**< Length on air of the LENGTH field. */

static volatile bool inited = false;

static volatile uint8_t current_power   = DEFAULT_POWER;
static volatile uint8_t current_channel = DEFAULT_CHANNEL;

/**
 * @brief Function for generating an 8-bit random number with the internal random generator.
 */
static uint32_t rnd8(void)
{
    NRF_RNG->EVENTS_VALRDY = 0;

    while (NRF_RNG->EVENTS_VALRDY == 0)
    {
        // Do nothing.
    }
    return NRF_RNG->VALUE;
}


/**
 * @brief Function for generating a 32-bit random number with the internal random generator.
 */
static uint32_t rnd32(void)
{
    uint8_t  i;
    uint32_t val = 0;

    for (i = 0; i < 4; i++)
    {
        val <<= 8;
        val  |= rnd8();
    }
    return val;
}

void radio_init()
{
    inited = false;

    // Reset Radio ramp-up time.
    NRF_RADIO->MODECNF0 &= (~RADIO_MODECNF0_RU_Msk);

    // Packet configuration:
    // Bit 25: 1 Whitening enabled
    // Bit 24: 1 Big endian,
    // 4-byte base address length (5-byte full address length),
    // 0-byte static length, max 255-byte payload .
    NRF_RADIO->PCNF1 = (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos) |
                       (RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos) |
                       (4UL << RADIO_PCNF1_BALEN_Pos) |
                       (0UL << RADIO_PCNF1_STATLEN_Pos) |
                       ((IEEE_MAX_PAYLOAD_LEN - 1) << RADIO_PCNF1_MAXLEN_Pos);
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Disabled << RADIO_CRCCNF_LEN_Pos);

    NRF_RADIO->TXADDRESS   = 0x00UL; // Set the device address 0 to use when transmitting
    NRF_RADIO->RXADDRESSES = 0x01UL; // Enable the device address 0 to use to select which addresses to receive

/*
    NRF_RADIO->PREFIX0 = rnd8();
    NRF_RADIO->BASE0   = rnd32();*/

    NRF_RADIO->PREFIX0 = 0xCCCCCCCC;
    NRF_RADIO->BASE0   = 0xCCCCCCCC;

    // Packet configuration:
    // S1 size = 0 bits, S0 size = 0 bytes, payload length size = 8 bits, 32-bit preamble.
    NRF_RADIO->PCNF0 = (RADIO_LENGTH_LENGTH_FIELD << RADIO_PCNF0_LFLEN_Pos) |
		       (RADIO_PCNF0_PLEN_32bitZero << RADIO_PCNF0_PLEN_Pos) |
		       (RADIO_PCNF0_CRCINC_Exclude << RADIO_PCNF0_CRCINC_Pos);
    NRF_RADIO->PCNF1 = (IEEE_MAX_PAYLOAD_LEN << RADIO_PCNF1_MAXLEN_Pos);

    NRF_RADIO->MODECNF0    |= (RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos);
    
    inited = true;
}

static void radio_disable(void)
{
    NRF_RADIO->SHORTS          = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;

    while (NRF_RADIO->EVENTS_DISABLED == 0)
    {
        // Do nothing.
    }
    NRF_RADIO->EVENTS_DISABLED = 0;
}

void set_channel(uint8_t channel)
{
    if (channel >= IEEE_MIN_CHANNEL && channel <= IEEE_MAX_CHANNEL)
    {
	NRF_RADIO->FREQUENCY = IEEE_FREQ_CALC(channel);
        current_channel = channel;
    }
    else
    {
	//NRF_RADIO->FREQUENCY = IEEE_DEFAULT_FREQ;
        set_channel(DEFAULT_CHANNEL);
    }

}

void set_power(uint8_t power)
{
    radio_disable();
    current_power = power;
    NRF_RADIO->TXPOWER = (power << RADIO_TXPOWER_TXPOWER_Pos);
}

void send_data(uint8_t* data, bool is_async)
{
    if (!inited)
	return;

    radio_disable();

    NRF_RADIO->PACKETPTR = (uint32_t)data;

    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk /*| RADIO_SHORTS_PHYEND_START_Msk /* ? */;
    
    NRF_RADIO->TXPOWER = (current_power << RADIO_TXPOWER_TXPOWER_Pos);
    NRF_RADIO->MODE    = (RADIO_MODE_MODE_Ieee802154_250Kbit << RADIO_MODE_MODE_Pos);
    set_channel(current_channel);
    

    NRF_RADIO->EVENTS_END = 0U;
    NRF_RADIO->TASKS_TXEN = 1;

    // if we don't need to wait
    if (is_async)
	return;

    while (NRF_RADIO->EVENTS_END == 0U)
    {
        // wait for transmission to end
    }   

}

void read_data(uint8_t* buf, bool is_async)
{
    if (!inited)
	return;

    radio_disable();
    
    NRF_RADIO->MODE      = (RADIO_MODE_MODE_Ieee802154_250Kbit << RADIO_MODE_MODE_Pos);
    NRF_RADIO->SHORTS    = RADIO_SHORTS_READY_START_Msk /*| RADIO_SHORTS_END_START_Msk*/;
    NRF_RADIO->PACKETPTR = (uint32_t)buf;
   
    radio_init();
    set_channel(current_channel);
    

    NRF_RADIO->TASKS_RXEN = 1U;

    if (is_async)
	return;

    while (!NRF_RADIO->EVENTS_PHYEND)
    {
	// wait for incoming packet
    }

    NRF_RADIO->EVENTS_PHYEND = 0;
}