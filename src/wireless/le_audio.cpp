// #include "le_audio.h"
// #include <glib-unix.h>
// #include <gio/gio.h> // Needed for GDBus functions
// #include <bluetooth/bluetooth.h>
// #include <bluetooth/hci.h>
// #include <bluetooth/hci_lib.h>
// #include <lc3.h>
// #include <cstring>
// #include <thread>
// #include <mutex>

// // Forward declarations for handler functions
// static void handle_release(GDBusMethodInvocation* invocation, gpointer user_data);
// static void handle_new_connection(GDBusMethodInvocation* invocation, 
//                                  GUnixFDList* fd_list,
//                                  GVariant* arg_device,
//                                  GVariant* arg_fd,
//                                  GVariant* arg_fd_properties,
//                                  gpointer user_data);
// static void handle_request_disconnection(GDBusMethodInvocation* invocation, 
//                                         GVariant* arg_device,
//                                         gpointer user_data);

// // Handle D-Bus method calls
// static void handle_method_call(GDBusConnection* connection,
//                               const gchar* sender,
//                               const gchar* object_path,
//                               const gchar* interface_name,
//                               const gchar* method_name,
//                               GVariant* parameters,
//                               GDBusMethodInvocation* invocation,
//                               gpointer user_data) {
//     LE_AudioManager* manager = static_cast<LE_AudioManager*>(user_data);
    
//     if (g_strcmp0(method_name, "Release") == 0) {
//         handle_release(invocation, manager);
//     } 
//     else if (g_strcmp0(method_name, "NewConnection") == 0) {
//         GUnixFDList* fd_list = g_dbus_message_get_unix_fd_list(
//             g_dbus_method_invocation_get_message(invocation));
        
//         const gchar* device_path = nullptr;
//         gint fd_index = 0;
//         GVariant* properties = nullptr;
        
//         g_variant_get(parameters, "(&sh@a{sv})", 
//                      &device_path, &fd_index, &properties);
        
//         GVariant* arg_device = g_variant_new_string(device_path);
//         GVariant* arg_fd = g_variant_new_handle(fd_index);
        
//         handle_new_connection(invocation, fd_list, 
//                               arg_device, arg_fd, properties, 
//                               manager);
        
//         g_variant_unref(properties);
//     } 
//     else if (g_strcmp0(method_name, "RequestDisconnection") == 0) {
//         const gchar* device_path = nullptr;
//         g_variant_get(parameters, "(&s)", &device_path);
//         GVariant* arg_device = g_variant_new_string(device_path);
//         handle_request_disconnection(invocation, arg_device, manager);
//         g_variant_unref(arg_device);
//     }
// }

// // LE Audio Manager implementation
// LE_AudioManager* le_audio_manager_create() {
//     LE_AudioManager* manager = new LE_AudioManager();
//     manager->conn = nullptr;
//     manager->manager = nullptr;
//     manager->connected_device = nullptr;
//     manager->streaming = false;
//     manager->channels = 2; // Default to stereo
//     manager->sample_rate = 48000; // Default sample rate
//     manager->volume = 50; // Default volume level
//     manager->mute = false;
//     return manager;
// }

// void le_audio_manager_free(LE_AudioManager* manager) {
//     if (!manager) return;
    
//     if (manager->connected_device) {
//         g_free(manager->connected_device);
//     }
    
//     if (manager->manager) {
//         g_object_unref(manager->manager);
//     }
    
//     if (manager->conn) {
//         g_object_unref(manager->conn);
//     }
    
//     delete manager;
// }

// // D-Bus method handlers
// static void handle_release(GDBusMethodInvocation* invocation, gpointer user_data) {
//     LE_AudioManager* manager = static_cast<LE_AudioManager*>(user_data);
//     g_dbus_method_invocation_return_value(invocation, nullptr);
// }

// static void handle_new_connection(GDBusMethodInvocation* invocation, 
//                                  GUnixFDList* fd_list,
//                                  GVariant* arg_device,
//                                  GVariant* arg_fd,
//                                  GVariant* arg_fd_properties,
//                                  gpointer user_data) {
//     LE_AudioManager* manager = static_cast<LE_AudioManager*>(user_data);
    
