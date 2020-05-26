
#include "alltime.h"

void alltime_init()
{
  /* Start low frequency crystal oscillator for app_timer (used by bsp) */
  NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
  NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
  NRF_CLOCK->TASKS_LFCLKSTART    = 1;

  while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
  {
    // Do nothing.
  }
}