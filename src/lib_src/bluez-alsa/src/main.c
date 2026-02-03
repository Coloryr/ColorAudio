/*
 * BlueALSA - main.c
 * SPDX-FileCopyrightText: 2016-2025 BlueALSA developers
 * SPDX-License-Identifier: MIT
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
#include <string.h>
#include <strings.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <gio/gio.h>
#include <glib-unix.h>
#include <glib.h>

#if ENABLE_LDAC
#include <ldacBT.h>
#endif

#include "a2dp.h"
#include "a2dp-sbc.h"
#include "audio.h"
#include "ba-config.h"
#include "bluez.h"
#include "codec-sbc.h"
#include "error.h"
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

void bluez_alsa_start(GDBusConnection *conn)
{
	ba_config_init();

	srandom(time(NULL));

	a2dp_sbc_sink.enabled = true;
	config.hfp.codecs.cvsd = true;
	config.profile.a2dp_sink = true;
	config.profile.hfp_hf = true;
	config.dbus = conn;

	if (a2dp_seps_init() != ERROR_CODE_OK)
	{
		error("Couldn't initialize ad2p");
		return;
	}

	storage_init(BLUEALSA_STORAGE_DIR);

	bluez_init();
}

void bluez_alsa_close()
{
	bluez_destroy();

	storage_destroy();
}