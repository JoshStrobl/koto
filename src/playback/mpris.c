/* mpris.c
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

// Huge thanks to the GLib folks for providing an example server test

#include <glib-2.0/glib.h>
#include <glib-2.0/gio/gio.h>
#include <gtk-4.0/gtk/gtk.h>
#include "../db/cartographer.h"
#include "../playlist/current.h"
#include "../playlist/playlist.h"
#include "../koto-paths.h"
#include "../koto-utils.h"
#include "engine.h"
#include "mimes.h"
#include "mpris.h"

extern gchar * koto_rev_dns;
extern KotoCartographer * koto_maps;
extern KotoCurrentPlaylist * current_playlist;
extern GtkApplication * app;
extern GtkWindow * main_window;
extern KotoPlaybackEngine * playback_engine;
extern GList * supported_mimes;

GDBusConnection * dbus_conn = NULL;
guint mpris_bus_id = 0;
GDBusNodeInfo * introspection_data = NULL;

static const gchar introspection_xml[] =
	"<node>"
	"	<interface name='org.mpris.MediaPlayer2'>"
	"		<method name='Raise' />"
	"		<method name='Quit' />"
	"		<property type='b' name='CanQuit' access='read' />"
	"		<property type='b' name='CanRaise' access='read' />"
	"		<property type='b' name='HasTrackList' access='read' />"
	"		<property type='s' name='Identity' access='read' />"
	"		<property type='s' name='DesktopEntry' access='read' />"
	"		<property type='as' name='SupportedUriSchemas' access='read' />"
	"		<property type='as' name='SupportedMimeTypes' access='read' />"
	"	</interface>"
	"	<interface name='org.mpris.MediaPlayer2.Player'>"
	"		<method name='Next' />"
	"		<method name='Previous' />"
	"		<method name='Pause' />"
	"		<method name='PlayPause' />"
	"		<method name='Stop' />"
	"		<method name='Play' />"
	"		<method name='Seek'>"
	"			<arg type='x' name='offset' />"
	"		</method>"
	"		<method name='SetPosition'>"
	"			<arg type='o' name='track_id' />"
	"			<arg type='x' name='pos' />"
	"		</method>"
	"		<property type='s' name='PlaybackStatus' access='read' />"
	"		<property type='s' name='LoopStatus' access='readwrite' />"
	"		<property type='d' name='Rate' access='readwrite' />"
	"		<property type='b' name='Shuffle' access='readwrite' />"
	"		<property type='a{sv}' name='Metadata' access='read' />"
	"		<property type='d' name='Volume' access='readwrite' />"
	"		<property type='x' name='Position' access='read' />"
	"		<property type='d' name='MinimumRate' access='read' />"
	"		<property type='d' name='MaximumRate' access='read' />"
	"		<property type='b' name='CanGoNext' access='read' />"
	"		<property type='b' name='CanGoPrevious' access='read' />"
	"		<property type='b' name='CanPlay' access='read' />"
	"		<property type='b' name='CanPause' access='read' />"
	"		<property type='b' name='CanSeek' access='read' />"
	"		<property type='b' name='CanControl' access='read' />"
	"	</interface>"
	"</node>";

void handle_method_call(
	GDBusConnection * connection,
	const gchar * sender,
	const gchar * object_path,
	const gchar * interface_name,
	const gchar * method_name,
	GVariant * parameters,
	GDBusMethodInvocation * invocation,
	gpointer user_data
) {
	(void) connection;
	(void) sender;
	(void) object_path;
	(void) parameters;
	(void) invocation;
	(void) user_data;

	if (g_strcmp0(interface_name, "org.mpris.MediaPlayer2") == 0) { // Root mediaplayer2 interface
		if (g_strcmp0(method_name, "Raise") == 0) { // Raise the window
			gtk_window_unminimize(main_window); // Ensure we unminimize the window
			gtk_window_present(main_window); // Present our main window
			return;
		}

		if (g_strcmp0(method_name, "Quit") == 0) { // Quit the application
			gtk_application_remove_window(app, main_window); // Remove the window, thereby closing the app
			return;
		}
	} else if (g_strcmp0(interface_name, "org.mpris.MediaPlayer2.Player") == 0) { // Root mediaplayer2 interface
		if (g_strcmp0(method_name, "Next") == 0) { // Next track
			koto_playback_engine_forwards(playback_engine);
			return;
		}

		if (g_strcmp0(method_name, "Previous") == 0) { // Previous track
			koto_playback_engine_backwards(playback_engine);
			return;
		}

		if (g_strcmp0(method_name, "Pause") == 0) { // Explicit pause
			koto_playback_engine_pause(playback_engine);
			return;
		}

		if (g_strcmp0(method_name, "PlayPause") == 0) { // Explicit playpause
			koto_playback_engine_toggle(playback_engine);
			return;
		}

		if (g_strcmp0(method_name, "Stop") == 0) { // Explicit stop
			koto_playback_engine_stop(playback_engine);
			return;
		}

		if (g_strcmp0(method_name, "Play") == 0) { // Explicit play
			koto_playback_engine_play(playback_engine);
			return;
		}
	}
}

GVariant * handle_get_property(
	GDBusConnection * connection,
	const gchar * sender,
	const gchar * object_path,
	const gchar * interface_name,
	const gchar * property_name,
	GError ** error,
	gpointer user_data
) {
	(void) connection;
	(void) sender;
	(void) object_path;
	(void) interface_name;
	(void) error;
	(void) user_data;
	GVariant * ret;


	ret = NULL;

	if (g_strcmp0(property_name, "CanQuit") == 0) { // If property is CanQuit
		ret = g_variant_new_boolean(TRUE); // Allow quitting. You can escape Hotel California for now.
	}

	if (g_strcmp0(property_name, "CanRaise") == 0) { // If property is CanRaise
		ret = g_variant_new_boolean(TRUE); // Allow raising.
	}

	if (g_strcmp0(property_name, "HasTrackList") == 0) { // If property is HasTrackList
		KotoPlaylist * playlist = koto_current_playlist_get_playlist(current_playlist);
		if (KOTO_IS_PLAYLIST(playlist)) {
			ret = g_variant_new_boolean(koto_playlist_get_length(playlist) > 0);
		} else { // Don't have a playlist
			ret = g_variant_new_boolean(FALSE);
		}
	}

	if (g_strcmp0(property_name, "Identity") == 0) { // Want identity
		ret = g_variant_new_string("Koto"); // Not Jason Bourne but close enough
	}

	if (g_strcmp0(property_name, "DesktopEntry") == 0) { // Desktop Entry
		ret = g_variant_new_string(koto_rev_dns);
	}

	if (g_strcmp0(property_name, "SupportedUriSchemas") == 0) { // Supported URI Schemas
		GVariantBuilder * builder = g_variant_builder_new(G_VARIANT_TYPE("as")); // Array of strings
		g_variant_builder_add(builder, "s", "file");
		ret = g_variant_new("as", builder);
		g_variant_builder_unref(builder); // Unref builder since we no longer need it
	}

	if (g_strcmp0(property_name, "SupportedMimeTypes") == 0) { // Supported mimetypes
		GVariantBuilder * builder = g_variant_builder_new(G_VARIANT_TYPE("as")); // Array of strings
		GList * mimes;
		mimes = NULL;

		for (mimes = supported_mimes; mimes != NULL; mimes = mimes->next) { // For each mimetype
			g_variant_builder_add(builder, "s", mimes->data); // Add the mime as a string
		}

		ret = g_variant_new("as", builder);
		g_variant_builder_unref(builder);
		g_list_free(mimes);
	}

	if (g_strcmp0(property_name, "Metadata") == 0) { // Metadata
		KotoTrack * current_track = koto_playback_engine_get_current_track(playback_engine);

		if (KOTO_IS_TRACK(current_track)) { // Currently playing a track
			ret = koto_track_get_metadata_vardict(current_track);
		} else { // No track
			GVariantBuilder * builder = g_variant_builder_new(G_VARIANT_TYPE_VARDICT); // Create an empty builder
			ret = g_variant_builder_end(builder); // return the vardict
		}
	}

	if (
		(g_strcmp0(property_name, "CanPlay") == 0) ||
		(g_strcmp0(property_name, "CanPause") == 0)
	) {
		KotoTrack * current_track = koto_playback_engine_get_current_track(playback_engine);
		ret = g_variant_new_boolean(KOTO_IS_TRACK(current_track));
	}

	if (g_strcmp0(property_name, "CanSeek") == 0) { // Can control position over mpris
		ret = g_variant_new_boolean(FALSE);
	}

	if (g_strcmp0(property_name, "CanGoNext") == 0) { // Can Go Next
		// TODO: Add some actual logic here
		ret = g_variant_new_boolean(TRUE);
	}

	if (g_strcmp0(property_name, "CanGoPrevious") == 0) { // Can Go Previous
		// TODO: Add some actual logic here
		ret = g_variant_new_boolean(TRUE);
	}

	if (g_strcmp0(property_name, "CanControl") == 0) { // Can Control
		ret = g_variant_new_boolean(TRUE);
	}

	if (g_strcmp0(property_name, "PlaybackStatus") == 0) { // Get our playback status
		GstState current_state = koto_playback_engine_get_state(playback_engine); // Get the current state

		if (current_state == GST_STATE_PLAYING) {
			ret = g_variant_new_string("Playing");
		} else if (current_state == GST_STATE_PAUSED) {
			ret = g_variant_new_string("Paused");
		} else {
			ret = g_variant_new_string("Stopped");
		}
	}

	return ret;
}

gboolean handle_set_property(
	GDBusConnection * connection,
	const gchar * sender,
	const gchar * object_path,
	const gchar * interface_name,
	const gchar * property_name,
	GVariant * value,
	GError ** error,
	gpointer user_data
) {
	(void) connection;
	(void) sender;
	(void) interface_name;
	(void) object_path;
	(void) error;
	(void) user_data;

	if (g_strcmp0(property_name, "LoopStatus") == 0) { // Changing LoopStatus
		koto_playback_engine_set_track_repeat(playback_engine, g_variant_get_boolean(value)); // Set the loop status state
		return TRUE;
	}

	if (g_strcmp0(property_name, "Shuffle") == 0) { // Changing Shuffle
		koto_playback_engine_set_track_shuffle(playback_engine, g_variant_get_boolean(value)); // Set the shuffle state
		return TRUE;
	}

	return FALSE;
}

void koto_update_mpris_playback_state(GstState state) {
	GVariantBuilder * builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);


	if (state == GST_STATE_PLAYING) {
		g_variant_builder_add(builder, "{sv}", "PlaybackStatus", g_variant_new_string("Playing"));
	} else if (state == GST_STATE_PAUSED) {
		g_variant_builder_add(builder, "{sv}", "PlaybackStatus", g_variant_new_string("Paused"));
	} else {
		g_variant_builder_add(builder, "{sv}", "PlaybackStatus", g_variant_new_string("Stopped"));
	}

	g_dbus_connection_emit_signal(
		dbus_conn,
		NULL,
		"/org/mpris/MediaPlayer2",
		"org.freedesktop.DBus.Properties",
		"PropertiesChanged",
		g_variant_new("(sa{sv}as)", "org.mpris.MediaPlayer2.Player", builder, NULL),
		NULL
	);
}

void koto_update_mpris_info_for_track(KotoTrack * track) {
	if (!KOTO_IS_TRACK(track)) {
		return;
	}

	GVariant * metadata = koto_track_get_metadata_vardict(track); // Get the GVariantBuilder variable dict for the metadata


	koto_update_mpris_info_for_track_with_metadata(track, metadata);
}

void koto_update_mpris_info_for_track_with_metadata(
	KotoTrack * track,
	GVariant * metadata
) {
	if (!KOTO_IS_TRACK(track)) {
		return;
	}

	GVariantBuilder * builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);


	g_variant_builder_add(builder, "{sv}", "Metadata", metadata);

	g_dbus_connection_emit_signal(
		dbus_conn,
		NULL,
		"/org/mpris/MediaPlayer2",
		"org.freedesktop.DBus.Properties",
		"PropertiesChanged",
		g_variant_new("(sa{sv}as)", "org.mpris.MediaPlayer2.Player", builder, NULL),
		NULL
	);
}

static const GDBusInterfaceVTable main_mpris_interface_vtable = {
	handle_method_call,
	handle_get_property,
	handle_set_property,
	{ 0 }
};

void on_main_mpris_bus_acquired(
	GDBusConnection * connection,
	const gchar * name,
	gpointer user_data
) {
	(void) name;
	(void) user_data;
	dbus_conn = connection;
	g_dbus_connection_register_object(
		dbus_conn,
		"/org/mpris/MediaPlayer2",
		introspection_data->interfaces[0],
		&main_mpris_interface_vtable,
		NULL,
		NULL,
		NULL
	);

	g_dbus_connection_register_object(
		dbus_conn,
		"/org/mpris/MediaPlayer2",
		introspection_data->interfaces[1],
		&main_mpris_interface_vtable,
		NULL,
		NULL,
		NULL
	);
}

void setup_mpris_interfaces() {
	introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
	g_assert(introspection_data != NULL);

	mpris_bus_id = g_bus_own_name(
		G_BUS_TYPE_SESSION,
		"org.mpris.MediaPlayer2.koto",
		G_BUS_NAME_OWNER_FLAGS_NONE,
		on_main_mpris_bus_acquired,
		NULL,
		NULL,
		NULL,
		NULL
	);

	g_assert(mpris_bus_id > 0);
}
