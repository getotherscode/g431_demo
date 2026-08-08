# CAN
this is a CAN communicaition application for stm32g4

## chip-manual
https://www.st.com/resource/en/datasheet/stm32g431c6.pdf

## flash download 
### cmd
openocd -f interface/cmsis-dap.cfg -f target/stm32g4x.cfg -c "program ./CAN.hex verify reset exit"  

## debug
### svdfile download
https://github.com/modm-io/cmsis-svd-stm32/blob/main/stm32g4/STM32G431.svd  

### .vscode/launch.json
i have prepared the file  


