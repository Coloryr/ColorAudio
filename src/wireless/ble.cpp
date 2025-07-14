#include "ble.h"
#include "ble_agent.h"

#include "../lvgl/src/misc/lv_log.h"

#include <gio/gio.h>
#include <dbus.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <pthread.h>
#include <regex.h>

GDBusConnection *ble_g_conn;
static GMainLoop *main_loop = NULL;

static pthread_t tid;

static const char *adapter_path = "/org/bluez/hci0";

static std::vector<std::string> trusted_devices;

static gint ble_music_id;

static void set_adapter_property(const char *property, GVariant *value)
{
    GError *error = NULL;
    GDBusProxy *proxy = g_dbus_proxy_new_sync(ble_g_conn,
                                              G_DBUS_PROXY_FLAGS_NONE,
                                              NULL,
                                              "org.bluez",
                                              adapter_path,
                                              "org.freedesktop.DBus.Properties",
                                              NULL,
                                              &error);
    if (error)
    {
        LV_LOG_ERROR("Error: %s", error->message);
        g_error_free(error);
        return;
    }

    GVariant *result = g_dbus_proxy_call_sync(proxy, "Set",
                                              g_variant_new("(ssv)", "org.bluez.Adapter1", property, value),
                                              G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (error)
    {
        LV_LOG_ERROR("Error: %s", error->message);
        g_error_free(error);
    }
    else
    {
        g_variant_unref(result);
    }

    g_object_unref(proxy);
}

static void set_device_property(const char *device_path, const char *property, GVariant *value)
{
    GError *error = NULL;
    GDBusProxy *proxy = g_dbus_proxy_new_sync(ble_g_conn,
                                              G_DBUS_PROXY_FLAGS_NONE,
                                              NULL,
                                              "org.bluez",
                                              device_path,
                                              "org.freedesktop.DBus.Properties",
                                              NULL, &error);
    if (error)
    {
        LV_LOG_ERROR("Error: %s", error->message);
        g_error_free(error);
        return;
    }

    GVariant *result = g_dbus_proxy_call_sync(proxy, "Set",
                                              g_variant_new("(ssv)", "org.bluez.Device1", property, value),
                                              G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error)
    {

        LV_LOG_ERROR("Error: %s", error->message);
        g_error_free(error);
    }
    else
    {
        g_variant_unref(result);
    }

    g_object_unref(proxy);
}

// static void get_player_list(const char *path)
// {
//     GError *error = NULL;
//     GDBusProxy *proxy = g_dbus_proxy_new_sync(ble_g_conn, G_DBUS_PROXY_FLAGS_NONE,
//                                               NULL, "org.bluez", path,
//                                               "org.freedesktop.DBus.Properties",
//                                               NULL, &error);

//     if (error)
//     {
//         LV_LOG_ERROR("Error: %s", error->message);
//         g_error_free(error);
//         return;
//     }

//     GVariant *result = g_dbus_proxy_call_sync(proxy, "Get",
//                                               g_variant_new("(ss)", "org.bluez.MediaPlaylist1", "NowPlaying"),
//                                               G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

//     if (error)
//     {
//         LV_LOG_ERROR("Error: %s", error->message);
//         g_error_free(error);
//     }
//     else
//     {
//         GVariant *playlist = g_variant_get_child_value(result, 0);
//         gchar **items;
//         g_variant_get(playlist, "a(sos)", &items);
//         for (int i = 0; items[i] != NULL; i++)
//         {
//             g_print("播放项 %d: %s\n", i, items[i]);
//         }
//         g_variant_unref(result);
//     }
//     g_object_unref(proxy);
// }

static void on_music_properties_changed(GDBusConnection *connection,
                                        const gchar *sender,
                                        const gchar *path,
                                        const gchar *interface,
                                        const gchar *signal_name,
                                        GVariant *parameters,
                                        gpointer user_data)
{
    const gchar *iface_name;
    GVariant *changed_props;
    g_variant_get(parameters, "(&s@a{sv}@as)", &iface_name, &changed_props, NULL);

    // 解析变化的属性
    GVariantIter iter;
    const gchar *key;
    GVariant *value;
    g_variant_iter_init(&iter, changed_props);

    while (g_variant_iter_next(&iter, "{&sv}", &key, &value))
    {
        gchar *val_str = g_variant_print(value, TRUE);
        LV_LOG_USER("[%s] 属性变化: %s = %s", path, key, val_str);
        if (strstr(key, "Playlist"))
        {
            // gsize size;
            // const gchar *path1 = g_variant_get_string(value, &size);
            // get_player_list(path);
        }
        else if (strstr(key, "Track"))
        {
        }
        g_free(val_str);
        g_variant_unref(value);
    }
    g_variant_unref(changed_props);
}

static void on_property_changed(GDBusConnection *connection,
                                const gchar *sender,
                                const gchar *object_path,
                                const gchar *interface_name,
                                const gchar *signal_name,
                                GVariant *parameters,
                                gpointer user_data)
{

    if (!g_str_has_prefix(object_path, "/org/bluez/hci0/dev_"))
        return;

    const gchar *interface;
    GVariantIter *changed_props;
    g_variant_get(parameters, "(&sa{sv}as)", &interface, &changed_props, NULL);

    if (g_str_equal(interface, "org.bluez.Device1"))
    {
        GVariant *value;
        const gchar *key;
        while (g_variant_iter_next(changed_props, "{&sv}", &key, &value))
        {
            if (g_str_equal(key, "Connected"))
            {
                gboolean connected = g_variant_get_boolean(value);
                LV_LOG_USER("设备 %s: %s", object_path, connected ? "已连接" : "已断开");
                char temp[256];
                sprintf(temp, "%s/player0", object_path);
                if (connected)
                {
                    ble_music_id = g_dbus_connection_signal_subscribe(
                        ble_g_conn,
                        "org.bluez",
                        "org.freedesktop.DBus.Properties",
                        "PropertiesChanged",
                        temp,
                        NULL,
                        G_DBUS_SIGNAL_FLAGS_NONE,
                        on_music_properties_changed,
                        NULL,
                        NULL);
                }
                else
                {
                    if (ble_music_id != 0)
                    {
                        g_dbus_connection_signal_unsubscribe(ble_g_conn, ble_music_id);
                        ble_music_id = 0;
                    }
                }
            }

            g_variant_unref(value);
        }
    }
    g_variant_iter_free(changed_props);
}

void set_bluetooth_name(const char *new_name)
{
    set_adapter_property("Alias", g_variant_new_string(new_name));
}

void set_power(bool state)
{
    set_adapter_property("Powered", g_variant_new_boolean(state));
}

void set_discoverable(bool state)
{
    set_adapter_property("Discoverable", g_variant_new_boolean(state));
}

void set_pairable(bool state)
{
    set_adapter_property("Pairable", g_variant_new_boolean(state));
    if (state)
    {
        set_adapter_property("PairableTimeout", g_variant_new_uint32(0));
    }
    else
    {
        set_adapter_property("PairableTimeout", g_variant_new_uint32(1));
    }
}

void *ble_run(void *arg)
{
    main_loop = g_main_loop_new(NULL, FALSE);
    LV_LOG_USER("蓝牙服务已启动，等待设备配对和连接...");
    g_main_loop_run(main_loop);

    ble_agent_close();

    g_main_loop_unref(main_loop);
    g_object_unref(ble_g_conn);
    return NULL;
}

void ble_stop()
{
    set_discoverable(false);
    set_power(false);

    if (main_loop && g_main_loop_is_running(main_loop))
    {
        g_main_loop_quit(main_loop);
    }
}

void ble_init()
{
    GError *error = NULL;
    ble_g_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (error)
    {
        g_error_free(error);
        LV_LOG_ERROR("Error: %s", error->message);
        return;
    }

    const char *adapter_path = "/org/bluez/hci0";

    g_dbus_connection_signal_subscribe(ble_g_conn,
                                       "org.bluez",
                                       "org.freedesktop.DBus.Properties",
                                       "PropertiesChanged",
                                       NULL, NULL,
                                       G_DBUS_SIGNAL_FLAGS_NONE,
                                       on_property_changed,
                                       NULL, NULL);

    ble_agent_init();

    set_bluetooth_name("coloraudio");

    set_power(true);
    set_discoverable(true);
    set_pairable(true);

    int res = pthread_create(&tid, NULL, ble_run, NULL);
    if (res)
    {
        LV_LOG_ERROR("Music play thread run fail: %d", res);
    }
}