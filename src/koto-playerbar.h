/* koto-playerbar.h
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
#include <gstreamer-1.0/gst/gst.h>
#include <gtk-4.0/gtk/gtk.h>
#include "playback/engine.h"

G_BEGIN_DECLS

#define KOTO_TYPE_PLAYERBAR (koto_playerbar_get_type())
G_DECLARE_FINAL_TYPE (KotoPlayerBar, koto_playerbar, KOTO, PLAYERBAR, GObject)
#define KOTO_IS_PLAYERBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_PLAYERBAR))

KotoPlayerBar* koto_playerbar_new (void);
GtkWidget* koto_playerbar_get_main(KotoPlayerBar* bar);
void koto_playerbar_create_playback_details(KotoPlayerBar* bar);
void koto_playerbar_create_primary_controls(KotoPlayerBar* bar);
void koto_playerbar_create_secondary_controls(KotoPlayerBar* bar);
void koto_playerbar_go_backwards(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void koto_playerbar_go_forwards(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void koto_playerbar_handle_duration_change(KotoPlaybackEngine *engine, gpointer user_data);
void koto_playerbar_handle_engine_state_change(KotoPlaybackEngine *engine, gpointer user_data);
void koto_playerbar_handle_progress_change(KotoPlaybackEngine *engine, gpointer user_data);
void koto_playerbar_reset_progressbar(KotoPlayerBar* bar);
void koto_playerbar_set_progressbar_duration(KotoPlayerBar* bar, gint64 duration);
void koto_playerbar_set_progressbar_value(KotoPlayerBar* bar, gint64 progress);
void koto_playerbar_toggle_play_pause(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);

G_END_DECLS
