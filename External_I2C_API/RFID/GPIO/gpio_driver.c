#include <stdio.h>
#include <linux/gpio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "gpio_driver.h"

#define GPIO_DEVICE "/dev/gpiochip0"
#define DEBUG

#ifdef DEBUG
#define ERR(...) do { fprintf(stderr, "Error in (%s), at line (%d) : %s\n", __FILE__, __LINE__, __VA_ARGS__);}while(0)
#define DBG(...) do { fprintf(stderr, __VA_ARGS__);}while(0)
#else
#define ERR(...)
#define DBG(str, ...)
#endif /*DEBUG*/

#define GPIO_MASK (1 << GPIO_PIN)

static int gpio_system_fd;
static struct gpio_v2_line_request request;

static struct gpio_v2_line_config set_gpio_config(void)
{
    struct gpio_v2_line_config config;
    memset(config.padding, 0x00, sizeof(config.padding));
    config.num_attrs = 0;
    config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    memset(config.attrs, 0x00, sizeof(config.attrs));

    return config;
}

static state_t gpio_line_request(void)
{
    int ioctl_result;
    state_t result;

    request.config = set_gpio_config();
    request.offsets[0] = GPIO_PIN;
    strcpy(request.consumer, "GPIO 5");
    request.num_lines = 1;


    ioctl_result = ioctl(gpio_system_fd, GPIO_V2_GET_LINE_IOCTL, &request);
    close(gpio_system_fd);
    if(ioctl_result < 0)
    {
        ERR(strerror(errno));
        result = STATE_NOK;
    }
    else
    {
        if(request.fd <=0)
        {
            ERR("Could not obtain file descriptor for access");
            result = STATE_NOK;
        }
        else
        {
            DBG("GPIO (%d) access granted.....\n", GPIO_PIN);
            result = STATE_OK;
        }
        
    }
    return result;
}

state_t gpio_init(void)
{
    state_t result;

    gpio_system_fd = open(GPIO_DEVICE, O_RDONLY);
    if(gpio_system_fd < 0)
    {
        ERR("Could not open file");
        result = STATE_NOK;
    }
    else
    {
        DBG("File Opened successfully....\n");
        result = gpio_line_request();
    }
    return result;
}

state_t gpio_set_output(const GPIO_state_t gpio_state)
{
    state_t result = STATE_OK;
    int ioctl_result;
    struct gpio_v2_line_values gpio_values_config;

    gpio_values_config.mask = 1;
    gpio_values_config.bits = gpio_state;
    

    ioctl_result = ioctl(request.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &gpio_values_config);

    if(ioctl_result < 0)
    {
        ERR(strerror(errno));
        result = STATE_NOK;
    }
    else
    {
        DBG("GPIO (%d) set to (%d)\n", GPIO_PIN, gpio_state);
        result = STATE_OK;
    }

    return result;
}

state_t gpio_DeInit(void)
{
    state_t result = STATE_OK;
    int sys_call_result;

    sys_call_result = close(request.fd);
    if(sys_call_result < 0)
    {
        ERR(strerror(errno));
        result = STATE_NOK;
    }
    else
    {
        result = STATE_OK; 
    }

    return result;
}

/*   Example usage
int main(void)
{
    int i = 3;

    (void)gpio_init();
    while(i>0)
    {
        gpio_set_output(HIGH);
        sleep(1);
        gpio_set_output(LOW);
        sleep(1);
        i--;
    }

    gpio_DeInit();
    return 0;
}
*/