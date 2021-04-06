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
#include "../db/cartographer.h"
#include "../playlist/current.h"
#include "../indexer/structs.h"
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

static glong NS = 1000000000;
static guint playback_engine_signals[N_SIGNALS] = { 0 };

extern KotoCartographer *koto_maps;
extern KotoCurrentPlaylist *current_playlist;

KotoPlaybackEngine *playback_engine;

struct _KotoPlaybackEngine {
	GObject parent_class;
	GstElement *player;
	GstElement *playbin;
	GstElement *suppress_video;
	GstBus *monitor;

	KotoIndexedTrack *current_track;

	gboolean is_muted;
	gboolean is_repeat_enabled;

	gboolean is_playing;

	gboolean tick_duration_timer_running;
	gboolean tick_track_timer_running;

	guint playback_position;
	gdouble volume;
};

struct _KotoPlaybackEngineClass {
	GObjectClass parent_class;

	void (* is_playing) (KotoPlaybackEngine *engine);
	void (* is_paused) (KotoPlaybackEngine *engine);
	void (* play_state_changed) (KotoPlaybackEngine *engine);
	void (* tick_duration) (KotoPlaybackEngine *engine);
	void (* tick_track) (KotoPlaybackEngine *engine);
	void (* track_changed) (KotoPlaybackEngine *engine);
	void (* track_repeat_changed) (KotoPlaybackEngine *engine);
	void (* track_shuffle_changed) (KotoPlaybackEngine *engine);
};

G_DEFINE_TYPE(KotoPlaybackEngine, koto_playback_engine, G_TYPE_OBJECT);

