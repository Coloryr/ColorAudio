#ifndef __WIRELESS_H__
#define __WIRELESS_H__

#include <stdbool.h>

#define WIFI_DEVICE_USB "/sys/bus/usb/devices/2-1/remove"

#ifdef BUILD_ARM

#ifdef __cplusplus
extern "C" {
#endif

bool get_wireless_power();
void set_wireless_power(bool enable);
void set_wireless_power_on();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

#endif // __WIRELESS_H__