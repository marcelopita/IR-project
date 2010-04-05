################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../riCode_old/test/testReader.cpp \
../riCode_old/test/testWriter.cpp 

OBJS += \
./riCode_old/test/testReader.o \
./riCode_old/test/testWriter.o 

CPP_DEPS += \
./riCode_old/test/testReader.d \
./riCode_old/test/testWriter.d 


# Each subdirectory must supply rules for building sources it contributes
riCode_old/test/%.o: ../riCode_old/test/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/pita/IR-project/Indexer/riCode_old/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


