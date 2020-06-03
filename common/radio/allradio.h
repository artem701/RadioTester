/*
 * Warp for radio driver
 *
 * * */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "nrf_gpio.h"
#include "nordic_common.h"
#include "nrf_error.h"
#include "alltime.h"

#define IEEE_MAX_PAYLOAD_LEN 127   
#define IEEE_MIN_CHANNEL     11    
#define IEEE_MAX_CHANNEL     26    

#define DEFAULT_CHANNEL IEEE_MIN_CHANNEL
#define DEFAULT_POWER RADIO_TXPOWER_TXPOWER_0dBm

void radio_init();

// first byte must be equal to the length of data
void send_data(uint8_t* data, bool is_async);

void read_data(uint8_t* buf, bool is_async);

// stop sending or listening
void radio_stop();

void    set_channel(uint8_t channel);
uint8_t get_channel();

void    set_power(uint8_t power);
uint8_t get_power();

// returns power level on the given channel (converted to the IEEE 802.15.4 scale)
uint8_t check_power(uint8_t channel);

// returns channel from start to end which has minimum level of incoming signal
uint8_t best_channel_in_range(uint8_t start, uint8_t end);

// same, but for all possible channels
uint8_t best_channel();

// translate a numeric constant of power to a string
char* power_to_str(uint8_t power);
