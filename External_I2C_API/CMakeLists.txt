cmake_minimum_required(VERSION 3.25)
project(Raspberry_RFID_I2C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)


set(SOURCES ExampleMain.c)
add_executable(Raspberry_RFID_I2C  ${SOURCES})

add_subdirectory(RFID)
target_link_libraries(Raspberry_RFID_I2C RFID_Manager)

