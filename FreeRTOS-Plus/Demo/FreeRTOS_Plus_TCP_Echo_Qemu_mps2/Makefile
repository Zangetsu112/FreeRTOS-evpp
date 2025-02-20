CXX = arm-none-eabi-g++
CC = arm-none-eabi-gcc
SIZE = arm-none-eabi-size
BIN := freertos_tcp_mps2_demo.axf

BUILD_DIR := build

FREERTOS_DIR_REL := ../../../FreeRTOS
FREERTOS_DIR := $(abspath $(FREERTOS_DIR_REL))
KERNEL_DIR := $(FREERTOS_DIR)/Source

FREERTOS_PLUS_DIR_REL := ../../../FreeRTOS-Plus
FREERTOS_PLUS_DIR := $(abspath $(FREERTOS_PLUS_DIR_REL))

INCLUDE_DIRS += -I.

FREERTOS_TCP = ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP

# Demo Source Files
# SOURCE_FILES += EventLoop.cpp
SOURCE_FILES += startup.c
SOURCE_FILES += syscalls.c
# SOURCE_FILES += main.c
# SOURCE_FILES += main_networking.c
SOURCE_FILES += TCPServer_SingleTasks.c


INCLUDE_DIRS += -ICMSIS

# FreeRTOS Kernel
INCLUDE_DIRS += -I$(KERNEL_DIR)/include

SOURCE_FILES += $(KERNEL_DIR)/tasks.c
SOURCE_FILES += $(KERNEL_DIR)/list.c
SOURCE_FILES += $(KERNEL_DIR)/queue.c
SOURCE_FILES += $(KERNEL_DIR)/event_groups.c

# FreeRTOS Kernel ARM Cortex-M3 Port
INCLUDE_DIRS += -I$(KERNEL_DIR)/portable/GCC/ARM_CM3

SOURCE_FILES += $(KERNEL_DIR)/portable/GCC/ARM_CM3/port.c
SOURCE_FILES += ${KERNEL_DIR}/portable/MemMang/heap_3.c

# FreeRTOS+TCP
INCLUDE_DIRS += -I${FREERTOS_TCP}/source/include/
INCLUDE_DIRS += -I${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/

SOURCE_FILES += $(wildcard ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-TCP/source/*.c )

# FreeRTOS+TCP Port for ARM MPS2 SoC
INCLUDE_DIRS += -I${FREERTOS_TCP}/source/portable/NetworkInterface/MPS2_AN385/ether_lan9118
INCLUDE_DIRS += -I${FREERTOS_TCP}/source/portable/Compiler/GCC

SOURCE_FILES += ${FREERTOS_TCP}/source/portable/BufferManagement/BufferAllocation_2.c
SOURCE_FILES += ${FREERTOS_TCP}/source/portable/NetworkInterface/MPS2_AN385/NetworkInterface.c
SOURCE_FILES += ${FREERTOS_TCP}/source/portable/NetworkInterface/MPS2_AN385/ether_lan9118/smsc9220_eth_drv.c
# SOURCE_FILES += $(wildcard ./evpp/*.cpp)

DEFINES := -DHEAP3
CPPFLAGS += $(DEFINES)

CFLAGS += -mthumb -mcpu=cortex-m3
ifeq ($(DEBUG), 1)
    CFLAGS += -g3 -O0 -ffunction-sections -fdata-sections
else
    CFLAGS += -Os -ffunction-sections -fdata-sections
endif
CFLAGS += -MMD
CFLAGS += -Wall -Wextra -Wshadow # -fpermissive
#CFLAGS += -Wpedantic -fanalyzer
#CFLAGS += -flto
CFLAGS += $(INCLUDE_DIRS)
# CXXFLAGS := $(CFLAGS)

LDFLAGS = -T mps2_m3.ld
LDFLAGS += -Xlinker -Map=${BUILD_DIR}/output.map
LDFLAGS += -Xlinker --gc-sections
LDFLAGS += -nostartfiles -specs=nano.specs -specs=nosys.specs -specs=rdimon.specs -lsupc++
# LDFLAGS += -nostartfiles -specs=nano.specs -specs=nosys.specs -specs=rdimon.specs 


OBJ_FILES := $(SOURCE_FILES:%.c=$(BUILD_DIR)/%.o)
OBJ_FILES += main.o
OBJ_FILES += main_networking.o

PRECOMPILED_OBJ_FILES := ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/EventLoop.o \
    ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/inner_pre.o \
    ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/example_tcp_echo.o \
    ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/event_loop_thread_pool.o \
    ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/event_loop_thread.o \
    ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/evpp_buffer.o \
    ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/fd_channel.o \
    ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/listener.o \
    ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/tcp_conn.o \
    ${FREERTOS_PLUS_DIR}/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/evpp/tcp_server.o

.PHONY: clean

$(BUILD_DIR)/$(BIN) : $(OBJ_FILES) 
	$(CXX) $(CFLAGS) $(LDFLAGS) $+ $(PRECOMPILED_OBJ_FILES) -o $(@)
	$(SIZE) $(@)

%.d: %.c
	@set -e; rm -f $@; \
	$(CXX) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

INCLUDES := $(SOURCE_FILES:%.c=$(BUILD_DIR)/%.d)
-include $(INCLUDES)

# ${BUILD_DIR}/main%.o: main%.c Makefile
# 	-mkdir -p $(@D)
# 	$(CXX) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

${BUILD_DIR}/%.o : %.c Makefile
	-mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# ${BUILD_DIR}/%.o : %.cpp Makefile
# 	-mkdir -p $(@D)
# 	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# arm-none-eabi-g++ -mthumb -mcpu=cortex-m3 -Os -ffunction-sections -fdata-sections -MMD -Wall -Wextra -Wshadow -I. -ICMSIS -I/home/space/FreeRTOS/FreeRTOS/Source/include -I/home/space/FreeRTOS/FreeRTOS/Source/portable/GCC/ARM_CM3 -I/home/space/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source/include/ -I/home/space/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source/portable/NetworkInterface/MPS2_AN385/ether_lan9118 -I/home/space/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-TCP/source/portable/Compiler/GCC -I/home/space/FreeRTOS/FreeRTOS-Plus/Demo/FreeRTOS_Plus_TCP_Echo_Qemu_mps2/ -DHEAP3 -c example_tcp_echo.cpp -o example_tcp_echo.o

clean:
	-rm -rf build

