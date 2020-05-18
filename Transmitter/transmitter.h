/*
 * Logic for transmitter subproject
 *
 * * */

 #include <stdint.h>

typedef enum {
    OK,
    TIMEOUT,
    DISCONNECTED,
    UNKNOWN
} spi_status_t;
extern spi_status_t spi_status;

// Changes channel of radio, initiates the same for receiver, waits for confirmation
void    transmitter_set_channel(uint8_t channel);

// Auto detects the best channel for transmission
uint8_t transmitter_set_channel_auto();

// Transfers a single packet to receiver, returns status of the transmission (see spi_protocol.h)
// delay is time after which pack is considered to be lost
uint8_t transmitter_single_transfer(uint8_t* tx, uint8_t tx_len, uint8_t** loopback, uint32_t delay);

// Starts testing session with outpu to cli
void	transmitter_begin_test(uint8_t packs_len, uint32_t delay, uint32_t packs_number);