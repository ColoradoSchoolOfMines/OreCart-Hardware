cmake_minimum_required(VERSION 3.13)

set(FREERTOS_KERNEL_PATH ${CMAKE_SOURCE_DIR}/kernel/FreeRTOS-Kernel)

if (DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    message("Using PICO_SDK_PATH from environment ('${PICO_SDK_PATH}')")
endif ()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "SDK location was not specified. Please set PICO_SDK_PATH env variable.")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(app C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(orecart_hw main.c)

target_include_directories(orecart_hw PRIVATE ${CMAKE_CURRENT_LIST_DIR}/kernel)
 
target_link_libraries(orecart_hw pico_stdlib FreeRTOS-Kernel FreeRTOS-Kernel-Heap4)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(orecart_hw)