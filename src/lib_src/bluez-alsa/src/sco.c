/*
 * BlueALSA - sco.c
 * SPDX-FileCopyrightText: 2016-2025 BlueALSA developers
 * SPDX-License-Identifier: MIT
 */

#include "sco.h"

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sco.h>

#include <glib.h>

#include "ba-device.h"
#include "ba-transport.h"
#include "ba-transport-pcm.h"
#include "error.h"
#include "hci.h"
#include "hfp.h"
#include "sco-cvsd.h"
#include "shared/bluetooth.h"
#include "shared/log.h"
#include "utils.h"

static int sco_connection_dispatcher(
		GIOChannel * ch,
		G_GNUC_UNUSED GIOCondition cond,
		void * userdata) {

	int listen_fd = g_io_channel_unix_get_fd(ch);
	struct ba_adapter * a = userdata;
	struct ba_device * d = NULL;
	struct ba_transport * t = NULL;
	int fd = -1;

	struct sockaddr_sco addr = { 0 };
	socklen_t addrlen = sizeof(addr);
	if ((fd = accept(listen_fd, (struct sockaddr *)&addr, &addrlen)) == -1) {
		error("Couldn't accept incoming SCO link: %s", strerror(errno));
		goto cleanup;
	}

	char addrstr[18];
	ba2str(&addr.sco_bdaddr, addrstr);
	debug("New incoming SCO link [%s]: %d", addrstr, fd);

	if ((d = ba_device_lookup(a, &addr.sco_bdaddr)) == NULL) {
		error("Couldn't lookup device: %s", addrstr);
		goto cleanup;
	}

	if ((t = ba_transport_lookup(d, d->bluez_dbus_path)) == NULL) {
		error("Couldn't lookup transport: %s", d->bluez_dbus_path);
		goto cleanup;
	}

	ba_transport_stop(t);

	pthread_mutex_lock(&t->bt_fd_mtx);

	t->bt_fd = fd;
	t->mtu_read = t->mtu_write = hci_sco_get_mtu(fd, a);
	fd = -1;

	pthread_mutex_unlock(&t->bt_fd_mtx);

	ba_transport_pcm_state_set_idle(&t->sco.pcm_spk);
	ba_transport_pcm_state_set_idle(&t->sco.pcm_mic);
	ba_transport_start(t);

cleanup:
	if (d != NULL)
		ba_device_unref(d);
	if (t != NULL)
		ba_transport_unref(t);
	if (fd != -1)
		close(fd);
	return G_SOURCE_CONTINUE;
}

__attribute__ ((weak))
error_code_t sco_setup_connection_dispatcher(struct ba_adapter * a) {

	/* Skip setup if dispatcher is already running. */
	if (a->sco_dispatcher != NULL)
		return ERROR_CODE_OK;

	/* XXX: It is a known issue with Broadcom chips, that by default, the SCO
	 *      packets are routed via the chip's PCM interface. However, the IO
	 *      thread expects data to be available via the transport interface. */
	if (a->chip.manufacturer == BT_COMPID_BROADCOM) {

		int dd;
		uint8_t routing, clock, frame, sync, clk;

		debug("Checking Broadcom internal SCO routing");

		if ((dd = hci_open_dev(a->hci.dev_id)) == -1 ||
				hci_bcm_read_sco_pcm_params(dd, &routing, &clock, &frame, &sync, &clk, 1000) == -1)
			error("Couldn't read SCO routing params: %s", strerror(errno));
		else {
			debug("Current SCO interface setup: %u %u %u %u %u", routing, clock, frame, sync, clk);
			if (routing != BT_BCM_PARAM_ROUTING_TRANSPORT) {
				debug("Setting SCO routing via transport interface");
				if (hci_bcm_write_sco_pcm_params(dd, BT_BCM_PARAM_ROUTING_TRANSPORT,
						clock, frame, sync, clk, 1000) == -1)
				error("Couldn't write SCO routing params: %s", strerror(errno));
			}
		}

		if (dd != -1)
			hci_close_dev(dd);

	}

	int fd;
	if ((fd = hci_sco_open(a->hci.dev_id)) == -1)
		return ERROR_SYSTEM(errno);

	if (listen(fd, 10) == -1)
		return close(fd), ERROR_SYSTEM(errno);

	g_autoptr(GIOChannel) ch = g_io_channel_unix_raw_new(fd);
	/* Attach SCO dispatcher to the default main context. Please note,
	 * that the adapter is not referenced. It is guaranteed that it will be
	 * available during the whole live-span of the dispatcher, because the
	 * dispatcher is destroyed in the adapter cleanup routine. For details
	 * see the ba_adapter_unref() function. */
	a->sco_dispatcher = g_io_create_watch_full(ch, G_PRIORITY_DEFAULT,
			G_IO_IN, sco_connection_dispatcher, a, NULL);

	debug("Created SCO dispatcher: %s", a->hci.name);
	return ERROR_CODE_OK;
}

