#ifndef __BLE_H__
#define __BLE_H__

#include <gio/gio.h>

#include "../player/player_info.h"

typedef enum
{
    BLE_STATE_POWER_OFF = 0,
    BLE_STATE_DISCONNECTED,
    BLE_STATE_PAIR,
    BLE_STATE_CONNECTED,
    BLE_STATE_UNKNOW = -1
} ble_state;

#ifdef __cplusplus
extern "C"
{
#endif

void bluez_alsa_start(GDBusConnection *conn);
void bluez_alsa_close();
void ble_send_volume();
void ble_send_battery();
void ble_later_send_volume();

#ifdef __cplusplus
} /*extern "C"*/
#endif

extern GDBusConnection *ble_g_conn;
extern const char *adapter_path;
extern ble_state ble_now_state;

void ble_log_state_change();
void ble_init();
void ble_run();
void ble_run_loop();

void ble_set_name(const char *new_name);
void ble_set_power(bool state);
void ble_set_discoverable(bool state);
void ble_set_pairable(bool state);
void ble_control_media(music_command command);

#endif // __BLE_H__
