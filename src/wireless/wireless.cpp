#include "wireless.h"

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#if BUILD_ARM
static const char *wireless_open = "1";
static const char *wireless_close = "0";

bool get_wireless_power()
{
    uint8_t temp[2];
    int fd = open(DEVICE_POWER, O_RDONLY);
    read(fd, temp, 2);
    close(fd);

    return temp[0] == '1';
}

void set_wireless_power(bool enable)
{
    int fd = open(DEVICE_POWER, O_WRONLY);
    write(fd, enable ? wireless_open : wireless_close, 2);
    close(fd);
}

void set_wireless_power_on()
{
    if (!get_wireless_power())
    {
        set_wireless_power(true);
        sleep(5);
    }
}

void set_wireless_power_off()
{
    if (get_wireless_power())
    {
        set_wireless_power(false);
    }
    if (access(DEVICE_USB, F_OK) == 0)
    {
        int fd = open(DEVICE_USB, O_WRONLY);
        write(fd, wireless_open, 2);
        close(fd);
    }
}

#endif