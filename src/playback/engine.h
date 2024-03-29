/* engine.h
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

#pragma once
#include <glib-2.0/glib-object.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/player/player.h>
#include "../config/config.h"
#include "../playlist/current.h"

G_BEGIN_DECLS

/**
 * Type Definition
 **/

#define KOTO_TYPE_PLAYBACK_ENGINE (koto_playback_engine_get_type())
#define KOTO_PLAYBACK_ENGINE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), KOTO_TYPE_PLAYBACK_ENGINE, KotoPlaybackEngine))
#define KOTO_IS_PLAYBACK_ENGINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_PLAYBACK_ENGINE))

typedef struct _KotoPlaybackEngine KotoPlaybackEngine;
typedef struct _KotoPlaybackEngineClass KotoPlaybackEngineClass;

GLIB_AVAILABLE_IN_ALL
GType koto_playback_engine_get_type(void) G_GNUC_CONST;

/**
 * Playback Engine Functions
 **/

KotoPlaybackEngine * koto_playback_engine_new();

void koto_playback_engine_apply_configuration_state(
	KotoConfig * config,
	guint prop_id,
	KotoPlaybackEngine * self
);

void koto_playback_engine_backwards(KotoPlaybackEngine * self);

void koto_playback_engine_handle_current_playlist_changed(
	KotoCurrentPlaylist * current_pl,
	KotoPlaylist * playlist,
	gboolean play_immediately,
	gboolean play_current,
	gpointer user_data
);

void koto_playback_engine_forwards(KotoPlaybackEngine * self);

KotoTrack * koto_playback_engine_get_current_track(KotoPlaybackEngine * self);

gint64 koto_playback_engine_get_duration(KotoPlaybackEngine * self);

gdouble koto_playback_engine_get_progress(KotoPlaybackEngine * self);

;

gdouble koto_playback_engine_get_sane_playback_rate(gdouble rate);

GstState koto_playback_engine_get_state(KotoPlaybackEngine * self);

gboolean koto_playback_engine_get_track_repeat(KotoPlaybackEngine * self);

gboolean koto_playback_engine_get_track_shuffle(KotoPlaybackEngine * self);

gdouble koto_playback_engine_get_volume(KotoPlaybackEngine * self);

void koto_playback_engine_jump_backwards(KotoPlaybackEngine * self);

void koto_playback_engine_jump_forwards(KotoPlaybackEngine * self);

void koto_playback_engine_mute(KotoPlaybackEngine * self);

gboolean koto_playback_engine_monitor_changed(
	GstBus * bus,
	GstMessage * msg,
	gpointer user_data
);

void koto_playback_engine_pause(KotoPlaybackEngine * self);

void koto_playback_engine_play(KotoPlaybackEngine * self);

void koto_playback_engine_toggle(KotoPlaybackEngine * self);

void koto_playback_engine_set_playback_rate(
	KotoPlaybackEngine * self,
	gdouble rate
);

void koto_playback_engine_set_position(
	KotoPlaybackEngine * self,
	int position
);

void koto_playback_engine_set_track_playback_position(
	KotoPlaybackEngine * self,
	guint64 pos
);

void koto_playback_engine_set_track_repeat(
	KotoPlaybackEngine * self,
	gboolean enable_repeat
);

void koto_playback_engine_set_track_shuffle(
	KotoPlaybackEngine * self,
	gboolean enable_shuffle
);

void koto_playback_engine_set_track_by_uuid(
	KotoPlaybackEngine * self,
	gchar * track_uuid,
	gboolean playing_specific_track
);

void koto_playback_engine_set_volume(
	KotoPlaybackEngine * self,
	gdouble volume
);

void koto_playback_engine_stop(KotoPlaybackEngine * self);

void koto_playback_engine_toggle_track_repeat(KotoPlaybackEngine * self);

void koto_playback_engine_toggle_track_shuffle(KotoPlaybackEngine * self);

void koto_playback_engine_update_duration(KotoPlaybackEngine * self);

void koto_playback_engine_update_track_position(KotoPlaybackEngine * self);

gboolean koto_playback_engine_tick_duration(gpointer user_data);

gboolean koto_playback_engine_tick_track(gpointer user_data);

gboolean koto_playback_engine_tick_update_playlist_state(gpointer user_data);
