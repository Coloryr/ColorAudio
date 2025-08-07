#include "gpio.h"

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef BUILD_ARM

bool get_wireless_power()
{
    uint8_t temp[2];
    int fd = open(WIFI_DEVICE_POWER, O_RDONLY);
    read(fd, temp, 2);
    close(fd);

    return temp[0] == '1';
}

void set_wireless_power(bool enable)
{
    int fd = open(WIFI_DEVICE_POWER, O_WRONLY);
    if(enable)
    {
        write(fd, POWER_ON, 2);
    }
    else
    {
        write(fd, POWER_OFF, 2);
    }
    close(fd);
}

void set_amp_power(bool enable)
{
    int fd = open(AMP_DEVICE_POWER, O_WRONLY);
    if(enable)
    {
        write(fd, POWER_ON, 2);
    }
    else
    {
        write(fd, POWER_OFF, 2);
    }
    close(fd);
}

#endif