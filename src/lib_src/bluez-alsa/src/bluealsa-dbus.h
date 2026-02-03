/*
 * BlueALSA - bluealsa-dbus.h
 * SPDX-FileCopyrightText: 2016-2025 BlueALSA developers
 * SPDX-License-Identifier: MIT
 */

#pragma once
#ifndef BLUEALSA_BLUEALSADBUS_H_
#define BLUEALSA_BLUEALSADBUS_H_

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

#include "ba-rfcomm.h"
#include "ba-device.h"
#include "ba-transport-pcm.h"

#define BA_DBUS_PCM_UPDATE_FORMAT           (1 << 0)
#define BA_DBUS_PCM_UPDATE_CHANNELS         (1 << 1)
#define BA_DBUS_PCM_UPDATE_CHANNEL_MAP      (1 << 2)
#define BA_DBUS_PCM_UPDATE_RATE             (1 << 3)
#define BA_DBUS_PCM_UPDATE_CODEC            (1 << 4)
#define BA_DBUS_PCM_UPDATE_CODEC_CONFIG     (1 << 5)
#define BA_DBUS_PCM_UPDATE_DELAY            (1 << 6)
#define BA_DBUS_PCM_UPDATE_CLIENT_DELAY     (1 << 7)
#define BA_DBUS_PCM_UPDATE_SOFT_VOLUME      (1 << 8)
#define BA_DBUS_PCM_UPDATE_VOLUME           (1 << 9)
#define BA_DBUS_PCM_UPDATE_RUNNING          (1 << 10)

#define BA_DBUS_RFCOMM_UPDATE_FEATURES (1 << 0)
#define BA_DBUS_RFCOMM_UPDATE_BATTERY  (1 << 1)

#endif
