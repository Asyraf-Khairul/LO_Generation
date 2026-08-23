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

if (WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(PICO_BOARD pico CACHE STRING "Board type")
set(PICO_SDK_FETCH_FROM_GIT ON)
set(PICO_SDK_FETCH_FROM_GIT_TAG "2.2.0")
set(PICO_SDK_FETCH_FROM_GIT_PATH "${CMAKE_BINARY_DIR}/pico-sdk")

include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)

project(standalone_quadrature_lo C CXX)

pico_sdk_init()

add_executable(standalone_quadrature_lo
    main.cpp
    ssd1306.c
)

target_compile_features(standalone_quadrature_lo PRIVATE c_std_11)

pico_generate_pio_header(standalone_quadrature_lo ${CMAKE_CURRENT_LIST_DIR}/nco.pio)
pico_generate_pio_header(standalone_quadrature_lo ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)

target_include_directories(standalone_quadrature_lo PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(standalone_quadrature_lo
    pico_stdlib
    hardware_pio
    hardware_i2c
    hardware_clocks
)

pico_enable_stdio_usb(standalone_quadrature_lo 1)
pico_enable_stdio_uart(standalone_quadrature_lo 0)
pico_add_extra_outputs(standalone_quadrature_lo)
