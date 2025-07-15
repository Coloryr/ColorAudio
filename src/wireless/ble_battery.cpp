// #include "ble_battery.h"
// #include "ble.h"
// #include "../lvgl/src/misc/lv_log.h"
// #include <gio/gio.h>
// #include <glib.h>
// #include <stdint.h>

// #define BATTERY_INTERFACE "org.bluez.Battery1"
// #define BATTERY_PATH "/org/bluez/battery"

// static GDBusConnection *connection = NULL;
// static guint battery_property_id = 0;
// static guint8 battery_level = 100; // Initial battery level
// static guint update_source_id = 0;

// static GVariant *handle_get_property(GDBusConnection *conn,
//                                      const gchar *sender,
//                                      const gchar *object_path,
//                                      const gchar *interface_name,
//                                      const gchar *property_name,
//                                      GError **error,
//                                      gpointer user_data)
// {
//     if (g_strcmp0(property_name, "Percentage") == 0)
//     {
//         return g_variant_new_byte(battery_level);
//     }
//     return NULL;
// }

// static gboolean update_battery_level(gpointer user_data)
// {
//     // Simulate battery level change (replace with actual battery reading)
//     if (battery_level > 0)
//     {
//         battery_level--;
//     }
//     else
//     {
//         battery_level = 100;
//     }

//     // Emit PropertiesChanged signal
//     GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
//     g_variant_builder_add(builder, "{sv}", "Percentage", g_variant_new_byte(battery_level));

//     GError *error = NULL;
//     g_dbus_connection_emit_signal(connection,
//                                   NULL,
//                                   BATTERY_PATH,
//                                   "org.freedesktop.DBus.Properties",
//                                   "PropertiesChanged",
//                                   g_variant_new("(sa{sv}as)",
//                                                 BATTERY_INTERFACE,
//                                                 builder,
//                                                 NULL),
//                                   &error);

//     g_variant_builder_unref(builder);

//     if (error)
//     {
//         LV_LOG_ERROR("Failed to emit PropertiesChanged: %s", error->message);
//         g_error_free(error);
//         return G_SOURCE_REMOVE;
//     }

//     return G_SOURCE_CONTINUE;
// }

// gboolean ble_battery_init(void)
// {
//     GError *error = NULL;
//     connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
//     if (!connection)
//     {
//         LV_LOG_ERROR("Failed to get DBus connection: %s", error->message);
//         g_error_free(error);
//         return FALSE;
//     }

//     // Register Battery interface
//     static GDBusInterfaceVTable interface_vtable = {
//         NULL, // method_call
//         handle_get_property,
//         NULL // set_property
//     };

//     battery_property_id = g_dbus_connection_register_object(
//         connection,
//         BATTERY_PATH,
//         g_dbus_node_info_lookup_interface(
//             g_dbus_node_info_new_for_xml(
//                 "<node>"
//                 "  <interface name='" BATTERY_INTERFACE "'>"
//                 "    <property name='Percentage' type='y' access='read'/>"
//                 "  </interface>"
//                 "</node>",
//                 NULL),
//             BATTERY_INTERFACE),
//         &interface_vtable,
//         NULL,
//         NULL,
//         &error);

//     if (error)
//     {
//         LV_LOG_ERROR("Failed to register battery interface: %s", error->message);
//         g_error_free(error);
//         return FALSE;
//     }

//     // Setup periodic update
//     update_source_id = g_timeout_add_seconds(10, update_battery_level, NULL);

//     return TRUE;
// }

// void ble_battery_deinit(void)
// {
//     if (update_source_id)
//     {
//         g_source_remove(update_source_id);
//         update_source_id = 0;
//     }

//     if (battery_property_id)
//     {
//         g_dbus_connection_unregister_object(connection, battery_property_id);
//         battery_property_id = 0;
//     }

//     if (connection)
//     {
//         g_object_unref(connection);
//         connection = NULL;
//     }
// }
