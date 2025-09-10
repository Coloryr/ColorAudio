#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#include "wireless.h"
#include "gpio.h"

#if BUILD_ARM

void wireless_delete()
{
    if (access(WIFI_DEVICE_USB, F_OK) == 0)
    {
        int fd = open(WIFI_DEVICE_USB, O_WRONLY);
        write(fd, POWER_ON, 2);
        close(fd);
    }
}

#endif