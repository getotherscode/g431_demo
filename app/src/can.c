/*
    PA12 CANFD_TX
    PA11 CANFD_RX
    AHB1-APB1-CAN1，GPIO is on AHB2
    hal_conf.h enable HAL_FDCAN_MODULE_ENABLED
*/
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_fdcan.h"
#include "stm32g4xx_hal_rcc.h"
#include "can.h"

FDCAN_HandleTypeDef hfdcan1 = {0};
CAN_CTRL_ST can_st = {0};

bool can_init(void)
{
    __HAL_RCC_FDCAN_CLK_ENABLE();
    __HAL_RCC_FDCAN_CONFIG(RCC_FDCANCLKSOURCE_PCLK1);

    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init.ClockDivider            = FDCAN_CLOCK_DIV1;    
    hfdcan1.Init.FrameFormat             = FDCAN_FRAME_CLASSIC; 
    hfdcan1.Init.Mode                    = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission      = DISABLE;
    hfdcan1.Init.TransmitPause           = DISABLE;
    hfdcan1.Init.ProtocolException       = DISABLE;
    /* arbitration segment：16MHz / (1+24+7) Tq = 500k */
    hfdcan1.Init.NominalPrescaler        = 1;
    hfdcan1.Init.NominalSyncJumpWidth    = 1;
    hfdcan1.Init.NominalTimeSeg1         = 24;     
    hfdcan1.Init.NominalTimeSeg2         = 7;      
    /* Data section: classic is not effective, but asked not zero*/
    hfdcan1.Init.DataPrescaler           = 1;
    hfdcan1.Init.DataSyncJumpWidth       = 1;
    hfdcan1.Init.DataTimeSeg1            = 13;
    hfdcan1.Init.DataTimeSeg2            = 2;
    /* filter number configure, use standard filter not extend filter */
    hfdcan1.Init.StdFiltersNbr           = 1; 
    hfdcan1.Init.ExtFiltersNbr           = 0;      
    /* TX FIFO mode */
    hfdcan1.Init.TxFifoQueueMode         = FDCAN_TX_FIFO_OPERATION;
    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
    {
        return false;
    }

    /* config filter and receive stardard frame: ID = 0x123 */
    
    FDCAN_FilterTypeDef sFilterConfig;
    sFilterConfig.IdType       = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex  = 0;
    sFilterConfig.FilterType   = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1    = 0x123;
    sFilterConfig.FilterID2    = 0x7FF;                            // mask：0x7FF = match all
    if(HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
    {
        return false;
    }
    

    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);

    return true;
}

bool can_send(uint8_t* data, uint32_t len)
{
    FDCAN_TxHeaderTypeDef TxHeader = {0};
    TxHeader.Identifier           = 0x123;
    TxHeader.IdType               = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType          = FDCAN_DATA_FRAME;
    TxHeader.DataLength           = len;
    TxHeader.ErrorStateIndicator  = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch        = FDCAN_BRS_OFF;
    TxHeader.FDFormat             = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl   = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker        = 0;

    if(HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, data) != HAL_OK)
    {
        return false;
    }

    return true;
}

void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t BufferIndexes)
{
    (void)hfdcan;
    (void)BufferIndexes;
    can_st.tx_done = 1;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    FDCAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    if(RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)
    {
        if(HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header,rx_data) == HAL_OK)
        {
            if(rx_header.Identifier == 0x120)
            {
                return;
            }
        }
    }
}

void FDCAN1_IT0_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&hfdcan1);
}