//     const gchar* device_path = g_variant_get_string(arg_device, nullptr);
//     gint fd_index = g_variant_get_handle(arg_fd);
//     int fd = g_unix_fd_list_get(fd_list, fd_index, nullptr);
    
//     // Store connected device
//     manager->connected_device = g_strdup(device_path);
    
//     // Start audio processing thread
//     manager->streaming = true;
//     std::thread([manager, fd]() {
//         uint8_t buffer[1024];
//         ssize_t bytes_read;
        
//         // Initialize LC3 decoder
//         lc3_decoder_t decoder = lc3_setup_decoder(
//             0, // No flags
//             manager->sample_rate, 
//             manager->channels,
//             nullptr
//         );
        
//         if (!decoder) {
//             g_warning("Failed to initialize LC3 decoder");
//             close(fd);
//             return;
//         }
        
//         // Set ALSA parameters
//         alsa_set(SND_PCM_FORMAT_S16_LE, manager->channels, manager->sample_rate);
        
//         while (manager->streaming) {
//             bytes_read = read(fd, buffer, sizeof(buffer));
//             if (bytes_read <= 0) {
//                 if (bytes_read == 0 || errno == EAGAIN) continue;
//                 break;
//             }
            
//             // Process audio data through LC3 decoder
//             int16_t pcm_buffer[480]; // 10ms of 48kHz audio
//             int pcm_samples = lc3_decode(
//                 decoder, 
//                 buffer, 
//                 bytes_read,
//                 LC3_PCM_FORMAT_S16,
//                 pcm_buffer,
//                 sizeof(pcm_buffer)
//             );
            
//             if (pcm_samples > 0) {
//                 // Apply volume
//                 if (!manager->mute && manager->volume != 100) {
//                     float volume_factor = static_cast<float>(manager->volume) / 100.0f;
//                     for (int i = 0; i < pcm_samples; i++) {
//                         pcm_buffer[i] = static_cast<int16_t>(pcm_buffer[i] * volume_factor);
//                     }
//                 }
                
//                 // Write to ALSA
//                 alsa_write_buffer(pcm_buffer, pcm_samples * sizeof(int16_t));
//             }
//         }
        
//         // Clean up
//         // Release decoder resources
//         #if defined(lc3_release_decoder)
//             lc3_release_decoder(decoder);
//         #elif defined(lc3_free)
//             lc3_free(decoder);
//         #else
//             // Fallback: just log a warning if we can't release the decoder
//             g_warning("Unable to release LC3 decoder - no cleanup function available");
//         #endif
//         close(fd);
//         manager->streaming = false;
//     }).detach();
    
//     g_dbus_method_invocation_return_value(invocation, nullptr);
// }

// static void handle_request_disconnection(GDBusMethodInvocation* invocation, 
//                                         GVariant* arg_device,
//                                         gpointer user_data) {
//     LE_AudioManager* manager = static_cast<LE_AudioManager*>(user_data);
    
//     const gchar* device_path = g_variant_get_string(arg_device, nullptr);
//     if (manager->connected_device && 
//         strcmp(manager->connected_device, device_path) == 0) {
//         manager->streaming = false;
//         g_free(manager->connected_device);
//         manager->connected_device = nullptr;
//     }
    
//     g_dbus_method_invocation_return_value(invocation, nullptr);
// }

// // Profile interface implementation
// static const GDBusInterfaceVTable profile_vtable = {
//     [](GDBusConnection* conn,
//        const gchar* sender,
//        const gchar* object_path,
//        const gchar* interface_name,
//        const gchar* method_name,
//        GVariant* parameters,
//        GDBusMethodInvocation* invocation,
//        gpointer user_data) {
        
//         LE_AudioManager* manager = static_cast<LE_AudioManager*>(user_data);
        
//         if (g_strcmp0(method_name, "Release") == 0) {
//             handle_release(invocation, manager);
//         } 
//         else if (g_strcmp0(method_name, "NewConnection") == 0) {
//             GUnixFDList* fd_list = g_dbus_message_get_unix_fd_list(
//                 g_dbus_method_invocation_get_message(invocation));
            
//             GVariant* arg_device = nullptr;
//             GVariant* arg_fd = nullptr;
//             GVariant* arg_fd_properties = nullptr;
            
//             g_variant_get(parameters, "(&s@h@a{sv})", 
//                          &arg_device, &arg_fd, &arg_fd_properties);
            
