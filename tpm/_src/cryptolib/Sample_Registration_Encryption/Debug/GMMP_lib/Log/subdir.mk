################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../GMMP_lib/Log/GMMP_Log.c 

OBJS += \
./GMMP_lib/Log/GMMP_Log.o 

C_DEPS += \
./GMMP_lib/Log/GMMP_Log.d 


# Each subdirectory must supply rules for building sources it contributes
GMMP_lib/Log/%.o: ../GMMP_lib/Log/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


