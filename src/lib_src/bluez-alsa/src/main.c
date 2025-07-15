/*
 * BlueALSA - main.c
 * Copyright (c) 2016-2024 Arkadiusz Bokowy
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <getopt.h>
#include <libgen.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <gio/gio.h>
#include <glib-unix.h>
#include <glib.h>

#include "a2dp.h"
#include "a2dp-sbc.h"
#include "a2dp-aac.h"
#include "a2dp-aptx.h"
#include "a2dp-opus.h"
#include "audio.h"
#include "ba-config.h"
#include "bluez.h"
#include "codec-sbc.h"
#include "hfp.h"
#include "shared/a2dp-codecs.h"
#include "shared/defs.h"
#include "shared/log.h"
#include "shared/nv.h"

/* If glib does not support immediate return in case of bus
 * name being owned by some other connection (glib < 2.54),
 * fall back to a default behavior - enter waiting queue. */
#ifndef G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE
#define G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE \
    G_BUS_NAME_OWNER_FLAGS_NONE
#endif

void bluez_alsa_start(GDBusConnection *conn)
{
    log_open("bluez-alsa", false);

    ba_config_init();

    config.profile.a2dp_sink = true;

    config.dbus = conn;

    a2dp_opus_sink.enabled = true;
    a2dp_aptx_sink.enabled = true;
    a2dp_aac_sink.enabled = true;
    a2dp_sbc_sink.enabled = true;

    if (a2dp_seps_init() == -1)
        return;

    bluez_init();
}

void bluez_alsa_close()
{
    bluez_destroy();
    g_dbus_connection_close_sync(config.dbus, NULL, NULL);
}
