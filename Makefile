CC = arm-none-eabi-gcc
TARGET = CAN

CFLAGS = -Wall -Wextra -mcpu=cortex-m4 -DSTM32G431xx
LDFLAGS = -mcpu=cortex-m4 --specs=nosys.specs -TSTM32G431CBUx_FLASH.ld

DEBUG_FLAGS = $(CFLAGS) -g -O0
RELEASE_FLAGS = $(CFLAGS) -O2

APP_SRC     = app/src/main.c
APP_SRC    += app/src/syscalls.c
APP_SRC    += app/src/can.c
START_SRC   = start_up/startup_stm32g431xx.s
CMSIS_SRC   = cmsis-device-g4/Source/system_stm32g4xx.c
DRIVER_SRC  = stm32g4xx-hal-driver/Src/stm32g4xx_hal_rcc.c
DRIVER_SRC += stm32g4xx-hal-driver/Src/stm32g4xx_hal.c
DRIVER_SRC += stm32g4xx-hal-driver/Src/stm32g4xx_hal_gpio.c
DRIVER_SRC += stm32g4xx-hal-driver/Src/stm32g4xx_hal_cortex.c
DRIVER_SRC += stm32g4xx-hal-driver/Src/stm32g4xx_hal_fdcan.c

OBJ  = $(APP_SRC:.c=.o)
OBJ += $(START_SRC:.s=.o)
OBJ += $(CMSIS_SRC:.c=.o)
OBJ += $(DRIVER_SRC:.c=.o)

APP_INC_DIR   = app/inc
LIB_INC_DIR   = stm32g4xx-hal-driver/Inc
CMSIS_INC_DIR = cmsis-device-g4/Include

INC = $(addprefix -I,$(APP_INC_DIR)) $(addprefix -I,$(CMSIS_INC_DIR)) $(addprefix -I,$(LIB_INC_DIR))

# $1 means first parameter placeholder
FIX_PATH = $(subst /,\,$1)

.PHONY: debug release clean dumptext dumphead dumpnm download

debug:$(TARGET).elf
	@arm-none-eabi-objcopy -O ihex $< $(TARGET).hex
	@arm-none-eabi-size $(TARGET).hex

$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.s
	$(CC) $(DEBUG_FLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(INC) $(DEBUG_FLAGS) -c -o $@ $<

clean:
	-del /Q $(call FIX_PATH, $(OBJ))
	-del /Q $(TARGET).hex $(TARGET).elf

# disassembly
dumptext:
	arm-none-eabi-objdump -d $(TARGET).elf

dumphead:
	arm-none-eabi-objdump -h $(TARGET).elf

dumpnm:
	arm-none-eabi-nm $(TARGET).elf

download:
	openocd -f interface/cmsis-dap.cfg -f target/stm32g4x.cfg -c "program ./CAN.hex verify reset exit"


