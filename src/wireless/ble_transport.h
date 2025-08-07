#ifndef __BLE_TRANSPORT_H__
#define __BLE_TRANSPORT_H__

#include <gio/gio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void bluez_alsa_start(GDBusConnection *conn);
void bluez_alsa_close();
void ble_send_volume();
void ble_send_battery();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __BLE_TRANSPORT_H__