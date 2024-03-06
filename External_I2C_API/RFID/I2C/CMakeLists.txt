set(SOURCES i2c_driver.c 
    i2c_driver.h 
)

add_library(GPIO_Driver STATIC ${SOURCES})