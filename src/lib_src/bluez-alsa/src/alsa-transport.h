#ifndef __ALSA_TRANSPORT_H__
#define __ALSA_TRANSPORT_H__

#include "bluez.h"

#include "ba-transport.h"

void ble_set_a2dp_state(enum bluez_a2dp_transport_state state);
uint16_t ble_get_volume();
void ble_set_volume(float value);
void ble_set_format(uint16_t channels, uint32_t rate);

void ble_set_ba_transport(struct ba_device *device,
		enum ba_transport_profile profile,
		const char *dbus_owner,
		const char *dbus_path,
		const struct a2dp_sep *sep,
		const void *configuration);
struct ba_transport * ble_get_ba_transport();

void ble_write(const void *buffer, size_t samples);

#endif // __ALSA-TRANSPORT_H__