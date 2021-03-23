/* koto-playerbar.c
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

#include <gstreamer-1.0/gst/gst.h>
#include <gtk-4.0/gtk/gtk.h>
#include "playback/engine.h"
#include "koto-button.h"
#include "koto-config.h"
#include "koto-playerbar.h"

extern KotoPlaybackEngine *playback_engine;

struct _KotoPlayerBar {
	GObject parent_instance;
	GtkWidget *main;
	GtkWidget *controls;

	/* Sections */
	GtkWidget *playback_section;
	GtkWidget *primary_controls_section;
	GtkWidget *secondary_controls_section;

	/* Primary Buttons */
	KotoButton *back_button;
	KotoButton *play_pause_button;
	KotoButton *forward_button;
	KotoButton *repeat_button;
	KotoButton *shuffle_button;
	KotoButton *playlist_button;
	KotoButton *eq_button;
	GtkWidget *volume_button;

	GtkWidget *progress_bar;

	/* Selected Playback Section */

	GtkWidget *playback_details_section;
	GtkWidget *artwork;
	GtkWidget *playback_title;
	GtkWidget *playback_album;
	GtkWidget *playback_artist;
};

struct _KotoPlayerBarClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE(KotoPlayerBar, koto_playerbar, G_TYPE_OBJECT);

static void koto_playerbar_constructed(GObject *obj);

static void koto_playerbar_class_init(KotoPlayerBarClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);

	gobject_class->constructed = koto_playerbar_constructed;
}

static void koto_playerbar_constructed(GObject *obj) {
	KotoPlayerBar *self = KOTO_PLAYERBAR(obj);
	self->main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	self->progress_bar = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 120, 1); // Default to 120 as random max
	gtk_scale_set_draw_value(GTK_SCALE(self->progress_bar), FALSE);
	gtk_scale_set_digits(GTK_SCALE(self->progress_bar), 0);
	gtk_range_set_increments(GTK_RANGE(self->progress_bar), 1, 1);

	self->controls = gtk_center_box_new();
	gtk_center_box_set_baseline_position(GTK_CENTER_BOX(self->controls), GTK_BASELINE_POSITION_CENTER);
	gtk_widget_add_css_class(self->main, "player-bar");

	self->playback_section = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	self->primary_controls_section = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	self->secondary_controls_section = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	gtk_center_box_set_start_widget(GTK_CENTER_BOX(self->controls), GTK_WIDGET(self->primary_controls_section));
	gtk_center_box_set_center_widget(GTK_CENTER_BOX(self->controls), GTK_WIDGET(self->playback_section));
	gtk_center_box_set_end_widget(GTK_CENTER_BOX(self->controls), GTK_WIDGET(self->secondary_controls_section));

	koto_playerbar_create_playback_details(self);
	koto_playerbar_create_primary_controls(self);
	koto_playerbar_create_secondary_controls(self);

	gtk_box_prepend(GTK_BOX(self->main), self->progress_bar);
	gtk_box_append(GTK_BOX(self->main), self->controls);

	gtk_widget_set_hexpand(GTK_WIDGET(self->main), TRUE);

	// Set up the bindings

	g_signal_connect(playback_engine, "duration-changed", G_CALLBACK(koto_playerbar_handle_duration_change), self);
	g_signal_connect(playback_engine, "play-state-changed", G_CALLBACK(koto_playerbar_handle_engine_state_change), self);
	g_signal_connect(playback_engine, "progress-changed", G_CALLBACK(koto_playerbar_handle_progress_change), self);
}

static void koto_playerbar_init(KotoPlayerBar *self) {
	(void) self;
}

KotoPlayerBar* koto_playerbar_new(void) {
	return g_object_new(KOTO_TYPE_PLAYERBAR, NULL);
}

void koto_playerbar_create_playback_details(KotoPlayerBar* bar) {
	bar->playback_details_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	GtkIconTheme *default_icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default()); // Get the icon theme for this display

	if (default_icon_theme != NULL) {
		gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(bar->main));
		GtkIconPaintable* audio_paintable = gtk_icon_theme_lookup_icon(default_icon_theme, "audio-x-generic-symbolic", NULL, 96, scale_factor, GTK_TEXT_DIR_NONE, GTK_ICON_LOOKUP_PRELOAD);

		if (GTK_IS_ICON_PAINTABLE(audio_paintable)) {
			if (GTK_IS_IMAGE(bar->artwork)) {
				gtk_image_set_from_paintable(GTK_IMAGE(bar->artwork), GDK_PAINTABLE(audio_paintable));
			} else { // Not an image
				bar->artwork = gtk_image_new_from_paintable(GDK_PAINTABLE(audio_paintable));
				gtk_widget_set_size_request(bar->artwork, 96, 96);
				gtk_box_append(GTK_BOX(bar->playback_section), bar->artwork);
			}
		}
	}

	bar->playback_title = gtk_label_new("Title");
	bar->playback_album = gtk_label_new("Album");
	bar->playback_artist = gtk_label_new("Artist");

	gtk_box_append(GTK_BOX(bar->playback_details_section), GTK_WIDGET(bar->playback_title));
	gtk_box_append(GTK_BOX(bar->playback_details_section), GTK_WIDGET(bar->playback_album));
	gtk_box_append(GTK_BOX(bar->playback_details_section), GTK_WIDGET(bar->playback_artist));

	gtk_box_append(GTK_BOX(bar->playback_section), GTK_WIDGET(bar->playback_details_section));
}

