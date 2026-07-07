# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pi_piano_lightbar C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

set(SOURCES
    src/main.cpp
    src/led/ws2812.cpp
    src/led/led_controller.cpp
    src/led/led_animator.cpp
    src/midi/midi_parser.cpp
    src/usb/usb_midi_device.cpp
    src/system/bootsel_button.c
    usb_descriptors.c
)

if(CFG_TUH_ENABLED)
    list(APPEND SOURCES src/usb/usb_midi_host.cpp)
endif()

add_executable(pi_piano_lightbar ${SOURCES})

pico_set_program_name(pi_piano_lightbar "pi_piano_lightbar")
pico_set_program_version(pi_piano_lightbar "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pi_piano_lightbar 1)
pico_enable_stdio_usb(pi_piano_lightbar 0)

# Add the standard library to the build
target_link_libraries(pi_piano_lightbar
        pico_stdlib
        hardware_pio
        hardware_clocks
        tinyusb_device
)

if(CFG_TUH_ENABLED)
    target_link_libraries(pi_piano_lightbar tinyusb_host)
endif()

# Add the standard include files to the build
target_include_directories(pi_piano_lightbar PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src
)

pico_add_extra_outputs(pi_piano_lightbar)