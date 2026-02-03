#ifndef __BLE_INFO_H__
#define __BLE_INFO_H__

#include <stdint.h>
#include <string>

#include <gio/gio.h>

typedef enum
{
    BT_MUSIC_COMMAND_PLAY = 0,
    BT_MUSIC_COMMAND_PAUSE,
    BT_MUSIC_COMMAND_NEXT,
    BT_MUSIC_COMMAND_LAST,
    BT_MUSIC_COMMAND_UNKNOW = -1
} bt_music_command;

extern std::string bt_device;
extern std::string bt_title;
extern std::string bt_artist;
extern std::string bt_album;
extern uint32_t bt_duration;
extern uint32_t bt_position;

void bt_set_adapter_property(const char *property, GVariant *value);
void bt_send_media_command(bt_music_command command);

void bt_info_init();
void bt_info_close();

#endif // __BLE_INFO_H__