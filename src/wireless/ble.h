#ifndef __BLE_H__
#define __BLE_H__

#include <gio/gio.h>

#ifdef __cplusplus
extern "C" {
#endif

int bluez_alsa_start();

#ifdef __cplusplus
} /*extern "C"*/
#endif

extern GDBusConnection *ble_g_conn;

void ble_init();

#endif // __BLE_H__