static void koto_playback_engine_class_init(KotoPlaybackEngineClass *c) {
	c->play_state_changed = NULL;

	GObjectClass *gobject_class;
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

static void koto_playback_engine_init(KotoPlaybackEngine *self) {
	self->current_track = NULL;

	self->player = gst_pipeline_new("player");
	self->playbin = gst_element_factory_make("playbin", NULL);
	self->suppress_video = gst_element_factory_make("fakesink", "suppress-video");

	g_object_set(self->playbin, "video-sink", self->suppress_video, NULL);
	self->volume = 0.5;
	koto_playback_engine_set_volume(self, 0.5);

	gst_bin_add(GST_BIN(self->player), self->playbin);
	self->monitor = gst_bus_new(); // Get the bus for the playbin

	if (GST_IS_BUS(self->monitor)) {
		gst_bus_add_watch(self->monitor, koto_playback_engine_monitor_changed, self);
		gst_element_set_bus(self->player, self->monitor); // Set our bus to monitor changes
	}

	self->is_muted = FALSE;
	self->is_repeat_enabled = FALSE;
	self->tick_duration_timer_running = FALSE;
	self->tick_track_timer_running = FALSE;

	if (KOTO_IS_CURRENT_PLAYLIST(current_playlist)) {
		g_signal_connect(current_playlist, "notify::current-playlist", G_CALLBACK(koto_playback_engine_current_playlist_changed), NULL);
	}
}

void koto_playback_engine_backwards(KotoPlaybackEngine *self) {
	KotoPlaylist *playlist = koto_current_playlist_get_playlist(current_playlist); // Get the current playlist

	if (!KOTO_IS_PLAYLIST(playlist)) { // If we do not have a playlist currently
		return;
	}

	koto_playback_engine_set_track_by_uuid(self, koto_playlist_go_to_previous(playlist));
}

void koto_playback_engine_current_playlist_changed() {
	if (!KOTO_IS_PLAYBACK_ENGINE(playback_engine)) {
		return;
	}

	KotoPlaylist *playlist = koto_current_playlist_get_playlist(current_playlist); // Get the current playlist

	if (!KOTO_IS_PLAYLIST(playlist)) { // If we do not have a playlist currently
		return;
	}

	koto_playback_engine_set_track_by_uuid(playback_engine, koto_playlist_go_to_next(playlist)); // Go to "next" which is the first track
}

void koto_playback_engine_forwards(KotoPlaybackEngine *self) {
	KotoPlaylist *playlist = koto_current_playlist_get_playlist(current_playlist); // Get the current playlist

	if (!KOTO_IS_PLAYLIST(playlist)) { // If we do not have a playlist currently
		return;
	}

	if (self->is_repeat_enabled) { // Is repeat enabled
		koto_playback_engine_set_position(self, 0); // Set position back to 0 to repeat the track
	} else { // Repeat not enabled
		koto_playback_engine_set_track_by_uuid(self, koto_playlist_go_to_next(playlist));
	}
}

KotoIndexedTrack* koto_playback_engine_get_current_track(KotoPlaybackEngine *self) {
	return self->current_track;
}

gint64 koto_playback_engine_get_duration(KotoPlaybackEngine *self) {
	gint64 duration = 0;
	if (gst_element_query_duration(self->player, GST_FORMAT_TIME, &duration)) {
		duration = duration / NS; // Divide by NS to get seconds
	}

	return duration;
}

gint64 koto_playback_engine_get_progress(KotoPlaybackEngine *self) {
	gint64 progress = 0;
	if (gst_element_query_position(self->player, GST_FORMAT_TIME, &progress)) {
		progress = progress / NS; // Divide by NS to get seconds
	}

	return progress;
}

GstState koto_playback_engine_get_state(KotoPlaybackEngine *self) {
	GstState current_state;
	gst_element_get_state(self->player, &current_state, NULL, GST_SECOND); // Get the current state, allowing up to a second to get it

	return current_state;
}

gboolean koto_playback_engine_get_track_repeat(KotoPlaybackEngine *self) {
	return self->is_repeat_enabled;
}

gboolean koto_playback_engine_get_track_shuffle(KotoPlaybackEngine *self) {
	(void) self;
	KotoPlaylist *playlist = koto_current_playlist_get_playlist(current_playlist);

	if (!KOTO_IS_PLAYLIST(playlist)) { // Don't have a playlist currently
		return FALSE;
	}

	gboolean currently_shuffling = FALSE;
	g_object_get(playlist, "is-shuffle-enabled", &currently_shuffling, NULL); // Get the current is-shuffle-enabled

	return currently_shuffling;
}

gboolean koto_playback_engine_monitor_changed(GstBus *bus, GstMessage *msg, gpointer user_data) {
	(void) bus;
	KotoPlaybackEngine *self = user_data;

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

void koto_playback_engine_play(KotoPlaybackEngine *self) {
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

void koto_playback_engine_pause(KotoPlaybackEngine *self) {
	self->is_playing = FALSE;
	gst_element_change_state(self->player, GST_STATE_CHANGE_PLAYING_TO_PAUSED);
	koto_update_mpris_playback_state(GST_STATE_PAUSED);
}

void koto_playback_engine_set_position(KotoPlaybackEngine *self, int position) {
	gst_element_seek_simple(self->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, position * NS);
}

void koto_playback_engine_set_track_repeat(KotoPlaybackEngine *self, gboolean enable_repeat) {
	self->is_repeat_enabled = enable_repeat;
	g_signal_emit(self, playback_engine_signals[SIGNAL_TRACK_REPEAT_CHANGE], 0); // Emit our track repeat changed event
}

void koto_playback_engine_set_track_shuffle(KotoPlaybackEngine *self, gboolean enable_shuffle) {
	KotoPlaylist *playlist = koto_current_playlist_get_playlist(current_playlist);

	if (!KOTO_IS_PLAYLIST(playlist)) { // Don't have a playlist currently
		return;
	}

	g_object_set(playlist, "is-shuffle-enabled", enable_shuffle, NULL); // Set the is-shuffle-enabled on any existing playlist
	g_signal_emit(self, playback_engine_signals[SIGNAL_TRACK_SHUFFLE_CHANGE], 0); // Emit our track shuffle changed event
}

void koto_playback_engine_set_track_by_uuid(KotoPlaybackEngine *self, gchar *track_uuid) {
	if (track_uuid == NULL) {
		return;
	}

	KotoIndexedTrack *track = koto_cartographer_get_track_by_uuid(koto_maps, track_uuid); // Get the track from cartographer

	if (!KOTO_IS_INDEXED_TRACK(track)) { // Not a track
		return;
	}

	self->current_track = track;

	gchar *track_file_path = NULL;
	g_object_get(track, "path", &track_file_path, NULL); // Get the path to the track

	koto_playback_engine_stop(self); // Stop current track

	gchar *gst_filename = gst_filename_to_uri(track_file_path, NULL); // Get the gst supported file naem

	g_object_set(self->playbin, "uri", gst_filename, NULL);
	g_free(gst_filename); // Free the filename

	koto_playback_engine_play(self); // Play the new track

	// TODO: Add prior position state setting here, like picking up at a specific part of an audiobook or podcast
	koto_playback_engine_set_position(self, 0);
	koto_playback_engine_set_volume(self, self->volume); // Re-enforce our volume on the updated playbin

	g_signal_emit(self, playback_engine_signals[SIGNAL_TRACK_CHANGE], 0); // Emit our track change signal
	koto_update_mpris_info_for_track(self->current_track);
}

void koto_playback_engine_set_volume(KotoPlaybackEngine *self, gdouble volume) {
	self->volume = volume;
	g_object_set(self->playbin, "volume", self->volume, NULL);
}

void koto_playback_engine_stop(KotoPlaybackEngine *self) {
	gst_element_set_state(self->player, GST_STATE_NULL);
	GstPad *pad = gst_element_get_static_pad(self->player, "sink"); // Get the static pad of the audio element

	if (!GST_IS_PAD(pad)) {
		return;
	}

	gst_pad_set_offset(pad, 0); // Change offset
	koto_update_mpris_playback_state(GST_STATE_NULL);
}

void koto_playback_engine_toggle(KotoPlaybackEngine *self) {
	if (self->is_playing) { // Currently playing
		koto_playback_engine_pause(self); // Pause
	} else {
		koto_playback_engine_play(self); // Play
	}
}

gboolean koto_playback_engine_tick_duration(gpointer user_data) {
	KotoPlaybackEngine *self = user_data;

	if (self->is_playing) { // Is playing
		g_signal_emit(self, playback_engine_signals[SIGNAL_TICK_DURATION], 0); // Emit our 1s track tick
	} else { // Not playing so exiting timer
		self->tick_duration_timer_running = FALSE;
	}

	return self->is_playing;
}

gboolean koto_playback_engine_tick_track(gpointer user_data) {
	KotoPlaybackEngine *self = user_data;

	if (self->is_playing) { // Is playing
		g_signal_emit(self, playback_engine_signals[SIGNAL_TICK_TRACK], 0); // Emit our 100ms track tick
	} else {
		self->tick_track_timer_running = FALSE;
	}

	return self->is_playing;
}

void koto_playback_engine_toggle_track_repeat(KotoPlaybackEngine *self) {
	koto_playback_engine_set_track_repeat(self, !self->is_repeat_enabled);
}

void koto_playback_engine_toggle_track_shuffle(KotoPlaybackEngine *self) {
	koto_playback_engine_set_track_shuffle(self, !koto_playback_engine_get_track_shuffle(self)); // Invert the currently shuffling vale
}

KotoPlaybackEngine* koto_playback_engine_new() {
	return g_object_new(KOTO_TYPE_PLAYBACK_ENGINE, NULL);
}
