################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/ILI9341.c \
../Core/Src/SI5351.c \
../Core/Src/SN76489.c \
../Core/Src/YM2151.c \
../Core/Src/fat32.c \
../Core/Src/font5x7.c \
../Core/Src/main.c \
../Core/Src/msm6295.c \
../Core/Src/puff.c \
../Core/Src/sd_spi.c \
../Core/Src/shuffle.c \
../Core/Src/stm32h5xx_hal_msp.c \
../Core/Src/stm32h5xx_it.c \
../Core/Src/storage.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32h5xx.c \
../Core/Src/ui.c \
../Core/Src/vgm.c \
../Core/Src/vgz.c 

OBJS += \
./Core/Src/ILI9341.o \
./Core/Src/SI5351.o \
./Core/Src/SN76489.o \
./Core/Src/YM2151.o \
./Core/Src/fat32.o \
./Core/Src/font5x7.o \
./Core/Src/main.o \
./Core/Src/msm6295.o \
./Core/Src/puff.o \
./Core/Src/sd_spi.o \
./Core/Src/shuffle.o \
./Core/Src/stm32h5xx_hal_msp.o \
./Core/Src/stm32h5xx_it.o \
./Core/Src/storage.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32h5xx.o \
./Core/Src/ui.o \
./Core/Src/vgm.o \
./Core/Src/vgz.o 

C_DEPS += \
./Core/Src/ILI9341.d \
./Core/Src/SI5351.d \
./Core/Src/SN76489.d \
./Core/Src/YM2151.d \
./Core/Src/fat32.d \
./Core/Src/font5x7.d \
./Core/Src/main.d \
./Core/Src/msm6295.d \
./Core/Src/puff.d \
./Core/Src/sd_spi.d \
./Core/Src/shuffle.d \
./Core/Src/stm32h5xx_hal_msp.d \
./Core/Src/stm32h5xx_it.d \
./Core/Src/storage.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32h5xx.d \
./Core/Src/ui.d \
./Core/Src/vgm.d \
./Core/Src/vgz.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32H563xx -c -I../Core/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc -I../Drivers/STM32H5xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H5xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/ILI9341.cyclo ./Core/Src/ILI9341.d ./Core/Src/ILI9341.o ./Core/Src/ILI9341.su ./Core/Src/SI5351.cyclo ./Core/Src/SI5351.d ./Core/Src/SI5351.o ./Core/Src/SI5351.su ./Core/Src/SN76489.cyclo ./Core/Src/SN76489.d ./Core/Src/SN76489.o ./Core/Src/SN76489.su ./Core/Src/YM2151.cyclo ./Core/Src/YM2151.d ./Core/Src/YM2151.o ./Core/Src/YM2151.su ./Core/Src/fat32.cyclo ./Core/Src/fat32.d ./Core/Src/fat32.o ./Core/Src/fat32.su ./Core/Src/font5x7.cyclo ./Core/Src/font5x7.d ./Core/Src/font5x7.o ./Core/Src/font5x7.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/msm6295.cyclo ./Core/Src/msm6295.d ./Core/Src/msm6295.o ./Core/Src/msm6295.su ./Core/Src/puff.cyclo ./Core/Src/puff.d ./Core/Src/puff.o ./Core/Src/puff.su ./Core/Src/sd_spi.cyclo ./Core/Src/sd_spi.d ./Core/Src/sd_spi.o ./Core/Src/sd_spi.su ./Core/Src/shuffle.cyclo ./Core/Src/shuffle.d ./Core/Src/shuffle.o ./Core/Src/shuffle.su ./Core/Src/stm32h5xx_hal_msp.cyclo ./Core/Src/stm32h5xx_hal_msp.d ./Core/Src/stm32h5xx_hal_msp.o ./Core/Src/stm32h5xx_hal_msp.su ./Core/Src/stm32h5xx_it.cyclo ./Core/Src/stm32h5xx_it.d ./Core/Src/stm32h5xx_it.o ./Core/Src/stm32h5xx_it.su ./Core/Src/storage.cyclo ./Core/Src/storage.d ./Core/Src/storage.o ./Core/Src/storage.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32h5xx.cyclo ./Core/Src/system_stm32h5xx.d ./Core/Src/system_stm32h5xx.o ./Core/Src/system_stm32h5xx.su ./Core/Src/ui.cyclo ./Core/Src/ui.d ./Core/Src/ui.o ./Core/Src/ui.su ./Core/Src/vgm.cyclo ./Core/Src/vgm.d ./Core/Src/vgm.o ./Core/Src/vgm.su ./Core/Src/vgz.cyclo ./Core/Src/vgz.d ./Core/Src/vgz.o ./Core/Src/vgz.su

.PHONY: clean-Core-2f-Src

