################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ralib/ralib.c \
../ralib/ra_com_mini.c \
../ralib/tpm_util.c \
../ralib/identity.c \
../ralib/remote_attestation.c

OBJS += \
./ralib/ralib.o \
./ralib/ra_com_mini.o \
./ralib/tpm_util.o \
./ralib/identity.o \
./ralib/remote_attestation.o

C_DEPS += \
./ralib/ralib.d \
./ralib/ra_com_mini.d \
./ralib/tpm_util.d \
./ralib/identity.d \
./ralib/remote_attestation.d


# Each subdirectory must supply rules for building sources it contributes
ralib/%.o: ../ralib/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


