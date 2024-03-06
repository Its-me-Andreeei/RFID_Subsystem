add_library(RFID_Manager STATIC)

add_subdirectory(GPIO)
add_subdirectory(I2C)

target_link_libraries(RFID_Manager GPIO_Driver I2C_Driver)

set(SOURCES
    RFID_Manager.c
    RFID_Manager.h
    crc.c
    crc.h
)

add_library(RFID_Manager STATIC ${SOURCES})