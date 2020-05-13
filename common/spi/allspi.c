
#include "allspi.h"

const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */


// initializing SPI
void allspi_init()
{

    // creating SPI config
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = SPI_SS_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;
    spi_config.sck_pin  = SPI_SCK_PIN;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
}

// start transmission
void allspi_transfer(uint8_t* tx, uint8_t len_tx, uint8_t* rx, uint8_t len_rx)
{
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, tx, len_tx, rx, len_rx));
}
