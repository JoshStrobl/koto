/* media-keys.c
 *
 * Copyright 2021 Joshua Strobl
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glib-2.0/glib.h>
#include <glib-2.0/gio/gio.h>
#include <gtk-4.0/gtk/gtk.h>
#include "engine.h"
#include "media-keys.h"

extern GtkWindow * main_window;
extern KotoPlaybackEngine * playback_engine;

GDBusConnection * media_keys_dbus_conn = NULL;
GDBusProxy * media_keys_proxy = NULL;
GDBusNodeInfo * media_keys_introspection_data = NULL;

static const gchar introspection_xml[] =
	"<node name='/org/gnome/SettingsDaemon/MediaKeys'>"
	"  <interface name='org.gnome.SettingsDaemon.MediaKeys'>"
	"    <annotation name='org.freedesktop.DBus.GLib.CSymbol' value='gsd_media_keys_manager'/>"
	"    <method name='GrabMediaPlayerKeys'>"
	"      <arg name='application' direction='in' type='s'/>"
	"      <arg name='time' direction='in' type='u'/>"
	"    </method>"
	"    <method name='ReleaseMediaPlayerKeys'>"
	"      <arg name='application' direction='in' type='s'/>"
	"    </method>"
	"    <signal name='MediaPlayerKeyPressed'>"
	"      <arg name='application' type='s'/>"
	"      <arg name='key' type='s'/>"
	"    </signal>"
	"  </interface>"
	"</node>";

void grab_media_keys() {
	if (media_keys_proxy == NULL) { // No connection
		return;
	}

	g_dbus_proxy_call(
		media_keys_proxy,
		"GrabMediaPlayerKeys",
		g_variant_new("(su)", "com.github.joshstrobl.koto", 0),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		handle_media_keys_async_done,
		NULL
	);
}

void handle_media_keys_async_done(
	GObject * source_object,
	GAsyncResult * res,
	gpointer user_data
) {
	(void) user_data;
	g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), res, NULL); // Ensure we finish our call
}

void handle_media_keys_signal(
	GDBusProxy * proxy,
	const gchar * sender_name,
	const gchar * signal_name,
	GVariant * parameters,
	gpointer user_data
) {
	(void) proxy;
	(void) sender_name;
	(void) user_data;
	if (g_strcmp0(signal_name, "MediaPlayerKeyPressed") != 0) { // Not MediaPlayerKeyPressed
		return;
	}

	gchar * application_name = NULL;
	gchar * key = NULL;


	g_variant_get(parameters, "(ss)", &application_name, &key);

	if (g_strcmp0(application_name, "com.github.joshstrobl.koto") != 0) { // Not for Koto
		return;
	}

	if (g_strcmp0(key, "Play") == 0) {
		koto_playback_engine_play(playback_engine);
	} else if (g_strcmp0(key, "Pause") == 0) {
		koto_playback_engine_pause(playback_engine);
	} else if (g_strcmp0(key, "Stop") == 0) {
		koto_playback_engine_stop(playback_engine);
	} else if (g_strcmp0(key, "Previous") == 0) {
		koto_playback_engine_backwards(playback_engine);
	} else if (g_strcmp0(key, "Next") == 0) {
		koto_playback_engine_forwards(playback_engine);
	} else if (g_strcmp0(key, "Repeat") == 0) {
		koto_playback_engine_toggle_track_repeat(playback_engine);
	} else if (g_strcmp0(key, "Shuffle") == 0) {
		koto_playback_engine_toggle_track_shuffle(playback_engine);
	}
}

void handle_window_enter(
	GtkEventControllerFocus * controller,
	gpointer user_data
) {
	(void) controller;
	(void) user_data;
	grab_media_keys(); // Grab our media keys
}

void handle_window_leave(
	GtkEventControllerFocus * controller,
	gpointer user_data
) {
	(void) controller;
	(void) user_data;
	release_media_keys(); // Release our media keys
}

void release_media_keys() {
	if (media_keys_dbus_conn == NULL) { // No connection
		return;
	}

	GVariant * params = g_variant_new_string(g_strdup("com.github.joshstrobl.koto"));


	g_dbus_proxy_call(
		media_keys_proxy,
		"ReleaseMediaPlayerKeys",
		params,
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		handle_media_keys_async_done,
		NULL
	);
}

void setup_mediakeys_interface() {
	media_keys_introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	g_assert(media_keys_introspection_data != NULL);

	GDBusConnection * bus;
	GError * error = NULL;


	bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

	if (bus == NULL) { // Failed to get session bus
		g_printerr("Failed to get our session bus: %s\n", error->message);
		g_error_free(error);
		return;
	}

	media_keys_proxy = g_dbus_proxy_new_sync(
		bus,
		G_DBUS_PROXY_FLAGS_NONE,
		NULL,
		"org.gnome.SettingsDaemon.MediaKeys",
		"/org/gnome/SettingsDaemon/MediaKeys",
		"org.gnome.SettingsDaemon.MediaKeys",
		NULL,
		&error
	);

	if (media_keys_proxy == NULL) {
		g_printerr("Failed to get a proxy to GNOME Settings Daemon Media-Keys: %s\n", error->message);
		g_error_free(error);
		return;
	}

	g_signal_connect_object(
		media_keys_proxy,
		"g-signal",
		G_CALLBACK(handle_media_keys_signal),
		NULL,
		0
	);

	GtkEventController * focus_controller = gtk_event_controller_focus_new(); // Create a new focus controller


	g_signal_connect(focus_controller, "enter", G_CALLBACK(handle_window_enter), NULL);
	g_signal_connect(focus_controller, "leave", G_CALLBACK(handle_window_leave), NULL);
	gtk_widget_add_controller(GTK_WIDGET(main_window), focus_controller);
}
