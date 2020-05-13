
#include "allspis.h"

const nrf_drv_spis_t spis = NRF_DRV_SPIS_INSTANCE(SPIS_INSTANCE);  /**< SPIS instance. */

// initializing SPIS
void allspis_init()
{

    // creating SPIS config
    nrf_drv_spis_config_t spis_config = NRF_DRV_SPIS_DEFAULT_CONFIG;
    spis_config.csn_pin   = APP_SPIS_CS_PIN;
    spis_config.miso_pin = APP_SPIS_MISO_PIN;
    spis_config.mosi_pin = APP_SPIS_MOSI_PIN;
    spis_config.sck_pin  = APP_SPIS_SCK_PIN;
    APP_ERROR_CHECK(nrf_drv_spis_init(&spis, &spis_config, spis_event_handler));
}

// start transmission
void allspis_transfer(uint8_t* tx, uint8_t len_tx, uint8_t* rx, uint8_t len_rx)
{
    APP_ERROR_CHECK(nrf_drv_spis_buffers_set(&spis, tx, len_tx, rx, len_rx));
}
