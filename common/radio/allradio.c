
#include <string.h>
#include <stdbool.h>

#include "allradio.h"
#include "random.h"

// Frequency calculation for a given channel in the IEEE 802.15.4 radio mode.
#define IEEE_FREQ_CALC(_channel) (IEEE_DEFAULT_FREQ + \
                                  (IEEE_DEFAULT_FREQ * ((_channel) - IEEE_MIN_CHANNEL))) 

#define IEEE_DEFAULT_FREQ         (5)   /**< IEEE 802.15.4 default frequency. */
#define RADIO_LENGTH_LENGTH_FIELD (8UL) /**< Length on air of the LENGTH field. */

static volatile uint8_t current_power   = DEFAULT_POWER;
static volatile uint8_t current_channel = DEFAULT_CHANNEL;



void radio_init()
{
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

uint8_t get_channel()
{
    return current_channel;
}

void set_power(uint8_t power)
{
    radio_disable();
    current_power = power;
    NRF_RADIO->TXPOWER = (power << RADIO_TXPOWER_TXPOWER_Pos);
}

uint8_t get_power()
{
    return current_power;
}

void send_data(uint8_t* data, bool is_async)
{
    radio_disable();

    radio_init();

    NRF_RADIO->PACKETPTR = (uint32_t)data;

    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk /* | RADIO_SHORTS_PHYEND_START_Msk /* ? */;
    
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
    radio_disable();
 
    radio_init();

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

#define ED_RSSISCALE 4 // From electrical specifications
uint8_t check_power(uint8_t channel)
{
    uint8_t last_channel = current_channel;
    
    radio_disable();

    //radio_init();

    NRF_RADIO->MODE      = (RADIO_MODE_MODE_Ieee802154_250Kbit << RADIO_MODE_MODE_Pos);
    NRF_RADIO->SHORTS    = 0;
    set_channel(channel);
    NRF_RADIO->TASKS_RXEN = 1U; // ?

    uint32_t sample;
    NRF_RADIO->EVENTS_EDEND  = 0;
    NRF_RADIO->TASKS_EDSTART = 1;

    while (NRF_RADIO->EVENTS_EDEND != 1)
    {
	// wait
    }
    sample = NRF_RADIO->EDSAMPLE; // Read level
    set_channel(last_channel); // return to the channel
    return (uint8_t)((sample > 63) ? 255 : sample * ED_RSSISCALE); // Convert to IEEE 802.15.4 scale
}

uint8_t best_channel_in_range(uint8_t start, uint8_t end)
{
    start = MAX(start, IEEE_MIN_CHANNEL);
    end   = MIN(end,   IEEE_MAX_CHANNEL);
    end	  = MAX(start, end);

    uint8_t min_power = check_power(start);
    uint8_t best_channel = start;

    for(uint8_t i = start + 1; i <= end; ++i)
    {
	uint8_t pwr = check_power(i);
        if (pwr < min_power)
        {
	    best_channel = i;
	    min_power    = pwr;
        }
    }

    return best_channel;
}

uint8_t best_channel()
{
    return best_channel_in_range(IEEE_MIN_CHANNEL, IEEE_MAX_CHANNEL);
}

char* power_to_str(uint8_t power)
{
    switch(power)
    {
	case 0x00UL:
		return "0 dBm";
	case 0x02UL:
		return "+2 dBm";
	case 0x03UL:
		return "+3 dBm";
	case 0x04UL:
		return "+4 dBm";
	case 0x05UL:
		return "+5 dBm";
	case 0x06UL:
		return "+6 dBm";
	case 0x07UL:
		return "+7 dBm";
	case 0x08UL:
		return "+8 dBm";
	case 0xD8UL:
		return "-40 dBm";
	case 0xE2UL:
		return "-40 dBm";
	case 0xECUL:
		return "-20 dBm";
	case 0xF0UL:
		return "-16 dBm";
	case 0xF4UL:
		return "-12 dBm";
	case 0xF8UL:
		return "-8 dBm";
	case 0xFCUL:
		return "-4 dBm";
    }
}