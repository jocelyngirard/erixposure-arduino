set(CMAKE_TOOLCHAIN_FILE arduino-cmake/cmake/ArduinoToolchain.cmake)

cmake_minimum_required(VERSION 2.8)

project(ArduinoExample C CXX)

set(ARDUINO_DEFAULT_BOARD mini328)
set(ARDUINO_DEFAULT_PORT /dev/ttyUSB0)

#print_board_list()
#print_programmer_list()

link_directories(arduino-libraries)

generate_arduino_firmware( blink_bundled
   SKETCH erixposure
   PROGRAMMER usbtinyisp
)
