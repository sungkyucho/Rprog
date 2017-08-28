################################################################################
# Automatically-generated file. Do not edit!
################################################################################

#-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include ralib/subdir.mk
-include GMMP_lib/Util/subdir.mk
-include GMMP_lib/Operation/Remote/subdir.mk
-include GMMP_lib/Operation/Reg/subdir.mk
-include GMMP_lib/Operation/ProfileInfo/subdir.mk
-include GMMP_lib/Operation/Notification/subdir.mk
-include GMMP_lib/Operation/Multimedia/subdir.mk
-include GMMP_lib/Operation/LongSentence/subdir.mk
-include GMMP_lib/Operation/LOB/subdir.mk
-include GMMP_lib/Operation/Heartbeat/subdir.mk
-include GMMP_lib/Operation/FTP/subdir.mk
-include GMMP_lib/Operation/Encrypt/subdir.mk
-include GMMP_lib/Operation/Delivery/subdir.mk
-include GMMP_lib/Operation/Control/subdir.mk
-include GMMP_lib/Operation/subdir.mk
-include GMMP_lib/Network/subdir.mk
-include GMMP_lib/Log/subdir.mk
-include GMMP_lib/ErrorCode/subdir.mk
-include GMMP_lib/subdir.mk
-include subdir.mk
-include objects.mk

#ifneq ($(MAKECMDGOALS),clean)
#ifneq ($(strip $(C_DEPS)),)
#-include $(C_DEPS)
#endif
#endif

#-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Sample_All

# Tool invocations
Sample_All: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc  -o "Sample_All" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS) $(C_DEPS) $(EXECUTABLES) Sample_All
	-@echo ' '

#.PHONY: all clean dependents
#.SECONDARY:

#-include ../makefile.targets
