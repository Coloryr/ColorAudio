#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdbool.h>

#define WIFI_DEVICE_POWER "/sys/class/leds/wifi-power/brightness"
#define AMP_DEVICE_POWER "/sys/class/leds/codec-en/brightness"

#define POWER_ON "1"
#define POWER_OFF "0"

#ifdef BUILD_ARM

#ifdef __cplusplus
extern "C" {
#endif

bool get_wireless_power();
void set_wireless_power(bool enable);
void set_amp_power(bool enable);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

#endif // __GPIO_H__