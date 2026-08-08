#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_rcc_ex.h"

#include <stdbool.h>

#include "can.h"

extern FDCAN_HandleTypeDef hfdcan1;
extern CAN_CTRL_ST can_st;

void gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    //CAN
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    //indicate light
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}



/*
    Light
    PC6
*/
void indicate_light_toggle(void)
{
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);
}

int main(void)
{
    //16Mhz
    HAL_Init();
    gpio_init();
    can_init();

    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_ActivateNotification(&hfdcan1, 
                                    FDCAN_IT_RX_FIFO0_NEW_MESSAGE|
                                    FDCAN_IT_BUS_OFF|
                                    FDCAN_IT_ERROR_WARNING|
                                    FDCAN_IT_ERROR_PASSIVE,
                                    0);
    
    uint8_t can_data[3] = {0x03, 0x02, 0x01};

    while(1)
    {
        if(can_send(can_data, FDCAN_DLC_BYTES_3))
        {
            indicate_light_toggle();
            can_st.tx_done = 0;
        }
        else
        {
            //FIFO FULL, STATE BUSY
            uint32_t error = HAL_FDCAN_GetError(&hfdcan1);
            uint32_t state = HAL_FDCAN_GetState(&hfdcan1);
        }
        HAL_Delay(1000);
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
