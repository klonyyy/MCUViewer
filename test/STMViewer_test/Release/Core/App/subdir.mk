################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU z strony ARMa
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/App/maincpp.cpp \
../Core/App/testClass.cpp 

OBJS += \
./Core/App/maincpp.o \
./Core/App/testClass.o 

CPP_DEPS += \
./Core/App/maincpp.d \
./Core/App/testClass.d 


# Each subdirectory must supply rules for building sources it contributes
Core/App/%.o Core/App/%.su: ../Core/App/%.cpp Core/App/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -DUSE_HAL_DRIVER -DSTM32G474xx -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-App

clean-Core-2f-App:
	-$(RM) ./Core/App/maincpp.d ./Core/App/maincpp.o ./Core/App/maincpp.su ./Core/App/testClass.d ./Core/App/testClass.o ./Core/App/testClass.su

.PHONY: clean-Core-2f-App

