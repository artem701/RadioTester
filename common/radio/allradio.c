
#include "allradio.h"
#include "random.h"

// Frequency calculation for a given channel in the IEEE 802.15.4 radio mode.
#define IEEE_FREQ_CALC(_channel) (IEEE_DEFAULT_FREQ + \
                                  (IEEE_DEFAULT_FREQ * ((_channel) - IEEE_MIN_CHANNEL))) 

#define IEEE_DEFAULT_FREQ         (5)   /**< IEEE 802.15.4 default frequency. */
#define RADIO_LENGTH_LENGTH_FIELD (8UL) /**< Length on air of the LENGTH field. */

static void radio_channel_set(uint8_t channel)
{
    if (channel >= IEEE_MIN_CHANNEL && channel <= IEEE_MAX_CHANNEL)
    {
	NRF_RADIO->FREQUENCY = IEEE_FREQ_CALC(channel);
    }
    else
    {
	NRF_RADIO->FREQUENCY = IEEE_DEFAULT_FREQ;
    }
}

uint8_t radio_init(transmit_pattern_t tpattern)
{
// mode = RADIO_MODE_MODE_Ieee802154_250Kbit
    // Отключение контрольной суммы
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Disabled << RADIO_CRCCNF_LEN_Pos);

    NRF_RADIO->TXADDRESS   = 0x00UL; // Set the device address 0 to use when transmitting
    NRF_RADIO->RXADDRESSES = 0x01UL; // Enable the device address 0 to use to select which addresses to receive

    // Set the address according to the transmission pattern.
    switch (tpattern)
    {
        case TRANSMIT_PATTERN_RANDOM:
            NRF_RADIO->PREFIX0 = rnd8();
            NRF_RADIO->BASE0   = rnd32();
            break;

        case TRANSMIT_PATTERN_11001100:
            NRF_RADIO->PREFIX0 = 0xCC;
            NRF_RADIO->BASE0   = 0xCCCCCCCC;
            break;

        case TRANSMIT_PATTERN_11110000:
            NRF_RADIO->PREFIX0 = 0xF0;
            NRF_RADIO->BASE0   = 0xF0F0F0F0;
            break;

        default:
            return RADIO_INIT_WRONG_PATTERN;
    }
    // Packet configuration:
    // S1 size = 0 bits, S0 size = 0 bytes, payload length size = 8 bits, 32-bit preamble.
    NRF_RADIO->PCNF0 = (RADIO_LENGTH_LENGTH_FIELD << RADIO_PCNF0_LFLEN_Pos) |
		       (RADIO_PCNF0_PLEN_32bitZero << RADIO_PCNF0_PLEN_Pos) |
		       (RADIO_PCNF0_CRCINC_Exclude << RADIO_PCNF0_CRCINC_Pos);
    NRF_RADIO->PCNF1 = (IEEE_MAX_PAYLOAD_LEN << RADIO_PCNF1_MAXLEN_Pos);

    NRF_RADIO->MODECNF0    |= (RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos);
      
    return RADIO_INIT_OK;
}

static void radio_disable(void)
{
    NRF_RADIO->SHORTS          = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;

    while (NRF_RADIO->EVENTS_DISABLED == 0)
    {
        __WFE(); // ?
        // Do nothing.
    }
    NRF_RADIO->EVENTS_DISABLED = 0;
}

void set_power(uint8_t power)
{
    radio_disable();
    NRF_RADIO->SHORTS  = RADIO_SHORTS_READY_START_Msk;
    NRF_RADIO->TXPOWER = (power << RADIO_TXPOWER_TXPOWER_Pos);
    //NRF_RADIO->MODE    = (RADIO_MODE_MODE_Ieee802154_250Kbit << RADIO_MODE_MODE_Pos);
    NRF_RADIO->TASKS_TXEN = 1;
}

