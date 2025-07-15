// #ifndef __LE_AUDIO_H__
// #define __LE_AUDIO_H__

// #include <glib.h>
// #include <gio/gio.h>
// #include <stdint.h>
// #include <stdbool.h>
// #include "../player/sound.h"

// // LE Audio Manager class
// typedef struct {
//     // D-Bus connection
//     GDBusConnection* conn;

//     // Objects
//     GDBusObjectManagerServer* manager;
    
//     // Device tracking
//     char* connected_device;
    
//     // Audio state
//     bool streaming;
//     uint16_t channels;
//     uint32_t sample_rate;
    
//     // Volume control
//     uint8_t volume;
//     bool mute;
// } LE_AudioManager;

// // Function prototypes
// LE_AudioManager* le_audio_manager_create();
// void le_audio_manager_free(LE_AudioManager* manager);
// bool le_audio_manager_register_profile(LE_AudioManager* manager);
// void le_audio_manager_on_device_connected(LE_AudioManager* manager, const char* device_path);
// void le_audio_manager_on_device_disconnected(LE_AudioManager* manager, const char* device_path);
// void le_audio_manager_set_volume(LE_AudioManager* manager, uint8_t volume);
// void le_audio_manager_set_mute(LE_AudioManager* manager, bool mute);
// void le_audio_manager_handle_audio_data(LE_AudioManager* manager, const uint8_t* data, size_t size);

// // D-Bus interface definitions
// #define LE_AUDIO_MANAGER_INTERFACE "org.bluez.LEAudioManager1"
// #define LE_AUDIO_PROFILE_PATH "/org/bluez/le_audio_profile"
// #define LE_AUDIO_PROFILE_INTERFACE "org.bluez.Profile1"

// #endif // __LE_AUDIO_H__
