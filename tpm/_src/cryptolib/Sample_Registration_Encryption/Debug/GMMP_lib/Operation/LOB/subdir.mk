################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../GMMP_lib/Operation/LOB/GMMP_LOB.c 

OBJS += \
./GMMP_lib/Operation/LOB/GMMP_LOB.o 

C_DEPS += \
./GMMP_lib/Operation/LOB/GMMP_LOB.d 


# Each subdirectory must supply rules for building sources it contributes
GMMP_lib/Operation/LOB/%.o: ../GMMP_lib/Operation/LOB/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


