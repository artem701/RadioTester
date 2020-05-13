
#include "allradio.h"
#include "random.h"


void radio_tx_init(transmit_pattern_t tpattern)
{
// mode = RADIO_MODE_MODE_Ieee802154_250Kbit
    /*
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
                       ((sizeof(m_tx_packet) - 1) << RADIO_PCNF1_MAXLEN_Pos);
    */

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
            return;
    }
    // Packet configuration:
    // S1 size = 0 bits, S0 size = 0 bytes, payload length size = 8 bits, 32-bit preamble.
    NRF_RADIO->PCNF0 = (RADIO_LENGTH_LENGTH_FIELD << RADIO_PCNF0_LFLEN_Pos) |
		       (RADIO_PCNF0_PLEN_32bitZero << RADIO_PCNF0_PLEN_Pos) |
		       (RADIO_PCNF0_CRCINC_Exclude << RADIO_PCNF0_CRCINC_Pos);
    NRF_RADIO->PCNF1 = (IEEE_MAX_PAYLOAD_LEN << RADIO_PCNF1_MAXLEN_Pos);

    NRF_RADIO->MODECNF0    |= (RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos);
      
}