void koto_playerbar_create_primary_controls(KotoPlayerBar* bar) {
	bar->back_button = koto_button_new_with_icon("", "media-skip-backward-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->play_pause_button = koto_button_new_with_icon("", "media-playback-start-symbolic", "media-playback-pause-symbolic", KOTO_BUTTON_PIXBUF_SIZE_LARGE); // TODO: Have this take in a state and switch to a different icon if necessary
	bar->forward_button = koto_button_new_with_icon("", "media-skip-forward-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);

	if (bar->back_button != NULL) {
		gtk_box_append(GTK_BOX(bar->primary_controls_section), GTK_WIDGET(bar->back_button));
	}

	if (bar->play_pause_button != NULL) {
		gtk_box_append(GTK_BOX(bar->primary_controls_section), GTK_WIDGET(bar->play_pause_button));
	}

	if (bar->forward_button != NULL) {
		gtk_box_append(GTK_BOX(bar->primary_controls_section), GTK_WIDGET(bar->forward_button));
	}


	GtkGesture *back_controller = gtk_gesture_click_new(); // Create a new GtkGestureClick
	g_signal_connect(back_controller, "pressed", G_CALLBACK(koto_playerbar_go_backwards), NULL);
	gtk_widget_add_controller(GTK_WIDGET(bar->back_button), GTK_EVENT_CONTROLLER(back_controller));

	GtkGesture *controller = gtk_gesture_click_new(); // Create a new GtkGestureClick
	g_signal_connect(controller, "pressed", G_CALLBACK(koto_playerbar_toggle_play_pause), NULL);
	gtk_widget_add_controller(GTK_WIDGET(bar->play_pause_button), GTK_EVENT_CONTROLLER(controller));

	GtkGesture *forwards_controller = gtk_gesture_click_new(); // Create a new GtkGestureClick
	g_signal_connect(forwards_controller, "pressed", G_CALLBACK(koto_playerbar_go_forwards), NULL);
	gtk_widget_add_controller(GTK_WIDGET(bar->forward_button), GTK_EVENT_CONTROLLER(forwards_controller));
}

void koto_playerbar_create_secondary_controls(KotoPlayerBar* bar) {
	bar->repeat_button = koto_button_new_with_icon("", "media-playlist-repeat-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->shuffle_button = koto_button_new_with_icon("", "media-playlist-shuffle-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->playlist_button = koto_button_new_with_icon("", "playlist-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->eq_button = koto_button_new_with_icon("", "multimedia-equalizer-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->volume_button = gtk_volume_button_new(); // Have this take in a state and switch to a different icon if necessary
	gtk_scale_button_set_value(GTK_SCALE_BUTTON(bar->volume_button), 0.5);

	if (bar->repeat_button != NULL) {
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->repeat_button));
	}

	if (bar->shuffle_button != NULL) {
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->shuffle_button));
	}

	if (bar->playlist_button != NULL) {
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->playlist_button));
	}

	if (bar->eq_button != NULL) {
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->eq_button));
	}

	if (bar->volume_button != NULL) {
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), bar->volume_button);
	}
}

void koto_playerbar_go_backwards(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
	(void) gesture; (void) n_press; (void) x; (void) y; (void) data;

	koto_playback_engine_backwards(playback_engine);
}

void koto_playerbar_go_forwards(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
	(void) gesture; (void) n_press; (void) x; (void) y; (void) data;

	koto_playback_engine_forwards(playback_engine);
}

void koto_playerbar_handle_duration_change(KotoPlaybackEngine *engine, gpointer user_data) {
	if (!KOTO_IS_PLAYBACK_ENGINE(engine)) {
		return;
	}

	KotoPlayerBar *bar = user_data;
	koto_playerbar_set_progressbar_duration(bar, koto_playback_engine_get_duration(engine));
}

void koto_playerbar_handle_engine_state_change(KotoPlaybackEngine *engine, gpointer user_data) {
	if (!KOTO_IS_PLAYBACK_ENGINE(engine)) {
		return;
	}

	KotoPlayerBar *bar = user_data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	GstState new_state = koto_playback_engine_get_state(playback_engine);

	if (new_state == GST_STATE_PLAYING) { // Now playing
		koto_button_show_image(bar->play_pause_button, TRUE); // Set to TRUE to show pause as the next action
	} else if (new_state == GST_STATE_PAUSED) { // Now paused
		koto_button_show_image(bar->play_pause_button, FALSE); // Set to FALSE to show play as the next action
	}
}

void koto_playerbar_handle_progress_change(KotoPlaybackEngine *engine, gpointer user_data) {
	if (!KOTO_IS_PLAYBACK_ENGINE(engine)) {
		return;
	}

	KotoPlayerBar *bar = user_data;
	koto_playerbar_set_progressbar_value(bar, koto_playback_engine_get_progress(engine));
}

void koto_playerbar_reset_progressbar(KotoPlayerBar* bar) {
	gtk_range_set_range(GTK_RANGE(bar->progress_bar), 0, 0); // Reset range
	gtk_range_set_value(GTK_RANGE(bar->progress_bar), 0); // Set value to 0
}

void koto_playerbar_set_progressbar_duration(KotoPlayerBar* bar, gint64 duration) {
	if (duration < 0) {
		return;
	}

	gtk_range_set_range(GTK_RANGE(bar->progress_bar), 0, duration);
}

void koto_playerbar_set_progressbar_value(KotoPlayerBar* bar, gint64 progress) {
	if (progress < 0) {
		return;
	}

	gtk_range_set_value(GTK_RANGE(bar->progress_bar), progress);
}

void koto_playerbar_toggle_play_pause(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
	(void) gesture; (void) n_press; (void) x; (void) y; (void) data;

	koto_playback_engine_toggle(playback_engine);
}

GtkWidget* koto_playerbar_get_main(KotoPlayerBar* bar) {
	return bar->main;
}
