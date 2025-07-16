#include "../lib_src/bluez-alsa/src/ba-transport.h"
#include "../lib_src/bluez-alsa/src/ba-transport-pcm.h"
#include "../lib_src/bluez-alsa/src/bluez.h"
#include "../lib_src/bluez-alsa/src/dbus.h"

#include "../player/sound.h"
#include "../lvgl/src/misc/lv_log.h"

#include <gio/gio.h>
#include <string.h>
#include <pthread.h>

static struct ba_transport *t;
static pthread_t tid;

extern GDBusConnection *ble_g_conn;

void ble_send_volume();
void ble_send_battery();

static void *later_send(void *arg)
{
    float vol = alsa_get_volume();
    guint8 avrcp_vol = (guint8)(vol / 100 * 127);

    usleep(500000);

    g_dbus_set_property(ble_g_conn, t->bluez_dbus_owner, t->bluez_dbus_path,
                        "org.bluez.MediaTransport1", "Volume", g_variant_new_uint16(avrcp_vol), NULL);

    ble_send_battery();
    return NULL;
}

void ble_send_volume()
{
    if (t == NULL)
    {
        return;
    }
    float vol = alsa_get_volume();
    guint8 avrcp_vol = (guint8)(vol / 100 * 127);

    g_dbus_set_property(ble_g_conn, t->bluez_dbus_owner, t->bluez_dbus_path,
                        "org.bluez.MediaTransport1", "Volume", g_variant_new_uint16(avrcp_vol), NULL);
}

void ble_send_battery()
{
    if (t == NULL)
    {
        return;
    }
    t->d->charge = 50;
    bluez_battery_provider_update(t->d);
}

void ble_set_ba_transport(struct ba_device *device,
                          enum ba_transport_profile profile,
                          const char *dbus_owner,
                          const char *dbus_path,
                          const struct a2dp_sep *sep,
                          const void *configuration)
{
    LV_LOG_USER("create alsa ba_transport: %s", dbus_path);

    if (t != NULL)
    {
        ba_transport_unref(t);
        t = NULL;
    }
    t = ba_transport_new_a2dp(device, profile, dbus_owner, dbus_path, sep, configuration);
    t->a2dp.pcm.fd = 0;
}

void ble_later_send_volume()
{
    int res = pthread_create(&tid, NULL, later_send, NULL);
    if (res)
    {
        LV_LOG_ERROR("Ble thread run fail: %d", res);
    }
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
