#ifndef __WIRELESS_H__
#define __WIRELESS_H__

#include <stdbool.h>

//echo 1 > /sys/bus/usb/devices/2-1/remove

#define WIFI_DEVICE_USB "/sys/bus/usb/devices/2-1/remove"

#ifdef BUILD_ARM

#ifdef __cplusplus
extern "C" {
#endif

void wireless_delete();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

#endif // __WIRELESS_H__