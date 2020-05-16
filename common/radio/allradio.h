/*
 * Обертка над стандартным радио интерфейсом
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

#define IEEE_MAX_PAYLOAD_LEN      127   
#define IEEE_MIN_CHANNEL          11    
#define IEEE_MAX_CHANNEL          26    

#define DEFAULT_CHANNEL IEEE_MIN_CHANNEL
#define DEFAULT_POWER	RADIO_TXPOWER_TXPOWER_0dBm

void radio_init();

// first byte must be equal to the length of data
void send_data(uint8_t* data, bool is_async);

void read_data(uint8_t* buf, bool is_async);

void set_channel(uint8_t channel);
void set_power	(uint8_t power	);