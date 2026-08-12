#include <stdbool.h>

#define CAN_RX_LEN  64

typedef struct
{
    uint8_t tx_done;
    uint8_t rx_done;
    uint8_t rx_buff[CAN_RX_LEN];
    uint16_t rx_len;
    
    bool bus_off;
    bool err_passive;
    bool err_warning;
    uint32_t psr;
    uint32_t ir;
    uint32_t ecr;
    uint32_t err_cnt;
    uint32_t err_type;
}CAN_CTRL_ST;

bool can_init(void);

void can_st_init(void);

bool can_send(uint8_t* data, uint32_t len);

void can_echo_demo(void);

void can_active_send_demo(void);