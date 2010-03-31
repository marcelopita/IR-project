################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../riCode_old/src/CollectionReader.cpp \
../riCode_old/src/CollectionWriter.cpp \
../riCode_old/src/Document.cpp 

OBJS += \
./riCode_old/src/CollectionReader.o \
./riCode_old/src/CollectionWriter.o \
./riCode_old/src/Document.o 

CPP_DEPS += \
./riCode_old/src/CollectionReader.d \
./riCode_old/src/CollectionWriter.d \
./riCode_old/src/Document.d 


# Each subdirectory must supply rules for building sources it contributes
riCode_old/src/%.o: ../riCode_old/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/04610922479/IR-project/Indexing/riCode_old/src" -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


