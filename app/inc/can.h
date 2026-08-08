#include <stdbool.h>

typedef struct
{
    uint16_t tx_done;
    uint16_t rx_done;
}CAN_CTRL_ST;

bool can_init(void);

bool can_send(uint8_t* data, uint32_t len);