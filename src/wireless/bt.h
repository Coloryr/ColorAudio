#ifndef __BT_H__
#define __BT_H__

#include <gio/gio.h>

typedef enum
{
    BT_STATE_POWER_OFF = 0,
    BT_STATE_DISCONNECTED,
    BT_STATE_PAIR,
    BT_STATE_CONNECTED,
    BT_STATE_STOP,
    BT_STATE_UNKNOW = -1
} bt_state;

extern GDBusConnection *bt_g_conn;
extern const char *adapter_path;
extern bt_state bt_now_state;
extern bool is_playing;

void bt_log_state_change();
void bt_init();
void bt_stop();
void bt_run_loop();

void bt_set_name(const char *new_name);
void bt_set_power(bool state);
void bt_set_discoverable(bool state);
void bt_set_pairable(bool state);

#endif // __BLE_H__
