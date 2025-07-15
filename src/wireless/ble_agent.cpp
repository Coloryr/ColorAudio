#include "ble_agent.h"
#include "ble.h"

#include "../lvgl/src/misc/lv_log.h"

#include <glib.h>
#include <gio/gio.h>

static guint registration_id;

static const gchar *agent_path = "/com/coloryr/coloraudio";

static const gchar *agent_interface_xml =
    "<node>"
    "  <interface name='org.bluez.Agent1'>"
    "    <method name='Release'/>"
    "    <method name='RequestPinCode'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='pincode' direction='out'/>"
    "    </method>"
    "    <method name='DisplayPinCode'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='pincode' direction='in'/>"
    "    </method>"
    "    <method name='RequestPasskey'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='out'/>"
    "    </method>"
    "    <method name='DisplayPasskey'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='in'/>"
    "      <arg type='y' name='entered' direction='in'/>"
    "    </method>"
    "    <method name='RequestConfirmation'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='in'/>"
    "    </method>"
    "    <method name='RequestAuthorization'>"
    "      <arg type='o' name='device' direction='in'/>"
    "    </method>"
    "    <method name='AuthorizeService'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='uuid' direction='in'/>"
    "    </method>"
    "    <method name='Cancel'/>"
    "  </interface>"
    "</node>";

static void handle_authorize_service(
    GDBusConnection *connection,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data)
{
    const gchar *device_path;
    const gchar *uuid;
    g_variant_get(parameters, "(&o&s)", &device_path, &uuid);

    LV_LOG_USER("  Device: %s  Service UUID: %s", device_path, uuid);

    ble_set_pairable(false);
    ble_now_state = BLE_STATE_CONNECTED;
    ble_log_state_change();

    g_dbus_method_invocation_return_value(invocation, NULL);
}

static void handle_request_confirmation(
    GDBusConnection *connection,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data)
{
    LV_LOG_USER("RequestConfirmation called");

    const gchar *device_path;
    guint32 passkey;
    g_variant_get(parameters, "(&ou)", &device_path, &passkey);

    LV_LOG_USER("  Device: %s  Passkey: %06u", device_path, passkey);

    ble_now_state = BLE_STATE_PAIR;
    ble_log_state_change();

    g_dbus_method_invocation_return_value(invocation, NULL);
}

// 处理 Cancel 方法调用
static void handle_cancel(
    GDBusConnection *connection,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data)
{
    LV_LOG_USER("Pairing cancelled");
    ble_now_state = BLE_STATE_DISCONNECTED;
    g_dbus_method_invocation_return_value(invocation, NULL);
}

static void register_agent(GDBusConnection *connection)
{
    GError *error = NULL;

    GDBusProxy *agent_manager = g_dbus_proxy_new_sync(
        connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.bluez",
        "/org/bluez",
        "org.bluez.AgentManager1",
        NULL,
        &error);

    if (error)
    {
        LV_LOG_ERROR("AgentManager proxy error: %s", error->message);
        g_clear_error(&error);
        return;
    }

    const gchar *capability = "DisplayYesNo";

    g_dbus_proxy_call_sync(
        agent_manager,
        "RegisterAgent",
        g_variant_new("(os)", agent_path, capability),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error)
    {
        LV_LOG_ERROR("RegisterAgent failed: %s", error->message);
        g_clear_error(&error);
    }
    else
    {
        LV_LOG_USER("Agent registered at %s", agent_path);
    }

    g_dbus_proxy_call_sync(
        agent_manager,
        "RequestDefaultAgent",
        g_variant_new("(o)", agent_path),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (error)
    {
        LV_LOG_ERROR("RequestDefaultAgent failed: %s", error->message);
        g_clear_error(&error);
    }
    else
    {
        LV_LOG_USER("Agent set as default");
    }

    g_object_unref(agent_manager);
}

void ble_agent_call(GDBusConnection *conn,
                    const gchar *sender,
                    const gchar *obj_path,
                    const gchar *iface_name,
                    const gchar *method_name,
                    GVariant *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer user_data)
{
    LV_LOG_USER("agent_call: %s", method_name);

    if (g_str_equal(method_name, "AuthorizeService"))
    {
        handle_authorize_service(conn, sender, obj_path, iface_name,
                                 method_name, parameters, invocation, user_data);
    }
    else if (g_str_equal(method_name, "RequestConfirmation"))
    {
        handle_request_confirmation(conn, sender, obj_path, iface_name,
                                    method_name, parameters, invocation, user_data);
    }
    else if (g_str_equal(method_name, "Cancel"))
    {
        handle_cancel(conn, sender, obj_path, iface_name,
                      method_name, parameters, invocation, user_data);
    }
    else
    {
        g_dbus_method_invocation_return_error(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
            "Method %s not implemented", method_name);
    }
}

void ble_agent_close()
{
    if (registration_id)
    {
        g_dbus_connection_unregister_object(ble_g_conn, registration_id);
    }
}

void ble_agent_init()
{
    GError *error = NULL;

    GDBusNodeInfo *agent_interface =
        g_dbus_node_info_new_for_xml(agent_interface_xml, &error);

    if (error)
    {
        LV_LOG_ERROR("Interface parse error: %s", error->message);
        g_clear_error(&error);
        return;
    }

    GDBusInterfaceVTable table = {
        .method_call = ble_agent_call};

    registration_id = g_dbus_connection_register_object(
        ble_g_conn,
        agent_path,
        agent_interface->interfaces[0],
        &table,
        NULL,
        NULL,
        &error);

    if (error)
    {
        LV_LOG_ERROR("Object registration failed: %s", error->message);
        g_clear_error(&error);
        return;
    }

    LV_LOG_USER("Agent object registered at %s", agent_path);

    register_agent(ble_g_conn);
}
