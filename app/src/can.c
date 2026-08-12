/*
    PA12 CANFD_TX
    PA11 CANFD_RX
    AHB1-APB1-CAN1，GPIO is on AHB2
    hal_conf.h enable HAL_FDCAN_MODULE_ENABLED
*/
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_fdcan.h"
#include "stm32g4xx_hal_rcc.h"

#include <string.h>

#include "can.h"

#define CAN_HEAD 0x120

FDCAN_HandleTypeDef hfdcan1 = {0};
CAN_CTRL_ST can_st = {0};

bool can_init(void)
{
    __HAL_RCC_FDCAN_CLK_ENABLE();
    __HAL_RCC_FDCAN_CONFIG(RCC_FDCANCLKSOURCE_PCLK1);

    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init.ClockDivider            = FDCAN_CLOCK_DIV1;   
    /* classic frame max length 8 bytes, fd have max length 64 bytes  */ 
    hfdcan1.Init.FrameFormat             = FDCAN_FRAME_CLASSIC;
    hfdcan1.Init.Mode                    = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission      = DISABLE;
    hfdcan1.Init.TransmitPause           = DISABLE;
    hfdcan1.Init.ProtocolException       = DISABLE;
    /* arbitration  */
    //Nominal bit timing ：16MHz / (1+24+7) Tq = 500k 
    //1 bit time = sync seg + time seg1 + time seg2 Time quantum
    //determine 1.can speed, 2.sample point, 3.fault tolerance
    hfdcan1.Init.NominalPrescaler        = 1;   // 1/16Mhz, 24 + 1 + 7 = 32Tq, sample point = (1+24)/32 = 78%
    hfdcan1.Init.NominalSyncJumpWidth    = 1;   // sample point max adjust amount
    hfdcan1.Init.NominalTimeSeg1         = 24;  // before sample time, phase segment 1
    hfdcan1.Init.NominalTimeSeg2         = 7;   // after sample time, phase segment 2   
    // Data bit timing : classic is not effective, but asked not zero, only can fd use
    hfdcan1.Init.DataPrescaler           = 1;
    hfdcan1.Init.DataSyncJumpWidth       = 1;
    hfdcan1.Init.DataTimeSeg1            = 13;
    hfdcan1.Init.DataTimeSeg2            = 2;
    /* filter number configure, use standard filter not extend filter, smaller id has high priority */
    hfdcan1.Init.StdFiltersNbr           = 1;  // 11 bit ID
    hfdcan1.Init.ExtFiltersNbr           = 0;  // 29 bit ID
    /* TX FIFO mode: 1.FIFO 2.Queue: smaller id has high priority */
    hfdcan1.Init.TxFifoQueueMode         = FDCAN_TX_FIFO_OPERATION;
    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
    {
        return false;
    }

    /* config filter and receive stardard frame: ID = 0x123 */
    
    FDCAN_FilterTypeDef sFilterConfig;
    sFilterConfig.IdType       = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex  = 0;                                //CAN can configure more than 1 filter
    sFilterConfig.FilterType   = FDCAN_FILTER_MASK;                //type: mask, range, dual
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;          //Fifo0 call back
    sFilterConfig.FilterID1    = 0x123;
    sFilterConfig.FilterID2    = 0x7FF;                            //mask means all bits(11) need to be compared
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
    TxHeader.IdType               = FDCAN_STANDARD_ID;  //standard id is 11 bits, extend is 29 bits
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

void can_st_init(void)
{
    can_st.rx_len = 0;
    can_st.rx_done = 0;
    can_st.tx_done = 1;
}

void can_echo_demo(void)
{
    if(can_st.rx_done)
    {
        if(can_send(can_st.rx_buff, can_st.rx_len))
        {
            can_st.rx_done = 0;   
        }
    }
}

void can_active_send_demo(void)
{
    uint8_t temp_buff[3] = {0x01, 0x02, 0x03};
    can_send(temp_buff, FDCAN_DLC_BYTES_3);
}

// if you just have few message to send in a low frequency, ignore this flag
void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t BufferIndexes)
{
    (void)BufferIndexes;
    if(hfdcan->Instance == FDCAN1)
    {
        can_st.tx_done = 1;
    }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    FDCAN_RxHeaderTypeDef rx_header;
    uint8_t temp_rx_buff[FDCAN_DLC_BYTES_64];

    if(hfdcan->Instance == FDCAN1)
    {
        if(RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)
        {
            if(HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header,temp_rx_buff) == HAL_OK)
            {
                if(rx_header.Identifier == CAN_HEAD)
                {
                    can_st.rx_done = 1;

                    switch(rx_header.DataLength)
                    {
                        case FDCAN_DLC_BYTES_0:  can_st.rx_len = 0;  break;
                        case FDCAN_DLC_BYTES_1:  can_st.rx_len = 1;  break;
                        case FDCAN_DLC_BYTES_2:  can_st.rx_len = 2;  break;
                        case FDCAN_DLC_BYTES_3:  can_st.rx_len = 3;  break;
                        case FDCAN_DLC_BYTES_4:  can_st.rx_len = 4;  break;
                        case FDCAN_DLC_BYTES_5:  can_st.rx_len = 5;  break;
                        case FDCAN_DLC_BYTES_6:  can_st.rx_len = 6;  break;
                        case FDCAN_DLC_BYTES_7:  can_st.rx_len = 7;  break;
                        case FDCAN_DLC_BYTES_8:  can_st.rx_len = 8;  break;
                        case FDCAN_DLC_BYTES_12: can_st.rx_len = 12; break;
                        case FDCAN_DLC_BYTES_16: can_st.rx_len = 16; break;
                        case FDCAN_DLC_BYTES_20: can_st.rx_len = 20; break;
                        case FDCAN_DLC_BYTES_24: can_st.rx_len = 24; break;
                        case FDCAN_DLC_BYTES_32: can_st.rx_len = 32; break;
                        case FDCAN_DLC_BYTES_48: can_st.rx_len = 48; break;
                        case FDCAN_DLC_BYTES_64: can_st.rx_len = 64; break;
                    }

                    memcpy(can_st.rx_buff, temp_rx_buff, can_st.rx_len);
                }
            }
        }
    }


}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan)
{
    if(hfdcan->Instance == FDCAN1)
    {
        can_st.err_type = HAL_FDCAN_GetError(hfdcan);
        can_st.psr = hfdcan->Instance->PSR;
        can_st.psr = hfdcan->Instance->ECR;
        can_st.psr = hfdcan->Instance->IR;
        can_st.err_cnt++;
    }
}

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
    if(hfdcan->Instance == FDCAN1)
    {
        if(ErrorStatusITs & FDCAN_IR_BO)
        {
            // Bus-Off
            can_st.bus_off = true;
        }

        if(ErrorStatusITs & FDCAN_IR_EP)
        {
            // Error Passive
            can_st.err_passive = true;
        }

        if(ErrorStatusITs & FDCAN_IR_EW)
        {
            // Error Warning
            can_st.err_warning = true;
        }
    }
}

void FDCAN1_IT0_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&hfdcan1);
}