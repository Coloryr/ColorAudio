#include <stdio.h>
#include <vector>
#include <string>
#include <regex.h>
#include <stdlib.h>

#include <gio/gio.h>
#include "lvgl/src/misc/lv_log.h"

#include "bt_agent.h"
#include "bt_transport.h"
#include "bt_info.h"
#include "sound/sound.h"

#include "bt.h"

GDBusConnection *bt_g_conn;
bt_state bt_now_state = BT_STATE_UNKNOW;
bool is_playing = false;

const char *adapter_path = "/org/bluez/hci0";

static GMainLoop *main_loop = NULL;

static const char *bt_state_to_string(bt_state state)
{
    switch (state)
    {
    case BT_STATE_POWER_OFF:
        return "POWER_OFF";
    case BT_STATE_DISCONNECTED:
        return "DISCONNECTED";
    case BT_STATE_PAIR:
        return "PAIRING";
    case BT_STATE_CONNECTED:
        return "CONNECTED";
    case BT_STATE_UNKNOW:
        return "UNKNOWN";
    default:
        return "INVALID";
    }
}

void bt_log_state_change()
{
    LV_LOG_USER("蓝牙状态：%s", bt_state_to_string(bt_now_state));
}

void bt_set_name(const char *new_name)
{
    char temp[256];
    sprintf(temp, "hciconfig hci0 name '%s'", new_name);
    std::system(temp);
    bt_set_adapter_property("Alias", g_variant_new_string(new_name));
}

void bt_set_power(bool state)
{
    bt_set_adapter_property("Powered", g_variant_new_boolean(state));
    if (!state)
    {
        bt_now_state = BT_STATE_POWER_OFF;
        bt_log_state_change();
    }
}

void bt_set_discoverable(bool state)
{
    bt_set_adapter_property("Discoverable", g_variant_new_boolean(state));
}

void bt_set_pairable(bool state)
{
    bt_set_adapter_property("Pairable", g_variant_new_boolean(state));
    if (state)
    {
        bt_set_adapter_property("PairableTimeout", g_variant_new_uint32(0));
    }
    else
    {
        bt_set_adapter_property("PairableTimeout", g_variant_new_uint32(1));
    }
}

void bt_device_add()
{
    bt_set_name("ColorAudio");
    bt_set_power(true);
    bt_set_discoverable(true);
    bt_set_pairable(true);
}

void bt_init()
{
    GError *error = NULL;
    bt_g_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (error)
    {
        LV_LOG_ERROR("Error: %s", error->message);
        g_error_free(error);
        return;
    }
    bt_now_state = BT_STATE_POWER_OFF;
}

void bt_stop()
{
    if (main_loop && g_main_loop_is_running(main_loop))
    {
        g_main_loop_quit(main_loop);

        main_loop = NULL;
    }
    bt_now_state = BT_STATE_STOP;
}

void bt_run_loop()
{
    main_loop = g_main_loop_new(NULL, FALSE);

    LV_LOG_USER("蓝牙服务正在启动");

    bt_agent_init();
    bt_info_init();
    bluez_alsa_start(bt_g_conn);

    bt_device_add();

    LV_LOG_USER("蓝牙服务已启动");

    g_main_loop_run(main_loop);

    LV_LOG_USER("蓝牙服务正在关闭");

    bt_set_discoverable(false);
    bt_set_pairable(false);
    bt_set_power(false);

    bt_agent_close();
    bt_info_close();

    bluez_alsa_close();
    g_main_loop_unref(main_loop);
    g_object_unref(bt_g_conn);
    bt_g_conn = NULL;
    main_loop = NULL;
}
