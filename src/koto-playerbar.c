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
#include "db/cartographer.h"
#include "playlist/current.h"
#include "playlist/playlist.h"
#include "playback/engine.h"
#include "koto-button.h"
#include "koto-config.h"
#include "koto-playerbar.h"

extern KotoCurrentPlaylist *current_playlist;
extern KotoCartographer* koto_maps;
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

	gint64 last_recorded_duration;

	gboolean is_progressbar_seeking;
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
	gtk_range_set_round_digits(GTK_RANGE(self->progress_bar), 1);

	GtkEventController *scroll_controller = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_HORIZONTAL);
	g_signal_connect(scroll_controller, "scroll-begin", G_CALLBACK(koto_playerbar_handle_progressbar_scroll_begin), self);
	gtk_widget_add_controller(GTK_WIDGET(self->progress_bar), scroll_controller);

	GtkGesture *press_controller = gtk_gesture_click_new(); // Create a new GtkGestureLongPress
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(press_controller), 1); // Set to left click

	g_signal_connect(GTK_GESTURE(press_controller), "begin", G_CALLBACK(koto_playerbar_handle_progressbar_gesture_begin), self);
	g_signal_connect(GTK_GESTURE(press_controller), "end", G_CALLBACK(koto_playerbar_handle_progressbar_gesture_end), self);
	g_signal_connect(press_controller, "pressed", G_CALLBACK(koto_playerbar_handle_progressbar_pressed), self);
	//g_signal_connect(press_controller, "unpaired-release", G_CALLBACK(koto_playerbar_handle_progressbar_unpaired_release), self);

	gtk_widget_add_controller(GTK_WIDGET(self->progress_bar), GTK_EVENT_CONTROLLER(press_controller));

	g_signal_connect(GTK_RANGE(self->progress_bar), "value-changed", G_CALLBACK(koto_playerbar_handle_progressbar_value_changed), self);

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

	g_signal_connect(playback_engine, "is-playing", G_CALLBACK(koto_playerbar_handle_is_playing), self);
	g_signal_connect(playback_engine, "is-paused", G_CALLBACK(koto_playerbar_handle_is_paused), self);
	g_signal_connect(playback_engine, "tick-duration", G_CALLBACK(koto_playerbar_handle_tick_duration), self);
	g_signal_connect(playback_engine, "tick-track", G_CALLBACK(koto_playerbar_handle_tick_track), self);
	g_signal_connect(playback_engine, "track-changed", G_CALLBACK(koto_playerbar_update_track_info), self);
}

static void koto_playerbar_init(KotoPlayerBar *self) {
	self->last_recorded_duration = 0;
	self->is_progressbar_seeking = FALSE;
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

	if (KOTO_IS_BUTTON(bar->back_button)) {
		gtk_box_append(GTK_BOX(bar->primary_controls_section), GTK_WIDGET(bar->back_button));

		GtkGesture *back_controller = gtk_gesture_click_new(); // Create a new GtkGestureClick
		g_signal_connect(back_controller, "pressed", G_CALLBACK(koto_playerbar_go_backwards), NULL);
		gtk_widget_add_controller(GTK_WIDGET(bar->back_button), GTK_EVENT_CONTROLLER(back_controller));
	}

	if (KOTO_IS_BUTTON(bar->play_pause_button)) {
		gtk_box_append(GTK_BOX(bar->primary_controls_section), GTK_WIDGET(bar->play_pause_button));

		GtkGesture *controller = gtk_gesture_click_new(); // Create a new GtkGestureClick
		g_signal_connect(controller, "pressed", G_CALLBACK(koto_playerbar_toggle_play_pause), NULL);
		gtk_widget_add_controller(GTK_WIDGET(bar->play_pause_button), GTK_EVENT_CONTROLLER(controller));
	}

	if (KOTO_IS_BUTTON(bar->forward_button)) {
		gtk_box_append(GTK_BOX(bar->primary_controls_section), GTK_WIDGET(bar->forward_button));

		GtkGesture *forwards_controller = gtk_gesture_click_new(); // Create a new GtkGestureClick
		g_signal_connect(forwards_controller, "pressed", G_CALLBACK(koto_playerbar_go_forwards), NULL);
		gtk_widget_add_controller(GTK_WIDGET(bar->forward_button), GTK_EVENT_CONTROLLER(forwards_controller));
	}
}