//             handle_new_connection(invocation, fd_list, 
//                                   arg_device, arg_fd, arg_fd_properties, 
//                                   manager);
            
//             g_variant_unref(arg_device);
//             g_variant_unref(arg_fd);
//             g_variant_unref(arg_fd_properties);
//         } 
//         else if (g_strcmp0(method_name, "RequestDisconnection") == 0) {
//             GVariant* arg_device = nullptr;
//             g_variant_get(parameters, "(&s)", &arg_device);
//             handle_request_disconnection(invocation, arg_device, manager);
//             g_variant_unref(arg_device);
//         }
//     },
//     nullptr, // get_property
//     nullptr  // set_property
// };

// bool le_audio_manager_register_profile(LE_AudioManager* manager) {
//     GError* error = nullptr;
    
//     // Get system bus connection
//     manager->conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
//     if (error) {
//         g_warning("Failed to get D-Bus connection: %s", error->message);
//         g_error_free(error);
//         return false;
//     }
    
//     // Create object manager
//     manager->manager = g_dbus_object_manager_server_new(LE_AUDIO_PROFILE_PATH);
    
//     // Create profile object
//     GDBusObjectSkeleton* profile_skeleton = 
//         g_dbus_object_skeleton_new(LE_AUDIO_PROFILE_PATH);
    
//     // Create and register profile interface
//     GDBusNodeInfo* introspect_data = g_dbus_node_info_new_for_xml(
//         "<node>"
//         "  <interface name='" LE_AUDIO_PROFILE_INTERFACE "'>"
//         "    <method name='Release'>"
//         "    </method>"
//         "    <method name='NewConnection'>"
//         "      <arg type='s' name='device' direction='in'/>"
//         "      <arg type='h' name='fd' direction='in'/>"
//         "      <arg type='a{sv}' name='properties' direction='in'/>"
//         "    </method>"
//         "    <method name='RequestDisconnection'>"
//         "      <arg type='s' name='device' direction='in'/>"
//         "    </method>"
//         "  </interface>"
//         "</node>", NULL);
    
//     GDBusInterfaceVTable vtable = {
//         handle_method_call,
//         NULL, // get_property
//         NULL  // set_property
//     };
    
//     guint registration_id = g_dbus_connection_register_object(
//         manager->conn,
//         LE_AUDIO_PROFILE_PATH,
//         introspect_data->interfaces[0],
//         &vtable,
//         manager,
//         NULL,
//         &error);
    
//     if (error) {
//         g_warning("Failed to register interface: %s", error->message);
//         g_error_free(error);
//         return false;
//     }
    
//     // Register profile with BlueZ
//     GVariantBuilder builder;
//     g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
//     g_variant_builder_add(&builder, "{sv}", "Name", g_variant_new_string("LE Audio"));
//     g_variant_builder_add(&builder, "{sv}", "Role", g_variant_new_string("sink"));
//     g_variant_builder_add(&builder, "{sv}", "Channel", g_variant_new_uint16(0));
    
//     g_dbus_connection_call_sync(
//         manager->conn,
//         "org.bluez",
//         "/org/bluez",
//         "org.bluez.ProfileManager1",
//         "RegisterProfile",
//         g_variant_new("(o{sv})", LE_AUDIO_PROFILE_PATH, &builder),
//         nullptr,
//         G_DBUS_CALL_FLAGS_NONE,
//         -1,
//         nullptr,
//         &error);
    
//     if (error) {
//         g_warning("Failed to register profile: %s", error->message);
//         g_error_free(error);
//         return false;
//     }
    
//     return true;
// }


// void le_audio_manager_set_volume(LE_AudioManager* manager, uint8_t volume) {
//     if (volume > 100) volume = 100;
//     manager->volume = volume;
    
//     // Map to ALSA volume range (0.0-1.0)
//     float alsa_vol = static_cast<float>(volume) / 100.0f;
//     alsa_set_volume(alsa_vol);
// }

// void le_audio_manager_set_mute(LE_AudioManager* manager, bool mute) {
//     manager->mute = mute;
//     // Implement mute by setting volume to 0
//     alsa_set_volume(mute ? 0.0f : static_cast<float>(manager->volume) / 100.0f);
// }
