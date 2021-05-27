/* engine.c
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
#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/player/player.h>
#include <gtk-4.0/gtk/gtk.h>
#include "../config/config.h"
#include "../db/cartographer.h"
#include "../playlist/current.h"
#include "../indexer/structs.h"
#include "../koto-paths.h"
#include "../koto-utils.h"
#include "engine.h"
#include "mpris.h"

enum {
	SIGNAL_IS_PLAYING,
	SIGNAL_IS_PAUSED,
	SIGNAL_PLAY_STATE_CHANGE,
	SIGNAL_TICK_DURATION,
	SIGNAL_TICK_TRACK,
	SIGNAL_TRACK_CHANGE,
	SIGNAL_TRACK_REPEAT_CHANGE,
	SIGNAL_TRACK_SHUFFLE_CHANGE,
	N_SIGNALS
};

extern gchar * koto_rev_dns;

static guint playback_engine_signals[N_SIGNALS] = {
	0
};

extern KotoCartographer * koto_maps;
extern KotoConfig * config;
extern KotoCurrentPlaylist * current_playlist;

KotoPlaybackEngine * playback_engine;

struct _KotoPlaybackEngine {
	GObject parent_class;
	GstElement * player;
	GstElement * playbin;
	GstElement * suppress_video;
	GstBus * monitor;

	GstQuery * duration_query;
	GstQuery * position_query;

	KotoTrack * current_track;

	gboolean is_muted;
	gboolean is_repeat_enabled;

	gboolean is_playing;
	gboolean is_playing_specific_track;
	gboolean is_shuffle_enabled;

	gboolean tick_duration_timer_running;
	gboolean tick_track_timer_running;

	gboolean via_config_continue_on_playlist; // Pulls from our Koto Config
	gboolean via_config_maintain_shuffle; // Pulls from our Koto Config

	guint playback_position;
	gdouble volume;
};

struct _KotoPlaybackEngineClass {
	GObjectClass parent_class;

	void (* is_playing) (KotoPlaybackEngine * engine);
	void (* is_paused) (KotoPlaybackEngine * engine);
	void (* play_state_changed) (KotoPlaybackEngine * engine);
	void (* tick_duration) (KotoPlaybackEngine * engine);
	void (* tick_track) (KotoPlaybackEngine * engine);
	void (* track_changed) (KotoPlaybackEngine * engine);
	void (* track_repeat_changed) (KotoPlaybackEngine * engine);
	void (* track_shuffle_changed) (KotoPlaybackEngine * engine);
};

G_DEFINE_TYPE(KotoPlaybackEngine, koto_playback_engine, G_TYPE_OBJECT);

static void koto_playback_engine_class_init(KotoPlaybackEngineClass * c) {
	c->play_state_changed = NULL;
	GObjectClass * gobject_class;
	gobject_class = G_OBJECT_CLASS(c);

	playback_engine_signals[SIGNAL_IS_PLAYING] = g_signal_new(
		"is-playing",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, play_state_changed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	playback_engine_signals[SIGNAL_IS_PAUSED] = g_signal_new(
		"is-paused",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, play_state_changed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	playback_engine_signals[SIGNAL_PLAY_STATE_CHANGE] = g_signal_new(
		"play-state-changed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, play_state_changed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	playback_engine_signals[SIGNAL_TICK_DURATION] = g_signal_new(
		"tick-duration",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, tick_duration),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	playback_engine_signals[SIGNAL_TICK_TRACK] = g_signal_new(
		"tick-track",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, tick_track),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	playback_engine_signals[SIGNAL_TRACK_CHANGE] = g_signal_new(
		"track-changed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, track_changed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	playback_engine_signals[SIGNAL_TRACK_REPEAT_CHANGE] = g_signal_new(
		"track-repeat-changed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, track_repeat_changed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	playback_engine_signals[SIGNAL_TRACK_SHUFFLE_CHANGE] = g_signal_new(
		"track-shuffle-changed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, track_shuffle_changed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);
}

static void koto_playback_engine_init(KotoPlaybackEngine * self) {
	self->current_track = NULL;

	self->player = gst_pipeline_new("player");
	self->playbin = gst_element_factory_make("playbin", NULL);
	self->suppress_video = gst_element_factory_make("fakesink", "suppress-video");

	g_object_set(self->playbin, "video-sink", self->suppress_video, NULL);
	self->volume = 0.5;
	koto_playback_engine_set_volume(self, 0.5);

	gst_bin_add(GST_BIN(self->player), self->playbin);
	self->monitor = gst_bus_new(); // Get the bus for the playbin

	self->duration_query = gst_query_new_duration(GST_FORMAT_TIME); // Create our re-usable query for querying the duration
	self->position_query = gst_query_new_position(GST_FORMAT_TIME); // Create our re-usable query for querying the position

	if (GST_IS_BUS(self->monitor)) {
		gst_bus_add_watch(self->monitor, koto_playback_engine_monitor_changed, self);
		gst_element_set_bus(self->player, self->monitor); // Set our bus to monitor changes
	}

	self->is_muted = FALSE;
	self->is_playing = FALSE;
	self->is_playing_specific_track = FALSE;
	self->is_repeat_enabled = FALSE;
	self->is_shuffle_enabled = FALSE;
	self->tick_duration_timer_running = FALSE;
	self->tick_track_timer_running = FALSE;
	self->via_config_continue_on_playlist = FALSE;
	self->via_config_maintain_shuffle = TRUE;

	koto_playback_engine_apply_configuration_state(NULL, 0, self); // Apply configuration immediately

	g_signal_connect(config, "notify::playback-continue-on-playlist", G_CALLBACK(koto_playback_engine_apply_configuration_state), self); // Handle changes to our config
	g_signal_connect(config, "notify::last-used-volume", G_CALLBACK(koto_playback_engine_apply_configuration_state), self); // Handle changes to our config
	g_signal_connect(config, "notify::maintain-shuffle", G_CALLBACK(koto_playback_engine_apply_configuration_state), self); // Handle changes to our config

	if (KOTO_IS_CURRENT_PLAYLIST(current_playlist)) {
		g_signal_connect(current_playlist, "notify::current-playlist", G_CALLBACK(koto_playback_engine_current_playlist_changed), self);
	}
}

void koto_playback_engine_apply_configuration_state(
	KotoConfig * c,
	guint prop_id,
	KotoPlaybackEngine * self
) {
	(void) c;
	(void) prop_id;

	gboolean config_continue_on_playlist = self->via_config_continue_on_playlist;
	gdouble config_last_used_volume = 0.5;
	gboolean config_maintain_shuffle = self->via_config_maintain_shuffle;

	g_object_get(
		config,
		"playback-continue-on-playlist",
		&config_continue_on_playlist,
		"playback-last-used-volume",
		&config_last_used_volume,
		"playback-maintain-shuffle",
		&config_maintain_shuffle,
		NULL
	);

	self->via_config_continue_on_playlist = config_continue_on_playlist;
	self->via_config_maintain_shuffle = config_maintain_shuffle;

	if (self->volume != config_last_used_volume) { // Not the same volume
		koto_playback_engine_set_volume(self, config_last_used_volume);
	}
}

void koto_playback_engine_backwards(KotoPlaybackEngine * self) {
	KotoPlaylist * playlist = koto_current_playlist_get_playlist(current_playlist); // Get the current playlist


	if (!KOTO_IS_PLAYLIST(playlist)) { // If we do not have a playlist currently
		return;
	}

	if (self->is_repeat_enabled || self->is_playing_specific_track) { // Repeat enabled or playing a specific track
		return;
	}

	koto_playback_engine_set_track_by_uuid(self, koto_playlist_go_to_previous(playlist), FALSE);
}

void koto_playback_engine_current_playlist_changed(
	KotoCurrentPlaylist * current_pl,
	guint prop_id,
	KotoPlaybackEngine * self
) {
	(void) current_pl;
	(void) prop_id;

	if (!KOTO_IS_PLAYBACK_ENGINE(self)) {
		return;
	}

	KotoPlaylist * playlist = koto_current_playlist_get_playlist(current_playlist); // Get the current playlist

	if (!KOTO_IS_PLAYLIST(playlist)) { // If we do not have a playlist currently
		return;
	}

	if (self->is_shuffle_enabled) { // If shuffle is enabled
		koto_playback_engine_set_track_shuffle(self, self->via_config_maintain_shuffle); // Set to our maintain shuffle value
	}

	koto_playback_engine_set_track_by_uuid(playback_engine, koto_playlist_go_to_next(playlist), FALSE); // Go to "next" which is the first track
}

void koto_playback_engine_forwards(KotoPlaybackEngine * self) {
	KotoPlaylist * playlist = koto_current_playlist_get_playlist(current_playlist); // Get the current playlist

	if (!KOTO_IS_PLAYLIST(playlist)) { // If we do not have a playlist currently
		return;
	}

	if (self->is_repeat_enabled) { // Is repeat enabled
		koto_playback_engine_set_position(self, 0); // Set position back to 0 to repeat the track
	} else { // If repeat is not enabled
		if (
			(self->via_config_continue_on_playlist && self->is_playing_specific_track) || // Playing a specific track and wanting to continue on the playlist
			(!self->is_playing_specific_track) // Not playing a specific track
		) {
			koto_playback_engine_set_track_by_uuid(self, koto_playlist_go_to_next(playlist), FALSE);
		}
	}
}

KotoTrack * koto_playback_engine_get_current_track(KotoPlaybackEngine * self) {
	return self->current_track;
}

gint64 koto_playback_engine_get_duration(KotoPlaybackEngine * self) {
	gint64 duration = 0;


	if (gst_element_query(self->player, self->duration_query)) { // Able to query our duration
		gst_query_parse_duration(self->duration_query, NULL, &duration); // Get the duration
		duration = duration / GST_SECOND; // Divide by NS to get seconds
	}

	return duration;
}

gdouble koto_playback_engine_get_progress(KotoPlaybackEngine * self) {
	gdouble progress = 0.0;
	gint64 gstprog = 0;


	if (gst_element_query(self->playbin, self->position_query)) { // Able to get our position
		gst_query_parse_position(self->position_query, NULL, &gstprog); // Get the progress

		if (gstprog < 1) { // Less than a second
			return 0.0;
		}

		progress = gstprog / GST_SECOND; // Divide by GST_SECOND then again by 100.
	}

	return progress;
}

GstState koto_playback_engine_get_state(KotoPlaybackEngine * self) {
	return GST_STATE(self->player);
}

gboolean koto_playback_engine_get_track_repeat(KotoPlaybackEngine * self) {
	return self->is_repeat_enabled;
}

gboolean koto_playback_engine_get_track_shuffle(KotoPlaybackEngine * self) {
	return self->is_shuffle_enabled;
}

gdouble koto_playback_engine_get_volume(KotoPlaybackEngine * self) {
	return self->volume;
}

gboolean koto_playback_engine_monitor_changed(
	GstBus * bus,
	GstMessage * msg,
	gpointer user_data
) {
	(void) bus;
	KotoPlaybackEngine * self = user_data;


	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ASYNC_DONE:
		case GST_MESSAGE_DURATION_CHANGED: { // Duration changed
			koto_playback_engine_tick_duration(self);
			koto_playback_engine_tick_track(self);
			break;
		}
		case GST_MESSAGE_STATE_CHANGED: { // State changed
			GstState old_state;
			GstState new_state;
			GstState pending_state;
			gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

			if (new_state == GST_STATE_PLAYING) { // Now playing
				koto_playback_engine_tick_duration(self);
				g_signal_emit(self, playback_engine_signals[SIGNAL_IS_PLAYING], 0); // Emit our is playing state signal
			} else if (new_state == GST_STATE_PAUSED) { // Now paused
				g_signal_emit(self, playback_engine_signals[SIGNAL_IS_PAUSED], 0); // Emit our is paused state signal
			}

			if (new_state != GST_STATE_VOID_PENDING) {
				g_signal_emit(self, playback_engine_signals[SIGNAL_PLAY_STATE_CHANGE], 0); // Emit our play state signal
			}

			break;
		}
		case GST_MESSAGE_EOS: { // Reached end of stream
			koto_playback_engine_forwards(self); // Go to the next track
			break;
		}
		default:
			break;
	}
	return TRUE;
}

void koto_playback_engine_play(KotoPlaybackEngine * self) {
	self->is_playing = TRUE;
	gst_element_set_state(self->player, GST_STATE_PLAYING); // Set our state to play

	if (!self->tick_duration_timer_running) {
		self->tick_duration_timer_running = TRUE;
		g_timeout_add(1000, koto_playback_engine_tick_duration, self); // Create a 1s duration tick
	}

	if (!self->tick_track_timer_running) {
		self->tick_track_timer_running = TRUE;
		g_timeout_add(100, koto_playback_engine_tick_track, self); // Create a 100ms track tick
	}

	koto_update_mpris_playback_state(GST_STATE_PLAYING);
}

void koto_playback_engine_pause(KotoPlaybackEngine * self) {
	self->is_playing = FALSE;
	gst_element_change_state(self->player, GST_STATE_CHANGE_PLAYING_TO_PAUSED);
	koto_update_mpris_playback_state(GST_STATE_PAUSED);
}

void koto_playback_engine_set_position(
	KotoPlaybackEngine * self,
	int position
) {
	gst_element_seek_simple(self->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, position * GST_SECOND);
}

void koto_playback_engine_set_track_repeat(
	KotoPlaybackEngine * self,
	gboolean enable_repeat
) {
	self->is_repeat_enabled = enable_repeat;
	g_signal_emit(self, playback_engine_signals[SIGNAL_TRACK_REPEAT_CHANGE], 0); // Emit our track repeat changed event
}

void koto_playback_engine_set_track_shuffle(
	KotoPlaybackEngine * self,
	gboolean enable_shuffle
) {
	KotoPlaylist * playlist = koto_current_playlist_get_playlist(current_playlist);

	if (!KOTO_IS_PLAYLIST(playlist)) { // Don't have a playlist currently
		return;
	}

	self->is_shuffle_enabled = enable_shuffle;
	g_object_set(playlist, "is-shuffle-enabled", self->is_shuffle_enabled, NULL); // Set the is-shuffle-enabled on any existing playlist
	g_signal_emit(self, playback_engine_signals[SIGNAL_TRACK_SHUFFLE_CHANGE], 0); // Emit our track shuffle changed event
}

void koto_playback_engine_set_track_by_uuid(
	KotoPlaybackEngine * self,
	gchar * track_uuid,
	gboolean playing_specific_track
) {
	if (track_uuid == NULL) {
		return;
	}

	KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, track_uuid); // Get the track from cartographer

	if (!KOTO_IS_TRACK(track)) { // Not a track
		return;
	}

	self->current_track = track;

	gchar * track_file_path = NULL;

	g_object_get(track, "path", &track_file_path, NULL); // Get the path to the track

	koto_playback_engine_stop(self); // Stop current track

	self->is_playing_specific_track = playing_specific_track;

	gchar * gst_filename = gst_filename_to_uri(track_file_path, NULL); // Get the gst supported file naem

	g_object_set(self->playbin, "uri", gst_filename, NULL);
	g_free(gst_filename); // Free the filename

	koto_playback_engine_play(self); // Play the new track

	// TODO: Add prior position state setting here, like picking up at a specific part of an audiobook or podcast
	koto_playback_engine_set_position(self, 0);
	koto_playback_engine_set_volume(self, self->volume); // Re-enforce our volume on the updated playbin

	GVariant * metadata = koto_track_get_metadata_vardict(track); // Get the GVariantBuilder variable dict for the metadata
	GVariantDict * metadata_dict = g_variant_dict_new(metadata);

	g_signal_emit(self, playback_engine_signals[SIGNAL_TRACK_CHANGE], 0); // Emit our track change signal
	koto_update_mpris_info_for_track(self->current_track);

	GVariant * track_name_var = g_variant_dict_lookup_value(metadata_dict, "xesam:title", NULL); // Get the GVariant for the name of the track
	const gchar * track_name = g_variant_get_string(track_name_var, NULL); // Get the string of the track name

	GVariant * album_name_var = g_variant_dict_lookup_value(metadata_dict, "xesam:album", NULL); // Get the GVariant for the album name
	const gchar * album_name = g_variant_get_string(album_name_var, NULL); // Get the string for the album name

	GVariant * artist_name_var = g_variant_dict_lookup_value(metadata_dict, "playbackengine:artist", NULL); // Get the GVariant for the name of the artist
	const gchar * artist_name = g_variant_get_string(artist_name_var, NULL); // Get the string for the artist name

	gchar * artist_album_combo = g_strjoin(" - ", artist_name, album_name, NULL); // Join artist and album name separated by " - "

	gchar * icon_name = "audio-x-generic-symbolic";

	if (g_variant_dict_contains(metadata_dict, "mpris:artUrl")) { // If we have artwork specified
		GVariant * art_url_var = g_variant_dict_lookup_value(metadata_dict, "mpris:artUrl", NULL); // Get the GVariant for the art URL
		const gchar * art_uri = g_variant_get_string(art_url_var, NULL); // Get the string for the artwork
		icon_name = koto_utils_replace_string_all(g_strdup(art_uri), "file://", "");
	}

	// Super important note: We are not using libnotify directly because the synchronous nature of notify_notification_send seems to result in dbus timeouts
	if (g_find_program_in_path("notify-send") != NULL) { // Have notify-send
		char * argv[14] = {
			"notify-send",
			"-a",
			"Koto",
			"-i",
			icon_name,
			"-c",
			"x-gnome.music",
			"-t",
			"5000",
			"-h",
			g_strdup_printf("string:desktop-entry:%s", koto_rev_dns),
			g_strdup(track_name),
			artist_album_combo,
			NULL
		};

		g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
	}
}

void koto_playback_engine_set_volume(
	KotoPlaybackEngine * self,
	gdouble volume
) {
	self->volume = volume;
	g_object_set(self->playbin, "volume", self->volume, NULL);
}

void koto_playback_engine_stop(KotoPlaybackEngine * self) {
	gst_element_set_state(self->player, GST_STATE_NULL);
	GstPad * pad = gst_element_get_static_pad(self->player, "sink"); // Get the static pad of the audio element


	if (!GST_IS_PAD(pad)) {
		return;
	}

	gst_pad_set_offset(pad, 0); // Change offset
	koto_update_mpris_playback_state(GST_STATE_NULL);
}

void koto_playback_engine_toggle(KotoPlaybackEngine * self) {
	if (self->is_playing) { // Currently playing
		koto_playback_engine_pause(self); // Pause
	} else {
		koto_playback_engine_play(self); // Play
	}
}

gboolean koto_playback_engine_tick_duration(gpointer user_data) {
	KotoPlaybackEngine * self = user_data;


	if (self->is_playing) { // Is playing
		g_signal_emit(self, playback_engine_signals[SIGNAL_TICK_DURATION], 0); // Emit our 1s track tick
	} else { // Not playing so exiting timer
		self->tick_duration_timer_running = FALSE;
	}

	return self->is_playing;
}

gboolean koto_playback_engine_tick_track(gpointer user_data) {
	KotoPlaybackEngine * self = user_data;

	if (self->is_playing) { // Is playing
		g_signal_emit(self, playback_engine_signals[SIGNAL_TICK_TRACK], 0); // Emit our 100ms track tick
	} else {
		self->tick_track_timer_running = FALSE;
	}

	return self->is_playing;
}

void koto_playback_engine_toggle_track_repeat(KotoPlaybackEngine * self) {
	koto_playback_engine_set_track_repeat(self, !self->is_repeat_enabled);
}

void koto_playback_engine_toggle_track_shuffle(KotoPlaybackEngine * self) {
	koto_playback_engine_set_track_shuffle(self, !self->is_shuffle_enabled); // Invert the currently shuffling vale
}

KotoPlaybackEngine * koto_playback_engine_new() {
	return g_object_new(KOTO_TYPE_PLAYBACK_ENGINE, NULL);
}