void koto_playerbar_create_secondary_controls(KotoPlayerBar* bar) {
	bar->repeat_button = koto_button_new_with_icon("", "media-playlist-repeat-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->shuffle_button = koto_button_new_with_icon("", "media-playlist-shuffle-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->playlist_button = koto_button_new_with_icon("", "playlist-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->eq_button = koto_button_new_with_icon("", "multimedia-equalizer-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);

	bar->volume_button = gtk_volume_button_new(); // Have this take in a state and switch to a different icon if necessary

	if (KOTO_IS_BUTTON(bar->repeat_button)) {
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->repeat_button));

		GtkGesture *controller = gtk_gesture_click_new(); // Create a new GtkGestureClick
		g_signal_connect(controller, "pressed", G_CALLBACK(koto_playerbar_toggle_track_repeat), bar);
		gtk_widget_add_controller(GTK_WIDGET(bar->repeat_button), GTK_EVENT_CONTROLLER(controller));
	}

	if (KOTO_IS_BUTTON(bar->shuffle_button)) {
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->shuffle_button));

		GtkGesture *controller = gtk_gesture_click_new(); // Create a new GtkGestureClick
		g_signal_connect(controller, "pressed", G_CALLBACK(koto_playerbar_toggle_playlist_shuffle), bar);
		gtk_widget_add_controller(GTK_WIDGET(bar->shuffle_button), GTK_EVENT_CONTROLLER(controller));
	}

	if (KOTO_IS_BUTTON(bar->playlist_button)) {
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->playlist_button));
	}

	if (KOTO_IS_BUTTON(bar->eq_button)) {
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->eq_button));
	}

	if (GTK_IS_VOLUME_BUTTON(bar->volume_button)) {
		GtkAdjustment *granular_volume_change = gtk_adjustment_new(0.5, 0, 1.0, 0.02, 0.02, 0.02);
		g_object_set(bar->volume_button, "use-symbolic", TRUE, NULL);
		gtk_range_set_round_digits(GTK_RANGE(bar->volume_button), FALSE);
		gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON(bar->volume_button), granular_volume_change); // Set our adjustment
		gtk_box_append(GTK_BOX(bar->secondary_controls_section), bar->volume_button);

		g_signal_connect(GTK_SCALE_BUTTON(bar->volume_button), "value-changed", G_CALLBACK(koto_playerbar_handle_volume_button_change), bar);

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

void koto_playerbar_handle_is_playing(KotoPlaybackEngine *engine, gpointer user_data) {
	if (!KOTO_IS_PLAYBACK_ENGINE(engine)) {
		return;
	}

	KotoPlayerBar *bar = user_data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	koto_button_show_image(bar->play_pause_button, TRUE); // Set to TRUE to show pause as the next action
}

void koto_playerbar_handle_is_paused(KotoPlaybackEngine *engine, gpointer user_data) {
	if (!KOTO_IS_PLAYBACK_ENGINE(engine)) {
		return;
	}

	KotoPlayerBar *bar = user_data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	koto_button_show_image(bar->play_pause_button, FALSE); // Set to FALSE to show play as the next action
}

void koto_playerbar_handle_progressbar_scroll_begin(GtkEventControllerScroll *controller, gpointer data){
	(void) controller;
	g_message("scroll-begin");
}

void koto_playerbar_handle_progressbar_gesture_begin(GtkGesture *gesture, GdkEventSequence *seq, gpointer data) {
	(void) gesture; (void) seq;
	KotoPlayerBar *bar = data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	g_message("Begin");
	bar->is_progressbar_seeking = TRUE;
}

void koto_playerbar_handle_progressbar_gesture_end(GtkGesture *gesture, GdkEventSequence *seq, gpointer data) {
	(void) gesture; (void) seq;
	KotoPlayerBar *bar = data;

	g_message("Ended");

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}
	bar->is_progressbar_seeking = FALSE;
}

void koto_playerbar_handle_progressbar_pressed(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
	(void) gesture; (void) n_press; (void) x; (void) y;
	KotoPlayerBar *bar = data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	g_message("Pressed");
	bar->is_progressbar_seeking = TRUE;
}

void koto_playerbar_handle_progressbar_value_changed(GtkRange *progress_bar, gpointer data) {
	KotoPlayerBar *bar = data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	if (!bar->is_progressbar_seeking) {
		return;
	}

	int desired_position = (int) gtk_range_get_value(progress_bar);

	g_message("value changed");
	koto_playback_engine_set_position(playback_engine, desired_position); // Update our position
}

void koto_playerbar_handle_tick_duration(KotoPlaybackEngine *engine, gpointer user_data) {
	if (!KOTO_IS_PLAYBACK_ENGINE(engine)) {
		return;
	}

	KotoPlayerBar *bar = user_data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	koto_playerbar_set_progressbar_duration(bar, koto_playback_engine_get_duration(engine));
}


void koto_playerbar_handle_tick_track(KotoPlaybackEngine *engine, gpointer user_data) {
	if (!KOTO_IS_PLAYBACK_ENGINE(engine)) {
		return;
	}

	KotoPlayerBar *bar = user_data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	if (!bar->is_progressbar_seeking) {
		koto_playerbar_set_progressbar_value(bar, koto_playback_engine_get_progress(engine));
	}
}

void koto_playerbar_handle_volume_button_change(GtkScaleButton *button, double value, gpointer user_data) {
	(void) button; (void) user_data;
	koto_playback_engine_set_volume(playback_engine, value);
}

void koto_playerbar_reset_progressbar(KotoPlayerBar* bar) {
	gtk_range_set_range(GTK_RANGE(bar->progress_bar), 0, 0); // Reset range
	gtk_range_set_value(GTK_RANGE(bar->progress_bar), 0); // Set value to 0
}

