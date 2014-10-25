cmake_minimum_required(VERSION 2.8.4)

set(CMAKE_TOOLCHAIN_FILE arduino-cmake/cmake/ArduinoToolchain.cmake)

project(EriXposure C CXX)

set(ARDUINO_DEFAULT_BOARD mini328)
set(ARDUINO_DEFAULT_PORT /dev/ttyUSB0)

#set(EXECUTABLE_OUTPUT_PATH build)
#set(LIBRARY_OUTPUT_PATH build)

#print_board_list()
#print_programmer_list()

include_directories(arduino-libraries/Adafruit_GFX)
include_directories(arduino-libraries/Adafruit_Sensor)
include_directories(arduino-libraries/Adafruit_SSD1306)
include_directories(arduino-libraries/Adafruit_TSL2591)
link_directories(arduino-libraries)

generate_arduino_firmware( erixposure
   SKETCH erixposure
   PROGRAMMER usbtinyisp
)
