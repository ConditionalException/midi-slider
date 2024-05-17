cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(test_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(midi-slider
  midi-slider.c
  usb_descriptors.c
)

pico_enable_stdio_usb(midi-slider 0)
pico_enable_stdio_uart(midi-slider 1)

target_include_directories(midi-slider PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_add_extra_outputs(midi-slider)
target_link_libraries(midi-slider PRIVATE pico_stdlib tinyusb_device tinyusb_board hardware_adc)