void koto_playerbar_set_progressbar_duration(KotoPlayerBar* bar, gint64 duration) {
	if (duration <= 0) {
		return;
	}

	if (duration != bar->last_recorded_duration) { // Duration is different than what we recorded
		bar->last_recorded_duration = duration;
		gtk_range_set_range(GTK_RANGE(bar->progress_bar), 0, bar->last_recorded_duration);
	}
}

void koto_playerbar_set_progressbar_value(KotoPlayerBar* bar, gint64 progress) {
	gtk_range_set_value(GTK_RANGE(bar->progress_bar), progress);
}

void koto_playerbar_toggle_play_pause(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
	(void) gesture; (void) n_press; (void) x; (void) y; (void) data;

	koto_playback_engine_toggle(playback_engine);
}

void koto_playerbar_toggle_playlist_shuffle(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
	(void) gesture; (void) n_press; (void) x; (void) y;
	KotoPlayerBar *bar = data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	KotoPlaylist *playlist = koto_current_playlist_get_playlist(current_playlist);

	if (!KOTO_IS_PLAYLIST(playlist)) { // Don't have a playlist currently
		gtk_widget_remove_css_class(GTK_WIDGET(bar->shuffle_button), "active"); // Remove active state
		return;
	}

	gboolean currently_shuffling = FALSE;
	g_object_get(playlist, "is-shuffle-enabled", &currently_shuffling, NULL); // Get the current is-shuffle-enabled

	(currently_shuffling) ? gtk_widget_remove_css_class(GTK_WIDGET(bar->shuffle_button), "active") : gtk_widget_add_css_class(GTK_WIDGET(bar->shuffle_button), "active");
	g_object_set(playlist, "is-shuffle-enabled", !currently_shuffling, NULL); // Provide inverse value
}

void koto_playerbar_toggle_track_repeat(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
	(void) gesture; (void) n_press; (void) x; (void) y;
	KotoPlayerBar *bar = data;

	if (koto_playback_engine_get_track_repeat(playback_engine)) { // Toggled on at the moment
		gtk_widget_remove_css_class(GTK_WIDGET(bar->repeat_button), "active"); // Remove active CSS class
	} else {
		gtk_widget_add_css_class(GTK_WIDGET(bar->repeat_button), "active"); // Add active CSS class
	}

	koto_playback_engine_toggle_track_repeat(playback_engine); // Toggle the state
}

void koto_playerbar_update_track_info(KotoPlaybackEngine *engine, gpointer user_data) {
	if (!KOTO_IS_PLAYBACK_ENGINE(engine)) {
		return;
	}

	KotoPlayerBar *bar = user_data;

	if (!KOTO_IS_PLAYERBAR(bar)) {
		return;
	}

	KotoIndexedTrack *current_track = koto_playback_engine_get_current_track(playback_engine); // Get the current track from the playback engine

	if (!KOTO_IS_INDEXED_TRACK(current_track)) {
		return;
	}

	gchar *track_name = NULL;
	gchar *artist_uuid = NULL;
	gchar *album_uuid = NULL;

	g_object_get(current_track, "parsed-name", &track_name, "artist-uuid", &artist_uuid, "album-uuid", &album_uuid, NULL);

	KotoIndexedArtist *artist = koto_cartographer_get_artist_by_uuid(koto_maps, artist_uuid);
	KotoIndexedAlbum *album = koto_cartographer_get_album_by_uuid(koto_maps, album_uuid);

	g_free(artist_uuid);
	g_free(album_uuid);

	if ((track_name != NULL) && (strcmp(track_name, "") != 0)) { // Have a track name
		gtk_label_set_text(GTK_LABEL(bar->playback_title), track_name); // Set the label
	}

	if (KOTO_IS_INDEXED_ARTIST(artist)) {
		gchar *artist_name = NULL;
		g_object_get(artist, "name", &artist_name, NULL);

		if ((artist_name != NULL) && (strcmp(artist_name, "") != 0)) { // Have an artist name
			gtk_label_set_text(GTK_LABEL(bar->playback_artist), artist_name);
			gtk_widget_show(bar->playback_artist);
		} else { // Don't have an artist name somehow
			gtk_widget_hide(bar->playback_artist);
		}
	}

	if (KOTO_IS_INDEXED_ALBUM(album)) {
		gchar *album_name = NULL;
		gchar *art_path = NULL;
		g_object_get(album, "name", &album_name, "art-path", &art_path, NULL); // Get album name and art path

		if ((album_name != NULL) && (strcmp(album_name, "") != 0)) { // Have an album name
			gtk_label_set_text(GTK_LABEL(bar->playback_album), album_name);
			gtk_widget_show(bar->playback_album);
		} else {
			gtk_widget_hide(bar->playback_album);
		}

		if ((art_path != NULL) && g_path_is_absolute(art_path)) { // Have an album artist path
			gtk_image_set_from_file(GTK_IMAGE(bar->artwork), art_path); // Update the art
		} else {
			gtk_image_set_from_icon_name(GTK_IMAGE(bar->artwork), "audio-x-generic-symbolic"); // Use generic instead
		}
	}
}

GtkWidget* koto_playerbar_get_main(KotoPlayerBar* bar) {
	return bar->main;
}
