################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../Core/Audio/hihat.S \
../Core/Audio/kick.S \
../Core/Audio/snare1.S \
../Core/Audio/snare2.S 

OBJS += \
./Core/Audio/hihat.o \
./Core/Audio/kick.o \
./Core/Audio/snare1.o \
./Core/Audio/snare2.o 

S_UPPER_DEPS += \
./Core/Audio/hihat.d \
./Core/Audio/kick.d \
./Core/Audio/snare1.d \
./Core/Audio/snare2.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Audio/%.o: ../Core/Audio/%.S Core/Audio/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m33 -g3 -DDEBUG -c -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-Core-2f-Audio

clean-Core-2f-Audio:
	-$(RM) ./Core/Audio/hihat.d ./Core/Audio/hihat.o ./Core/Audio/kick.d ./Core/Audio/kick.o ./Core/Audio/snare1.d ./Core/Audio/snare1.o ./Core/Audio/snare2.d ./Core/Audio/snare2.o

.PHONY: clean-Core-2f-Audio

