#include <string>

#include <stdio.h>
#include <gio/gio.h>
#include "lvgl/src/misc/lv_log.h"

#include "ble.h"
#include "ui/ble_view.h"

#include "ble_info.h"

static gint property_id;
static gint music_id;
static gint iface_added_id;

static char *device_path;
static char *device_player_path;

std::string ble_device;
std::string ble_title;
std::string ble_artist;
std::string ble_album;
uint32_t ble_duration;
uint32_t ble_position;

static void set_device_path(const char *path)
{
    if (device_path)
    {
        free(device_path);
    }

    if (device_player_path)
    {
        free(device_player_path);
    }

    char temp[256];
    sprintf(temp, "%s/player0", path);

    device_path = strdup(path);
    device_player_path = strdup(temp);
}

static void clear_device_path()
{
    if (device_path)
    {
        free(device_path);
        device_path = NULL;
    }

    if (device_player_path)
    {
        free(device_player_path);
        device_player_path = NULL;
    }
}

static void on_music_properties_changed(
    GDBusConnection *connection,
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

    GVariantIter iter;
    const gchar *key;
    GVariant *value;
    g_variant_iter_init(&iter, changed_props);

    while (g_variant_iter_next(&iter, "{&sv}", &key, &value))
    {
        if (g_str_equal(key, "Status"))
        {
            const gchar *status = g_variant_get_string(value, NULL);
            LV_LOG_USER("播放状态: %s", status);
        }
        else if (g_str_equal(key, "Track"))
        {
            const gchar *title = NULL, *artist = NULL, *album = NULL;
            guint32 duration = 0;

            if (g_variant_lookup(value, "Title", "&s", &title))
            {
                ble_title = title;
            }
            if (g_variant_lookup(value, "Artist", "&s", &artist))
            {
                ble_artist = artist;
            }
            if (g_variant_lookup(value, "Album", "&s", &album))
            {
                ble_album = album;
            }
            if (g_variant_lookup(value, "Duration", "u", &duration))
            {
                ble_duration = duration;
            }

            view_ble_update_info();
        }
        else if (g_str_equal(key, "Position"))
        {
            ble_position = g_variant_get_uint32(value);

            view_ble_update_info();
        }
        g_variant_unref(value);
    }

    if (changed_props)
    {
        g_variant_unref(changed_props);
    }
}

