# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================




cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico CACHE STRING "Board type")

include(pico_sdk_import.cmake)
project(task09-alert C CXX ASM)
pico_sdk_init()
add_executable(task09-alert task09-alert.c lib/ssd1306.c)
pico_set_program_name(task09-alert "task09-alert")
pico_set_program_version(task09-alert "0.1")
pico_enable_stdio_uart(task09-alert 0)
pico_enable_stdio_usb(task09-alert 1)
target_link_libraries(task09-alert pico_stdlib hardware_i2c hardware_adc hardware_pwm pico_bootrom hardware_irq pico_time) 
target_include_directories(task09-alert PRIVATE   ${CMAKE_CURRENT_LIST_DIR})
pico_add_extra_outputs(task09-alert)

