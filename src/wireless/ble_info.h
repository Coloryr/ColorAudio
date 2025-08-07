#ifndef __BLE_INFO_H__
#define __BLE_INFO_H__

#include <gio/gio.h>
#include <stdint.h>
#include <string>

typedef enum
{
    BLE_MUSIC_COMMAND_PLAY = 0,
    BLE_MUSIC_COMMAND_PAUSE,
    BLE_MUSIC_COMMAND_NEXT,
    BLE_MUSIC_COMMAND_LAST,
    BLE_MUSIC_COMMAND_UNKNOW = -1
} ble_music_command;

extern std::string ble_device;
extern std::string ble_title;
extern std::string ble_artist;
extern std::string ble_album;
extern uint32_t ble_duration;
extern uint32_t ble_position;

void ble_set_adapter_property(const char *property, GVariant *value);
void ble_send_media_command(ble_music_command command);

void ble_info_init();
void ble_info_close();

#endif // __BLE_INFO_H__