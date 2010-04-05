################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../riCode_old/CMakeFiles/CompilerIdCXX/CMakeCXXCompilerId.cpp 

OBJS += \
./riCode_old/CMakeFiles/CompilerIdCXX/CMakeCXXCompilerId.o 

CPP_DEPS += \
./riCode_old/CMakeFiles/CompilerIdCXX/CMakeCXXCompilerId.d 


# Each subdirectory must supply rules for building sources it contributes
riCode_old/CMakeFiles/CompilerIdCXX/%.o: ../riCode_old/CMakeFiles/CompilerIdCXX/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/pita/IR-project/Indexer/riCode_old/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


