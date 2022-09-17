cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(GP2040-CE C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(${PROJECT_NAME} 
src/main.cpp
src/gp2040.cpp
src/gp2040aux.cpp
src/gamepad.cpp
src/configmanager.cpp
src/storagemanager.cpp
lib/FlashPROM/src/FlashPROM.cpp
MPG/src/GamepadDebouncer.cpp
MPG/src/MPG.cpp
MPG/src/GamepadDescriptors.cpp
lib/CRC32/src/CRC32.cpp
${PICO_SDK_PATH}/lib/tinyusb/src/class/net/ecm_rndis_device.c
${PICO_SDK_PATH}/lib/tinyusb/src/class/net/ncm_device.c
${PICO_SDK_PATH}/lib/tinyusb/src/class/hid/hid_device.c
${PICO_SDK_PATH}/lib/tinyusb/src/device/usbd.c
${PICO_SDK_PATH}/lib/tinyusb/src/device/usbd_control.c
${PICO_SDK_PATH}/lib/tinyusb/src/tusb.c
lib/TinyUSB_Gamepad/src/tusb_driver.cpp
lib/TinyUSB_Gamepad/src/usb_descriptors.cpp
lib/TinyUSB_Gamepad/src/net_driver.cpp
lib/TinyUSB_Gamepad/src/hid_driver.cpp
lib/TinyUSB_Gamepad/src/xinput_driver.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
src
src/addons
headers
headers/addons
configs/Pico
MPG/src
MPG/src/descriptors
lib/OneBitDisplay
lib/BitBang_I2C
lib/AnimationStation/src
lib/AnimationStation/src/Effects
lib/NeoPico/src
lib/PlayerLEDs/include
lib/PlayerLEDs/src
lib/TinyUSB_Gamepad/include
lib/TinyUSB_Gamepad/src
lib/FlashPROM/include
lib/FlashPROM/src
${PICO_SDK_PATH}/lib/tinyusb/lib/networking
lib/CRC32/src
${PICO_SDK_PATH}/lib/tinyusb/src/device/
${PICO_SDK_PATH}/lib/tinyusb/src/class/hid/
${PICO_SDK_PATH}/lib/tinyusb/src/class/net/
)

target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    pico_multicore
    hardware_i2c
    hardware_spi
    hardware_pio
    tinyusb_device
    tinyusb_board
    hardware_flash
    )

pico_add_extra_outputs(${PROJECT_NAME})

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

        install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)