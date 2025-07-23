#ifndef __WIRELESS_H__
#define __WIRELESS_H__

#define DEVICE_POWER "/sys/class/leds/wifi-power/brightness"
#define DEVICE_USB "/sys/bus/usb/devices/2-1/remove"

bool get_wireless_power();
void set_wireless_power(bool enable);
void set_wireless_power_on();

#endif // __WIRELESS_H__