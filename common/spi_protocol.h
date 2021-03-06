
// RECEIVER'S STATUSES

typedef enum {
  RX_NO_RESPONSE = 0,
  RX_WAITING,         // waiting for cmd
  RX_LOSS,            // hash fail
  RX_NOT_READY,       // cmd in progress
  RX_SUCCESS,         // performed cmd successfully
  RX_FAIL             // cmd failed

  // Don't need them, no packet <=> pack len set to 0
  //RX_NO_PACKET,
  //RX_GO_PACKET
} rx_status_t;

// max number of packs, stored in receiver
#define RX_MAX_PACK_BUFFER 5

typedef struct
{
  uint8_t pack_num;
  uint8_t status;
} rx_spis_header_t;

// COMMANDS FROM TRANSMITTER

// report current status
#define TX_GET_STATUS 1

// forget all packs, return writing pos to the start
#define TX_RESET_RECEIVER 2

// commands 11-26 for channel switch

#define TX_REQUEST_PACK_FLOOR 27
// command for requesting loopback of n'th pack
#define TX_REQUEST_PACK(n) (TX_REQUEST_PACK_FLOOR + (n))

typedef struct
{
  uint8_t pack_num;
  uint8_t cmd;
} tx_spi_header_t;