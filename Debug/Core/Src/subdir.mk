################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/font12.c \
../Core/Src/font16.c \
../Core/Src/font20.c \
../Core/Src/font24.c \
../Core/Src/font48.c \
../Core/Src/font8.c \
../Core/Src/main.c \
../Core/Src/stm32_lcd.c \
../Core/Src/stm32l4xx_hal_msp.c \
../Core/Src/stm32l4xx_it.c \
../Core/Src/stm32l4xx_nucleo_bus.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32l4xx.c \
../Core/Src/vesc.c 

OBJS += \
./Core/Src/font12.o \
./Core/Src/font16.o \
./Core/Src/font20.o \
./Core/Src/font24.o \
./Core/Src/font48.o \
./Core/Src/font8.o \
./Core/Src/main.o \
./Core/Src/stm32_lcd.o \
./Core/Src/stm32l4xx_hal_msp.o \
./Core/Src/stm32l4xx_it.o \
./Core/Src/stm32l4xx_nucleo_bus.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32l4xx.o \
./Core/Src/vesc.o 

C_DEPS += \
./Core/Src/font12.d \
./Core/Src/font16.d \
./Core/Src/font20.d \
./Core/Src/font24.d \
./Core/Src/font48.d \
./Core/Src/font8.d \
./Core/Src/main.d \
./Core/Src/stm32_lcd.d \
./Core/Src/stm32l4xx_hal_msp.d \
./Core/Src/stm32l4xx_it.d \
./Core/Src/stm32l4xx_nucleo_bus.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32l4xx.d \
./Core/Src/vesc.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32L476xx -c -I../DISPLAY/App -I../DISPLAY/Target -I../Core/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc -I../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32L4xx/Include -I../Drivers/CMSIS/Include -I../Drivers/BSP/Components/mx25l6433f -I../Drivers/BSP/Components/Common -I../Drivers/BSP/Components/ili9341 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/font12.cyclo ./Core/Src/font12.d ./Core/Src/font12.o ./Core/Src/font12.su ./Core/Src/font16.cyclo ./Core/Src/font16.d ./Core/Src/font16.o ./Core/Src/font16.su ./Core/Src/font20.cyclo ./Core/Src/font20.d ./Core/Src/font20.o ./Core/Src/font20.su ./Core/Src/font24.cyclo ./Core/Src/font24.d ./Core/Src/font24.o ./Core/Src/font24.su ./Core/Src/font48.cyclo ./Core/Src/font48.d ./Core/Src/font48.o ./Core/Src/font48.su ./Core/Src/font8.cyclo ./Core/Src/font8.d ./Core/Src/font8.o ./Core/Src/font8.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32_lcd.cyclo ./Core/Src/stm32_lcd.d ./Core/Src/stm32_lcd.o ./Core/Src/stm32_lcd.su ./Core/Src/stm32l4xx_hal_msp.cyclo ./Core/Src/stm32l4xx_hal_msp.d ./Core/Src/stm32l4xx_hal_msp.o ./Core/Src/stm32l4xx_hal_msp.su ./Core/Src/stm32l4xx_it.cyclo ./Core/Src/stm32l4xx_it.d ./Core/Src/stm32l4xx_it.o ./Core/Src/stm32l4xx_it.su ./Core/Src/stm32l4xx_nucleo_bus.cyclo ./Core/Src/stm32l4xx_nucleo_bus.d ./Core/Src/stm32l4xx_nucleo_bus.o ./Core/Src/stm32l4xx_nucleo_bus.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32l4xx.cyclo ./Core/Src/system_stm32l4xx.d ./Core/Src/system_stm32l4xx.o ./Core/Src/system_stm32l4xx.su ./Core/Src/vesc.cyclo ./Core/Src/vesc.d ./Core/Src/vesc.o ./Core/Src/vesc.su

.PHONY: clean-Core-2f-Src