void *sco_enc_thread(struct ba_transport_pcm *pcm) {
	switch (ba_transport_get_codec(pcm->t)) {
	case HFP_CODEC_CVSD:
	default:
		return sco_cvsd_enc_thread(pcm);
	}
}

__attribute__ ((weak))
void *sco_dec_thread(struct ba_transport_pcm *pcm) {
	switch (ba_transport_get_codec(pcm->t)) {
	case HFP_CODEC_CVSD:
	default:
		return sco_cvsd_dec_thread(pcm);
	}
}

int sco_transport_init(struct ba_transport * t) {

	t->sco.pcm_spk.format = BA_TRANSPORT_PCM_FORMAT_S16_2LE;
	t->sco.pcm_spk.channels = 1;
	t->sco.pcm_spk.channel_map[0] = BA_TRANSPORT_PCM_CHANNEL_MONO;

	t->sco.pcm_mic.format = BA_TRANSPORT_PCM_FORMAT_S16_2LE;
	t->sco.pcm_mic.channels = 1;
	t->sco.pcm_mic.channel_map[0] = BA_TRANSPORT_PCM_CHANNEL_MONO;

	uint32_t codec_id;
	switch (codec_id = ba_transport_get_codec(t)) {
	case HFP_CODEC_UNDEFINED:
		t->sco.pcm_spk.rate = 0;
		t->sco.pcm_mic.rate = 0;
		break;
	case HFP_CODEC_CVSD:
		t->sco.pcm_spk.rate = 8000;
		t->sco.pcm_mic.rate = 8000;
		break;
	default:
		debug("Unsupported SCO codec: %#x", codec_id);
		g_assert_not_reached();
	}

	// if (t->sco.pcm_spk.ba_dbus_exported)
		// bluealsa_dbus_pcm_update(&t->sco.pcm_spk,
		// 		BA_DBUS_PCM_UPDATE_RATE |
		// 		BA_DBUS_PCM_UPDATE_CODEC |
		// 		BA_DBUS_PCM_UPDATE_CLIENT_DELAY);

	// if (t->sco.pcm_mic.ba_dbus_exported)
		// bluealsa_dbus_pcm_update(&t->sco.pcm_mic,
		// 		BA_DBUS_PCM_UPDATE_RATE |
		// 		BA_DBUS_PCM_UPDATE_CODEC |
		// 		BA_DBUS_PCM_UPDATE_CLIENT_DELAY);

	return 0;
}

int sco_transport_start(struct ba_transport * t) {

	int rv = 0;

	if (t->profile & BA_TRANSPORT_PROFILE_MASK_AG) {
		rv |= ba_transport_pcm_start(&t->sco.pcm_spk, sco_enc_thread, "ba-sco-enc");
		rv |= ba_transport_pcm_start(&t->sco.pcm_mic, sco_dec_thread, "ba-sco-dec");
		return rv;
	}

	if (t->profile & BA_TRANSPORT_PROFILE_MASK_HF) {
		rv |= ba_transport_pcm_start(&t->sco.pcm_spk, sco_dec_thread, "ba-sco-dec");
		rv |= ba_transport_pcm_start(&t->sco.pcm_mic, sco_enc_thread, "ba-sco-enc");
		return rv;
	}

	g_assert_not_reached();
	return -1;
}
