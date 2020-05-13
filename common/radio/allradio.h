/*
 * ������� ��� ����������� ����� �����������
 *
 * * */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "radio_config.h"
#include "nrf_gpio.h"
#include "nordic_common.h"
#include "nrf_error.h"

#include "alltime.h"



#define IEEE_MAX_PAYLOAD_LEN      127   /**< IEEE 802.15.4 maximum payload length. */
#define IEEE_MIN_CHANNEL          11    /**< IEEE 802.15.4 minimum channel. */
#define IEEE_MAX_CHANNEL          26    /**< IEEE 802.15.4 maximum channel. */

/**@brief Radio transmit and address pattern.
 */
typedef enum
{
    TRANSMIT_PATTERN_RANDOM,   /**< Random pattern generated by RNG. */
    TRANSMIT_PATTERN_11110000, /**< Pattern 11110000(F0). */
    TRANSMIT_PATTERN_11001100, /**< Pattern 11001100(CC). */
} transmit_pattern_t;
//#define TPATTERN TRANSMIT_PATTERN_11110000;

#define RADIO_INIT_OK 0
#define RADIO_INIT_WRONG_PATTERN 1
uint8_t radio_init(transmit_pattern_t tpattern);



void set_power(uint8_t power);

void     send_packet(uint32_t packet);
uint32_t read_packet();

//void radio_disable();