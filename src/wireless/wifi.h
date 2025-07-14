#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdbool.h>
#include <stdint.h>
#include <string>

#define WIFI_POWER "/sys/class/leds/wifi-power/brightness"
#define WIFI_NAME "wlan0"
#define WIFI_PATH "/var/run/wpa_supplicant/" WIFI_NAME
#define WIFI_RUN "wpa_supplicant -B -i " WIFI_NAME " -c /etc/wpa_supplicant.conf"
#define WIFI_STOP "sudo killall wpa_supplicant"

#define WIFI_SCAN "SCAN"
#define WIFI_SCAN_RESULTS "SCAN_RESULTS"
#define WIFI_REMOVE_ALL_NETWORK "REMOVE_NETWORK all"
#define WIFI_ADD_NETWORK "ADD_NETWORK"
#define WIFI_SET_NETWORK_SSID "SET_NETWORK %d ssid \"%s\""
#define WIFI_SET_NETWORK_PSK "SET_NETWORK %d psk \"%s\""
#define WIFI_ENABLE_NETWORK "ENABLE_NETWORK %d"
#define WIFI_SELECT_NETWORK "SELECT_NETWORK %d"
#define WIFI_STATUS "STATUS"

#define WIFI_RES_OK "OK"

#define WIFI_DISCONNECTED "DISCONNECTED"
#define WIFI_SCANNING "SCANNING"
#define WIFI_COMPLETED "COMPLETED"

typedef enum
{
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_SCANNING,
    WIFI_STATE_COMPLETED,
    WIFI_STATE_UNKNOW = -1
} wifi_state;

typedef struct
{
    std::string ssid;
    std::string bssid;
    std::string wpa;
    int16_t level;
    uint16_t frequency;
} wifi_item_t;

bool get_wifi_power();
void set_wifi_power(bool enable);

bool have_wifi_device();
void wifi_test();

#endif // __WIFI_H__