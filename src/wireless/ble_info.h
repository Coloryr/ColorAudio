#ifndef __BLE_INFO_H__
#define __BLE_INFO_H__

#include <gio/gio.h>

#include "../player/player_info.h"

void ble_set_adapter_property(const char *property, GVariant *value);
void ble_send_media_command(music_command command);
void ble_send_volume(guint8 value);

void ble_info_init();
void ble_info_close();

#endif // __BLE_INFO_H__