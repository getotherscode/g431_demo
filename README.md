# CAN
this is a CAN communicaition application for stm32g4

## chip-manual
https://www.st.com/resource/en/datasheet/stm32g431c6.pdf

## hal and ll lib
i do not want to put the large library folder in repository, please use link download and renamed it stm32g4xx-hal-drivers or create it by stm32cubemx  
https://github.com/STMicroelectronics/stm32g4xx-hal-driver/tree/a6001282dfacfff57e9710250f15e4333b578865  

## flash download 
### cmd
openocd -f interface/cmsis-dap.cfg -f target/stm32g4x.cfg -c "program ./CAN.hex verify reset exit"  

## debug
### svdfile download
https://github.com/modm-io/cmsis-svd-stm32/blob/main/stm32g4/STM32G431.svd  

### .vscode/launch.json
i have prepared the file  

## study tasks

### configure information understanding
read the comments from codes initialization in configure part   

### error handler  
CAN Frame: SOF ID DLC DATA CRC ACK EOF  

1> ACK  
Transmitter sends recessive 1 in ACK slot.  
Any receiver that correctly receives the frame sends dominant 0.  
If the transmitter samples 1, no receiver acknowledged the frame.  

2> Error Detective : TEC(Transmit Error Counter) & REC(Receive Error Counter)  
1) ack  error : send but no ack  
2) bit error  : the transmitted bit differs from the observed bus bit (transmitor Listen while Transmit), do not conclude arbitration lost  
3) crc error  : crc check  
4) stuff error: consecutive identical bits can not beyond 5  
   if transmitor need to send consecutive identical bits beyond 5, reverse bit insertion in 6 is needed   
5) form error : different part of the frame should be in correct place  

3> Node State Machine
1) Error Active:  TEC < 128 and REC < 128 (less errors)  
2) Error Passive: TEC > 128 or  REC > 128 (more errors)  
3) Bus-Off:       TEC >= 256, need to wait more than 128 times 11 consecutive bit, quit Bus  

### Arbitration

CAN uses CSMA/CR with non-destructive bitwise arbitration.  

Dominant = 0  
Recessive = 1  

If multiple nodes transmit simultaneously:  
the node transmitting recessive 1 but observing dominant 0  
loses arbitration and stops transmitting.  

Smaller CAN ID has higher priority.  

### hardware bus
1> mode theory  
1) Recessive, CANH ≈ CANL ≈ 2.5V, logic = 1  
2) Dominant, CANH - CANL typically ≈ 2V, logic = 0  
3) 120Ω teminal resistor to prevent reflection; normally use bus type, not use star topology  

2> my device reality  
1) specification: SN65HVD230  
2) resistance value: connect to node : 60Ω, independent: 120Ω  
3) voltage: idel, canh-gnd = canh-gnd = 1.66V  
4) oscilloscope: canh-canl = 2.5V  
5) wave picture  

### Test Error Catch

#### test-1
describe: reverse connect CANH and CANL  
as receiver: do not catch any error infomation  
as sender  : trigger and HAL_FDCAN_ErrorCallback and get error count, error type, psr  

#### test-2
describe: do not connect any node  
as sender: err_passive and err_warning is triggered when send once;  
           bus_off is triggered when err_cnt more than 16 times  
           test result do not represent TEC REC
