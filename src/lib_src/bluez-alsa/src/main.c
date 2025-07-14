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
#include "storage.h"
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

static bool dbus_name_acquired = false;
static int retval = EXIT_SUCCESS;
static GMainLoop *loop;

int bluez_alsa_start()
{
    log_open("bluez-alsa", false);

    if (ba_config_init() != 0)
    {
        error("Couldn't initialize configuration");
        return EXIT_FAILURE;
    }

    config.profile.a2dp_sink = true;

    /* initialize random number generator */
    srandom(time(NULL));

    char *address;
    GError *err;

    err = NULL;
    address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    if ((config.dbus = g_dbus_connection_new_for_address_sync(address,
                                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                                  G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                                              NULL, NULL, &err)) == NULL)
    {
        error("Couldn't obtain D-Bus connection: %s", err->message);
        return EXIT_FAILURE;
    }

    /* Make sure that mandatory codecs are enabled. */

    a2dp_opus_sink.enabled = true;
    a2dp_aptx_sink.enabled = true;
    a2dp_aac_sink.enabled = true;
    a2dp_sbc_sink.enabled = true;

    config.battery.available = true;
    config.battery.level = 100;

    if (a2dp_seps_init() == -1)
        return EXIT_FAILURE;

    storage_init("./bluealsa");

    loop = g_main_loop_new(NULL, FALSE);

    bluez_init();

    /* main dispatching loop */
    debug("Starting main dispatching loop");
    g_main_loop_run(loop);

    /* cleanup internal structures */
    bluez_destroy();

    storage_destroy();
    g_dbus_connection_close_sync(config.dbus, NULL, NULL);
    g_main_loop_unref(loop);
    g_free(address);

    return retval;
}

void main_loop_exit_handler()
{
    if (loop)
    {
        g_main_loop_quit(loop);
    }
}