#include "../lib_src/bluez-alsa/src/ba-transport.h"
#include "../lib_src/bluez-alsa/src/ba-transport-pcm.h"
#include "../lib_src/bluez-alsa/src/bluez.h"

#include "../player/sound.h"

#include <string.h>

static struct ba_transport *t;

void ble_send();

void ble_set_ba_transport(struct ba_device *device,
                          enum ba_transport_profile profile,
                          const char *dbus_owner,
                          const char *dbus_path,
                          const struct a2dp_sep *sep,
                          const void *configuration)
{
    if (t != NULL)
    {
        ba_transport_unref(t);
        t = NULL;
    }
    t = ba_transport_new_a2dp(device, profile, dbus_owner, dbus_path, sep, configuration);
    t->a2dp.pcm.fd = 0;

    ble_send();
}

struct ba_transport *ble_get_ba_transport()
{
    return t;
}

void ble_set_a2dp_state(enum bluez_a2dp_transport_state state)
{
    switch (state)
    {
    case BLUEZ_A2DP_TRANSPORT_STATE_IDLE:
        alsa_clear();
        break;
    case BLUEZ_A2DP_TRANSPORT_STATE_ACTIVE:
        alsa_ready();
        break;
    default:
        break;
    }
}

uint16_t ble_get_volume()
{
    return alsa_get_volume();
}

void ble_set_volume(float value)
{
    alsa_set_volume(value);
}

void ble_set_format(uint16_t channels, uint32_t rate)
{
    alsa_reset();
    alsa_set(SND_PCM_FORMAT_S16, channels, rate);
}

void ble_write(const void *buffer, size_t samples)
{
    // alsa_check_buffer(samples);

    // int16_t *buffer1 = (int16_t *)buffer;
    // int16_t *buffer2 = (int16_t *)sound_buf;

    // for (size_t i = 0; i < samples; i++)
    // {
    //     buffer2[i] = buffer1[i];
    // }

    alsa_write_buffer(buffer, samples);
}

void ble_send()
{
    if (t != NULL)
    {
        t->d->charge = 100;
        bluez_battery_provider_update(t->d);
    }
}