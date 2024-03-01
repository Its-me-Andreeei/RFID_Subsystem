#ifndef __GPIO_DRIVER_1_H
#define __GPIO_DRIVER_1_H

#define GPIO_PIN 5

typedef enum state_t
{
    STATE_OK,
    STATE_PENDING,
    STATE_NOK
}state_t;

typedef enum GPIO_state_t
{
    LOW = 0U,
    HIGH = 1U
}GPIO_state_t;

state_t gpio_init(void);
state_t gpio_set_output(const GPIO_state_t gpio_state);
state_t gpio_DeInit(void);
#endif /*__GPIO_DRIVER_H*/
