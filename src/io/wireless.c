#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#include "wireless.h"
#include "gpio.h"

#if BUILD_ARM

void set_wireless_power_on()
{
    if (!get_wireless_power())
    {
        set_wireless_power(true);
        sleep(10);
    }
}

void set_wireless_power_off()
{
    if (get_wireless_power())
    {
        set_wireless_power(false);
    }
    if (access(WIFI_DEVICE_USB, F_OK) == 0)
    {
        int fd = open(WIFI_DEVICE_USB, O_WRONLY);
        write(fd, POWER_ON, 2);
        close(fd);
    }
}

#endif