static void ble_get_name(const char *path)
{
    GError *error = NULL;
    GVariant *result;
    GDBusProxy *proxy = g_dbus_proxy_new_sync(
        ble_g_conn,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.bluez",
        path,
        "org.freedesktop.DBus.Properties",
        NULL,
        &error);
    if (error)
    {
        LV_LOG_ERROR("Error creating DBus proxy: %s", error->message);
        g_error_free(error);
        goto fail;
    }

    result = g_dbus_proxy_call_sync(
        proxy, "Get",
        g_variant_new("(ss)", "org.bluez.Device1", "Alias"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (error)
    {
        LV_LOG_ERROR("Error getting device name: %s", error->message);
        g_error_free(error);
        if (result)
        {
            g_variant_unref(result);
        }
    }
    else if (result)
    {
        GVariant *value;
        g_variant_get(result, "(v)", &value);
        const gchar *name = g_variant_get_string(value, NULL);
        if (name)
        {
            ble_device = name;
            LV_LOG_USER("Got device name: %s", name);
            view_ble_update_info();
        }
        g_variant_unref(value);
        g_variant_unref(result);
    }
fail:
    if (proxy)
    {
        g_object_unref(proxy);
    }
}

static void on_property_changed(
    GDBusConnection *connection,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data)
{
    if (!g_str_has_prefix(object_path, "/org/bluez/hci0/dev_"))
    {
        return;
    }

    const gchar *interface;
    GVariantIter *changed_props;
    g_variant_get(parameters, "(&sa{sv}as)", &interface, &changed_props, NULL);

    if (g_str_equal(interface, "org.bluez.MediaControl1"))
    {
        GVariant *value;
        const gchar *key;
        while (g_variant_iter_next(changed_props, "{&sv}", &key, &value))
        {
            if (g_str_equal(key, "Connected"))
            {
                gboolean connected = g_variant_get_boolean(value);
                LV_LOG_USER("设备 %s: %s", object_path, connected ? "已连接" : "已断开");

                if (connected)
                {
                    ble_now_state = BLE_STATE_CONNECTED;
                    ble_log_state_change();
                    set_device_path(object_path);
                    ble_set_discoverable(false);
                    ble_set_pairable(false);
                    music_id = g_dbus_connection_signal_subscribe(
                        ble_g_conn,
                        "org.bluez",
                        "org.freedesktop.DBus.Properties",
                        "PropertiesChanged",
                        device_player_path,
                        "org.bluez.MediaPlayer1",
                        G_DBUS_SIGNAL_FLAGS_NONE,
                        on_music_properties_changed,
                        NULL,
                        NULL);
                    LV_LOG_USER("register on_music_properties_changed: %d", music_id);
                    ble_get_name(object_path);
                }
                else
                {
                    ble_now_state = BLE_STATE_DISCONNECTED;
                    ble_log_state_change();
                    clear_device_path();
                    if (music_id != 0)
                    {
                        g_dbus_connection_signal_unsubscribe(ble_g_conn, music_id);
                        music_id = 0;
                    }
                    ble_device.clear();
                    ble_title.clear();
                    ble_artist.clear();
                    ble_album.clear();
                    ble_duration = 0;
                    ble_position = 0;
                    view_ble_update_info();
                }
            }
            g_variant_unref(value);
        }
    }

    if (changed_props)
    {
        g_variant_iter_free(changed_props);
        changed_props = NULL;
    }
}

void ble_set_adapter_property(const char *property, GVariant *value)
{
    GError *error = NULL;
    GVariant *result;
    GDBusProxy *proxy = g_dbus_proxy_new_sync(
        ble_g_conn,
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
        goto fail;
    }

    result = g_dbus_proxy_call_sync(
        proxy, "Set",
        g_variant_new("(ssv)", "org.bluez.Adapter1", property, value),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (error)
    {
        LV_LOG_ERROR("Error: %s", error->message);
        g_error_free(error);
        if (result)
        {
            g_variant_unref(result);
        }
    }
    else if (result)
    {
        g_variant_unref(result);
    }
fail:
    if (proxy)
    {
        g_object_unref(proxy);
    }
}

static void set_device_property(const char *device_path, const char *property, GVariant *value)
{
    GError *error = NULL;
    GVariant *result;
    GDBusProxy *proxy = g_dbus_proxy_new_sync(
        ble_g_conn,
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
        goto fail;
    }

    result = g_dbus_proxy_call_sync(
        proxy, "Set",
        g_variant_new("(ssv)", "org.bluez.Device1", property, value),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error)
    {
        LV_LOG_ERROR("Error: %s", error->message);
        g_error_free(error);
        if (result)
        {
            g_variant_unref(result);
        }
    }
    else if (result)
    {
        g_variant_unref(result);
    }
fail:
    if (proxy)
    {
        g_object_unref(proxy);
    }
}

void ble_send_media_command(ble_music_command command)
{
    if (!device_player_path || !command)
    {
        return;
    }

    const char *method = NULL;
    GError *error = NULL;
    GVariant *result;
    GDBusProxy *proxy = g_dbus_proxy_new_sync(
        ble_g_conn,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.bluez",
        device_player_path,
        "org.bluez.MediaPlayer1",
        NULL,
        &error);
    if (error)
    {
        LV_LOG_ERROR("Failed to create MediaControl proxy: %s", error->message);
        g_error_free(error);
        goto fail;
    }

    if (command == BLE_MUSIC_COMMAND_NEXT)
    {
        method = "Next";
    }
    else if (command == BLE_MUSIC_COMMAND_LAST)
    {
        method = "Previous";
    }
    else if (command == BLE_MUSIC_COMMAND_PLAY)
    {
        method = "Play";
    }
    else if (command == BLE_MUSIC_COMMAND_PAUSE)
    {
        method = "Pause";
    }
    else
    {
        LV_LOG_ERROR("Unknown media command: %d", command);
        goto fail;
    }

    result = g_dbus_proxy_call_sync(
        proxy,
        method,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);
    if (error)
    {
        LV_LOG_ERROR("Failed to send %s command: %s", method, error->message);
        g_error_free(error);
    }
    else if (result)
    {
        g_variant_unref(result);
    }

fail:
    g_object_unref(proxy);
}

void ble_info_init()
{
    property_id = g_dbus_connection_signal_subscribe(
        ble_g_conn,
        "org.bluez", "org.freedesktop.DBus.Properties", "PropertiesChanged", NULL, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE, on_property_changed, NULL, NULL);
}

void ble_info_close()
{
    if (property_id)
    {
        g_dbus_connection_signal_unsubscribe(ble_g_conn, property_id);
        property_id = 0;
    }

    if (music_id)
    {
        g_dbus_connection_signal_unsubscribe(ble_g_conn, music_id);
        music_id = 0;
    }

    if (iface_added_id)
    {
        g_dbus_connection_signal_unsubscribe(ble_g_conn, iface_added_id);
        iface_added_id = 0;
    }
}
