################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../riCode_old/src/old/CollectionReader.cpp 

OBJS += \
./riCode_old/src/old/CollectionReader.o 

CPP_DEPS += \
./riCode_old/src/old/CollectionReader.d 


# Each subdirectory must supply rules for building sources it contributes
riCode_old/src/old/%.o: ../riCode_old/src/old/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/pita/IR-project/Indexer/riCode_old/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


