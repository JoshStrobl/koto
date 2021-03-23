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

enum {
	SIGNAL_DURATION_CHANGE,
	SIGNAL_PLAY_STATE_CHANGE,
	SIGNAL_PROGRESS_CHANGE,
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

	gboolean is_paused;
	gboolean is_playing;
	gboolean is_muted;
	gboolean is_repeat_enabled;
	gboolean is_shuffle_enabled;
	gboolean requested_playing;
	guint playback_position;
	gdouble volume;
};

struct _KotoPlaybackEngineClass {
	GObjectClass parent_class;

	void (* duration_changed) (KotoPlaybackEngine *engine, gint64 duration);
	void (* play_state_changed) (KotoPlaybackEngine *engine);
	void (* progress_changed) (KotoPlaybackEngine *engine, gint64 progress);
};

G_DEFINE_TYPE(KotoPlaybackEngine, koto_playback_engine, G_TYPE_OBJECT);

static void koto_playback_engine_class_init(KotoPlaybackEngineClass *c) {
	c->play_state_changed = NULL;

	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);

	playback_engine_signals[SIGNAL_DURATION_CHANGE] = g_signal_new(
		"duration-changed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, duration_changed),
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

	playback_engine_signals[SIGNAL_PROGRESS_CHANGE] = g_signal_new(
		"progress-changed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaybackEngineClass, progress_changed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);
}

static void koto_playback_engine_init(KotoPlaybackEngine *self) {
	self->player = gst_pipeline_new("player");
	self->playbin = gst_element_factory_make("playbin", NULL);
	self->suppress_video = gst_element_factory_make("fakesink", "suppress-video");

	g_object_set(self->playbin, "video-sink", self->suppress_video, NULL);
	g_object_set(self->playbin, "volume", 0.5, NULL);

	gst_bin_add(GST_BIN(self->player), self->playbin);
	self->monitor = gst_bus_new(); // Get the bus for the playbin

	if (GST_IS_BUS(self->monitor)) {
		gst_bus_add_watch(self->monitor, koto_playback_engine_monitor_changed, self);
		gst_element_set_bus(self->playbin, self->monitor); // Set our bus to monitor changes
	}

	self->is_muted = FALSE;
	self->is_playing = FALSE;
	self->is_paused = FALSE;
	self->is_repeat_enabled = FALSE;
	self->is_shuffle_enabled = FALSE;
	self->requested_playing = FALSE;

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

	koto_playback_engine_set_track_by_uuid(playback_engine, koto_playlist_get_current_uuid(playlist)); // Get the current UUID
}

void koto_playback_engine_forwards(KotoPlaybackEngine *self) {
	KotoPlaylist *playlist = koto_current_playlist_get_playlist(current_playlist); // Get the current playlist

	if (!KOTO_IS_PLAYLIST(playlist)) { // If we do not have a playlist currently
		return;
	}

	koto_playback_engine_set_track_by_uuid(self, koto_playlist_go_to_next(playlist));
}

gint64 koto_playback_engine_get_duration(KotoPlaybackEngine *self) {
	gint64 duration = 0;
	if (gst_element_query_duration(self->playbin, GST_FORMAT_TIME, &duration)) {
		duration = duration / NS; // Divide by NS to get seconds
	}

	return duration;
}

GstState koto_playback_engine_get_state(KotoPlaybackEngine *self) {
	GstState current_state;
	GstStateChangeReturn ret = gst_element_get_state(self->playbin, &current_state, NULL, GST_SECOND); // Get the current state, allowing up to a second to get it

	if (ret != GST_STATE_CHANGE_SUCCESS) { // Got the data we need
		return GST_STATE_NULL;
	}

	return current_state;
}

gint64 koto_playback_engine_get_progress(KotoPlaybackEngine *self) {
	gint64 progress = 0;
	if (gst_element_query_position(self->playbin, GST_FORMAT_TIME, &progress)) {
		progress = progress / NS; // Divide by NS to get seconds
	}

	return progress;
}

gboolean koto_playback_engine_monitor_changed(GstBus *bus, GstMessage *msg, gpointer user_data) {
	(void) bus;
	KotoPlaybackEngine *self = user_data;

	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ASYNC_DONE: { // Finished loading
			if (self->requested_playing) {
				self->is_playing = TRUE;
				self->is_paused = FALSE;
				g_timeout_add(50, koto_playback_engine_update_progress, self);
				gst_element_set_state(self->playbin, GST_STATE_PLAYING); // Make double sure the state is playing
				g_signal_emit(self, playback_engine_signals[SIGNAL_DURATION_CHANGE], 0); // Emit our duration signal
				g_signal_emit(self, playback_engine_signals[SIGNAL_PLAY_STATE_CHANGE], 0); // Emit our playing signal
			}
			break;
		}
		case GST_MESSAGE_STATE_CHANGED: { // State changed
			GstState old_state;
			GstState requested_state;
			GstState new_state;
			gst_message_parse_state_changed(msg, &old_state, &requested_state, &new_state);

			if ((old_state == GST_STATE_PLAYING) && (requested_state == GST_STATE_PAUSED)) {
				self->is_playing = FALSE;
				self->is_paused = TRUE;

				g_signal_emit(self, playback_engine_signals[SIGNAL_PLAY_STATE_CHANGE], 0); // Emit our paused signal
			} else if (requested_state == GST_STATE_PLAYING && !self->is_playing) {
				self->is_playing = TRUE;
				self->is_paused = FALSE;
				g_signal_emit(self, playback_engine_signals[SIGNAL_PLAY_STATE_CHANGE], 0); // Emit our paused signal
			} else if (((old_state == GST_STATE_PLAYING) || (old_state == GST_STATE_PAUSED)) && (new_state == GST_STATE_NULL)) { // If we're freeing resources
				gst_bus_post(self->monitor, gst_message_new_reset_time(GST_OBJECT(self->playbin), 0));
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
	self->requested_playing = TRUE;
	gst_element_set_state(self->playbin, GST_STATE_PLAYING); // Set our state to play
}

void koto_playback_engine_pause(KotoPlaybackEngine *self) {
	self->requested_playing = FALSE;
	gst_element_set_state(self->playbin, GST_STATE_PAUSED); // Set our state to paused
}

void koto_playback_engine_set_track_by_uuid(KotoPlaybackEngine *self, gchar *track_uuid) {
	if (track_uuid == NULL) {
		return;
	}

	KotoIndexedTrack *track = koto_cartographer_get_track_by_uuid(koto_maps, track_uuid); // Get the track from cartographer

	if (!KOTO_IS_INDEXED_TRACK(track)) { // Not a track
		return;
	}

	gchar *track_file_path = NULL;
	g_object_get(track, "path", &track_file_path, NULL); // Get the path to the track

	koto_playback_engine_stop(self); // Stop current track

	gchar *gst_filename = gst_filename_to_uri(track_file_path, NULL); // Get the gst supported file naem

	g_object_set(self->playbin, "uri", gst_filename, NULL);
	g_free(gst_filename); // Free the filename

	koto_playback_engine_play(self); // Play the new track
}

void koto_playback_engine_stop(KotoPlaybackEngine *self) {
	gst_element_set_state(self->playbin, GST_STATE_NULL);
	GstPad *pad = gst_element_get_static_pad(self->playbin, "audio-sink"); // Get the static pad of the audio element

	if (!GST_IS_PAD(pad)) {
		return;
	}

	gst_pad_set_offset(pad, 0); // Change offset
}

void koto_playback_engine_toggle(KotoPlaybackEngine *self) {
	if (koto_playback_engine_get_state(self) == GST_STATE_PLAYING) { // Currently playing
		koto_playback_engine_pause(self); // Pause
	} else {
		koto_playback_engine_play(self); // Play
	}
}

gboolean koto_playback_engine_update_progress(gpointer user_data) {
	KotoPlaybackEngine *self = user_data;
	if (self->is_playing) { // Is playing
		g_signal_emit(self, playback_engine_signals[SIGNAL_PROGRESS_CHANGE], 0); // Emit our progress change signal
	}

	return self->is_playing;
}

KotoPlaybackEngine* koto_playback_engine_new() {
	return g_object_new(KOTO_TYPE_PLAYBACK_ENGINE, NULL);
}
