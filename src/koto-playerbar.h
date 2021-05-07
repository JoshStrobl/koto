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
void koto_playerbar_handle_is_playing(KotoPlaybackEngine *engine, gpointer user_data);
void koto_playerbar_handle_is_paused(KotoPlaybackEngine *engine, gpointer user_data);
void koto_playerbar_handle_playlist_button_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void koto_playerbar_handle_progressbar_scroll_begin(GtkEventControllerScroll *controller, gpointer data);
void koto_playerbar_handle_progressbar_gesture_begin(GtkGesture *gesture, GdkEventSequence *seq, gpointer data);
void koto_playerbar_handle_progressbar_gesture_end(GtkGesture *gesture, GdkEventSequence *seq, gpointer data);
void koto_playerbar_handle_progressbar_pressed(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void koto_playerbar_handle_progressbar_value_changed(GtkRange *progress_bar, gpointer data);
void koto_playerbar_handle_tick_duration(KotoPlaybackEngine *engine, gpointer user_data);
void koto_playerbar_handle_tick_track(KotoPlaybackEngine *engine, gpointer user_data);
void koto_playerbar_handle_track_repeat(KotoPlaybackEngine *engine, gpointer user_data);
void koto_playerbar_handle_track_shuffle(KotoPlaybackEngine *engine, gpointer user_data);
void koto_playerbar_handle_volume_button_change(GtkScaleButton *button, double value, gpointer user_data);
void koto_playerbar_reset_progressbar(KotoPlayerBar* bar);
void koto_playerbar_set_progressbar_duration(KotoPlayerBar* bar, gint64 duration);
void koto_playerbar_set_progressbar_value(KotoPlayerBar* bar, gdouble progress);
void koto_playerbar_toggle_play_pause(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void koto_playerbar_toggle_playlist_shuffle(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void koto_playerbar_toggle_track_repeat(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void koto_playerbar_update_track_info(KotoPlaybackEngine *engine, gpointer user_data);

G_END_DECLS
