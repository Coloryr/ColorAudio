#include "ble.h"
#include "ble_agent.h"
#include "ble_battery.h"
#include "ble_info.h"

#include "../player/sound.h"
#include "../player/player_info.h"

#include "../lvgl/src/misc/lv_log.h"

#include <gio/gio.h>
#include <dbus.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <pthread.h>
#include <regex.h>

#define AVRCP_CMD_REGISTER_NOTIF 0x31
#define AVRCP_EVENT_VOLUME_CHANGED 0x0D
#define AVRCP_CMD_SET_ABSOLUTE_VOL 0x40

GDBusConnection *ble_g_conn;
ble_state ble_now_state;

const char *adapter_path = "/org/bluez/hci0";

static GMainLoop *main_loop = NULL;
static pthread_t tid;

static GMainLoop *loop;

static const char *ble_state_to_string(ble_state state)
{
    switch (state)
    {
    case BLE_STATE_POWER_OFF:
        return "POWER_OFF";
    case BLE_STATE_DISCONNECTED:
        return "DISCONNECTED";
    case BLE_STATE_PAIR:
        return "PAIRING";
    case BLE_STATE_CONNECTED:
        return "CONNECTED";
    case BLE_STATE_UNKNOW:
        return "UNKNOWN";
    default:
        return "INVALID";
    }
}

void ble_log_state_change()
{
    LV_LOG_USER("蓝牙状态：%s", ble_state_to_string(ble_now_state));
}

void ble_set_name(const char *new_name)
{
    ble_set_adapter_property("Alias", g_variant_new_string(new_name));
}

void ble_set_power(bool state)
{
    ble_set_adapter_property("Powered", g_variant_new_boolean(state));
    if (!state)
    {
        ble_now_state = BLE_STATE_POWER_OFF;
        ble_log_state_change();
    }
}

void ble_set_discoverable(bool state)
{
    ble_set_adapter_property("Discoverable", g_variant_new_boolean(state));
}

void ble_set_pairable(bool state)
{
    ble_set_adapter_property("Pairable", g_variant_new_boolean(state));
    if (state)
    {
        ble_set_adapter_property("PairableTimeout", g_variant_new_uint32(0));
    }
    else
    {
        ble_set_adapter_property("PairableTimeout", g_variant_new_uint32(1));
    }
}

void ble_control_media(music_command command)
{
    ble_send_media_command(command);
}

void ble_report_volume()
{
    float vol = alsa_get_volume();
    guint8 avrcp_vol = (guint8)(vol / 100 * 127);

    ble_send_volume(avrcp_vol);
}

void ble_device_add()
{
    ble_set_name("ColorAudio_V1");
    ble_set_power(true);
    ble_set_discoverable(true);
    ble_set_pairable(true);
}

static void *ble_loop_run(void *arg)
{
    ble_agent_init();
    ble_info_init();
    bluez_alsa_start(ble_g_conn);

    ble_device_add();

    LV_LOG_USER("蓝牙服务已启动");

    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    LV_LOG_USER("蓝牙服务正在关闭");

    ble_set_discoverable(false);
    ble_set_pairable(false);
    ble_set_power(false);

    ble_agent_close();
    ble_info_close();

    bluez_alsa_close();
    g_main_loop_unref(main_loop);
    g_object_unref(ble_g_conn);
    ble_g_conn = NULL;
    main_loop = NULL;

    return NULL;
}

void ble_init()
{
    GError *error = NULL;
    ble_g_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    ble_now_state = BLE_STATE_POWER_OFF;

    if (error)
    {
        LV_LOG_ERROR("Error: %s", error->message);
        g_error_free(error);
        return;
    }
}

void ble_stop()
{
    if (main_loop && g_main_loop_is_running(main_loop))
    {
        g_main_loop_quit(main_loop);

        main_loop = NULL;
    }
}

void ble_run()
{
    loop = g_main_loop_new(NULL, FALSE);

    int res = pthread_create(&tid, NULL, ble_loop_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Ble thread run fail: %d", res);
    }
}

void ble_run_loop()
{
    loop = g_main_loop_new(NULL, FALSE);
    ble_loop_run(NULL);
}
