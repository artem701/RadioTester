
#include <stdint.h>
#include "random.h"

#include "nrf.h"
#include "nrf52840.h"
#include "app_util_platform.h"

void random_init()
{
    NRF_RNG->TASKS_START = 1;
}

uint8_t  rnd8 ()
{
    NRF_RNG->EVENTS_VALRDY = 0;

    while (NRF_RNG->EVENTS_VALRDY == 0)
    {
	__WFE();
        // Do nothing.
    }
    return NRF_RNG->VALUE;
}

uint32_t rnd32()
{
    uint32_t rnd = 0;
    for (uint8_t i = 0; i < 4; ++i)
    {
        rnd <<= 8;
	rnd |= rnd8();
    }
    return rnd;
}