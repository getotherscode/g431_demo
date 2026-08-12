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
    can_st_init();

    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_ActivateNotification(&hfdcan1, 
                                    FDCAN_IT_RX_FIFO0_NEW_MESSAGE|
                                    FDCAN_IT_BUS_OFF|
                                    FDCAN_IT_ERROR_WARNING|
                                    FDCAN_IT_ARB_PROTOCOL_ERROR |
                                    FDCAN_IT_DATA_PROTOCOL_ERROR |
                                    FDCAN_IT_ERROR_PASSIVE|
                                    FDCAN_IT_ERROR_LOGGING_OVERFLOW,
                                    0);

    while(1)
    {
        //can_echo_demo();
        can_active_send_demo();
        HAL_Delay(100);
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
