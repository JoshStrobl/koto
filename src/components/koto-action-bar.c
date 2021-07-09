/* koto-action-bar.c
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
#include <gtk-4.0/gtk/gtk.h>
#include "koto-action-bar.h"
#include "../config/config.h"
#include "../db/cartographer.h"
#include "../indexer/album-playlist-funcs.h"
#include "../pages/music/music-local.h"
#include "../playlist/add-remove-track-popover.h"
#include "../playlist/current.h"
#include "../playback/engine.h"
#include "../koto-button.h"
#include "../koto-utils.h"
#include "../koto-window.h"

extern KotoAddRemoveTrackPopover * koto_add_remove_track_popup;
extern KotoCartographer * koto_maps;
extern KotoConfig * config;
extern KotoCurrentPlaylist * current_playlist;
extern KotoPageMusicLocal * music_local_page;
extern KotoPlaybackEngine * playback_engine;
extern KotoWindow * main_window;

enum {
	SIGNAL_CLOSED,
	N_SIGNALS
};

static guint actionbar_signals[N_SIGNALS] = {
	0
};

struct _KotoActionBar {
	GObject parent_instance;

	GtkActionBar * main;

	GtkWidget * center_box_content;
	GtkWidget * start_box_content;
	GtkWidget * stop_box_content;

	KotoButton * close_button;
	GtkWidget * go_to_artist;
	GtkWidget * playlists;
	GtkWidget * play_track;
	GtkWidget * remove_from_playlist;

	GList * current_list;
	gchar * current_album_uuid;
	gchar * current_playlist_uuid;
	KotoActionBarRelative relative;
};

struct _KotoActionBarClass {
	GObjectClass parent_class;

	void (* closed) (KotoActionBar * self);
};

G_DEFINE_TYPE(KotoActionBar, koto_action_bar, G_TYPE_OBJECT);

KotoActionBar * action_bar;

static void koto_action_bar_class_init(KotoActionBarClass * c) {
	GObjectClass * gobject_class = G_OBJECT_CLASS(c);

	actionbar_signals[SIGNAL_CLOSED] = g_signal_new(
		"closed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoActionBarClass, closed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);
}

static void koto_action_bar_init(KotoActionBar * self) {
	self->main = GTK_ACTION_BAR(gtk_action_bar_new()); // Create a new action bar
	self->current_list = NULL;

	self->close_button = koto_button_new_with_icon(NULL, "window-close-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	self->go_to_artist = gtk_button_new_with_label("Go to Artist");
	self->playlists = gtk_button_new_with_label("Playlists");
	self->play_track = gtk_button_new_with_label("Play");
	self->remove_from_playlist = gtk_button_new_with_label("Remove from Playlist");

	gtk_widget_add_css_class(self->playlists, "suggested-action");
	gtk_widget_add_css_class(self->play_track, "suggested-action");
	gtk_widget_add_css_class(self->remove_from_playlist, "destructive-action");

	gtk_widget_set_size_request(self->play_track, 160, -1);

	self->center_box_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	self->start_box_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	self->stop_box_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);

	gtk_box_prepend(GTK_BOX(self->start_box_content), GTK_WIDGET(self->go_to_artist));

	gtk_box_prepend(GTK_BOX(self->center_box_content), GTK_WIDGET(self->play_track));

	gtk_box_prepend(GTK_BOX(self->stop_box_content), GTK_WIDGET(self->playlists));
	gtk_box_append(GTK_BOX(self->stop_box_content), GTK_WIDGET(self->remove_from_playlist));
	gtk_box_append(GTK_BOX(self->stop_box_content), GTK_WIDGET(self->close_button));

	gtk_action_bar_pack_start(self->main, self->start_box_content);
	gtk_action_bar_pack_end(self->main, self->stop_box_content);
	gtk_action_bar_set_center_widget(self->main, self->center_box_content);

	// Hide all the widgets by default
	gtk_widget_hide(GTK_WIDGET(self->go_to_artist));
	gtk_widget_hide(GTK_WIDGET(self->playlists));
	gtk_widget_hide(GTK_WIDGET(self->play_track));
	gtk_widget_hide(GTK_WIDGET(self->remove_from_playlist));

	// Set up bindings

	koto_button_add_click_handler(self->close_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_action_bar_handle_close_button_clicked), self);
	g_signal_connect(self->go_to_artist, "clicked", G_CALLBACK(koto_action_bar_handle_go_to_artist_button_clicked), self);
	g_signal_connect(self->playlists, "clicked", G_CALLBACK(koto_action_bar_handle_playlists_button_clicked), self);
	g_signal_connect(self->play_track, "clicked", G_CALLBACK(koto_action_bar_handle_play_track_button_clicked), self);
	g_signal_connect(self->remove_from_playlist, "clicked", G_CALLBACK(koto_action_bar_handle_remove_from_playlist_button_clicked), self);

	koto_action_bar_toggle_reveal(self, FALSE); // Hide by default
}

void koto_action_bar_close(KotoActionBar * self) {
	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	gtk_widget_hide(GTK_WIDGET(koto_add_remove_track_popup)); // Hide the Add / Remove Track Playlists popover
	koto_action_bar_toggle_reveal(self, FALSE); // Hide the action bar

	g_signal_emit(
		self,
		actionbar_signals[SIGNAL_CLOSED],
		0,
		NULL
	);
}

GtkActionBar * koto_action_bar_get_main(KotoActionBar * self) {
	if (!KOTO_IS_ACTION_BAR(self)) {
		return NULL;
	}

	return self->main;
}

void koto_action_bar_handle_close_button_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer data
) {
	(void) gesture;
	(void) n_press;
	(void) x;
	(void) y;
	koto_action_bar_close(data);
}

void koto_action_bar_handle_go_to_artist_button_clicked(
	GtkButton * button,
	gpointer data
) {
	(void) button;
	KotoActionBar * self = data;

	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	if (self->current_list == NULL || (g_list_length(self->current_list) != 1)) { // Not a list or not exactly one item
		return;
	}

	KotoTrack * selected_track = g_list_nth_data(self->current_list, 0); // Get the first item

	if (!KOTO_IS_TRACK(selected_track)) { // Not a track
		return;
	}

	gchar * artist_uuid = NULL;

	g_object_get(
		selected_track,
		"artist-uuid",
		&artist_uuid,
		NULL
	);

	koto_page_music_local_go_to_artist_by_uuid(music_local_page, artist_uuid); // Go to the artist
	koto_window_go_to_page(main_window, "music.local"); // Navigate to the local music stack so we can see the substack page
	koto_action_bar_close(self); // Close the action bar
}
void koto_action_bar_handle_playlists_button_clicked(
	GtkButton * button,
	gpointer data
) {
	(void) button;
	KotoActionBar * self = data;

	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	if (self->current_list == NULL || (g_list_length(self->current_list) == 0)) { // Not a list or no content
		return;
	}

	koto_add_remove_track_popover_set_tracks(koto_add_remove_track_popup, self->current_list); // Set the popover tracks
	koto_add_remove_track_popover_set_pointing_to_widget(koto_add_remove_track_popup, GTK_WIDGET(self->playlists), GTK_POS_TOP); // Show the popover above the button
	gtk_widget_show(GTK_WIDGET(koto_add_remove_track_popup));
}

void koto_action_bar_handle_play_track_button_clicked(
	GtkButton * button,
	gpointer data
) {
	(void) button;
	KotoActionBar * self = data;

	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	if (self->current_list == NULL || (g_list_length(self->current_list) != 1)) { // Not a list or not exactly 1 item selected
		goto doclose;
	}

	KotoTrack * track = g_list_nth_data(self->current_list, 0); // Get the first track

	if (!KOTO_IS_TRACK(track)) { // Not a track
		goto doclose;
	}

	KotoPlaylist * playlist = NULL;

	if (self->relative == KOTO_ACTION_BAR_IS_PLAYLIST_RELATIVE) { // Relative to a playlist
		playlist = koto_cartographer_get_playlist_by_uuid(koto_maps, self->current_playlist_uuid);
	} else if (self->relative == KOTO_ACTION_BAR_IS_ALBUM_RELATIVE) { // Relative to an Album
		KotoAlbum * album = koto_cartographer_get_album_by_uuid(koto_maps, self->current_album_uuid); // Get the Album

		if (KOTO_IS_ALBUM(album)) { // Have an Album
			playlist = koto_album_create_playlist(album); // Create our playlist dynamically for the Album
		}
	}

	if (KOTO_IS_PLAYLIST(playlist)) { // Is a playlist
		koto_current_playlist_set_playlist(current_playlist, playlist, FALSE); // Update our playlist to the one associated with the track we are playing
		koto_playlist_set_track_as_current(playlist, koto_track_get_uuid(track)); // Get this track as the current track in the position
	}

	gboolean continue_playback = FALSE;
	g_object_get(config, "playback-continue-on-playlist", &continue_playback, NULL);
	koto_playback_engine_set_track_by_uuid(playback_engine, koto_track_get_uuid(track), continue_playback); // Set the track to play

doclose:
	koto_action_bar_close(self);
}

void koto_action_bar_handle_remove_from_playlist_button_clicked(
	GtkButton * button,
	gpointer data
) {
	(void) button;
	KotoActionBar * self = data;

	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	if (self->current_list == NULL || (g_list_length(self->current_list) == 0)) { // Not a list or no content
		goto doclose;
	}

	if (!koto_utils_is_string_valid(self->current_playlist_uuid)) { // Not valid UUID
		goto doclose;
	}

	KotoPlaylist * playlist = koto_cartographer_get_playlist_by_uuid(koto_maps, self->current_playlist_uuid);

	if (!KOTO_IS_PLAYLIST(playlist)) { // Not a playlist
		goto doclose;
	}

	GList * cur_list;

	for (cur_list = self->current_list; cur_list != NULL; cur_list = cur_list->next) { // For each KotoTrack
		KotoTrack * track = cur_list->data;
		koto_playlist_remove_track_by_uuid(playlist, koto_track_get_uuid(track)); // Remove this track
	}

doclose:
	koto_action_bar_close(self);
}

void koto_action_bar_set_tracks_in_album_selection(
	KotoActionBar * self,
	gchar * album_uuid,
	GList * tracks
) {
	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	if (koto_utils_is_string_valid(self->current_album_uuid)) { // Album UUID currently set
		g_free(self->current_album_uuid);
	}

	if (koto_utils_is_string_valid(self->current_playlist_uuid)) { // Playlist UUID currently set
		g_free(self->current_playlist_uuid);
	}

	self->current_playlist_uuid = NULL;
	self->current_album_uuid = g_strdup(album_uuid);
	self->relative = KOTO_ACTION_BAR_IS_ALBUM_RELATIVE;

	g_list_free(self->current_list);
	self->current_list = g_list_copy(tracks);

	koto_add_remove_track_popover_clear_tracks(koto_add_remove_track_popup); // Clear the current popover contents
	koto_add_remove_track_popover_set_tracks(koto_add_remove_track_popup, self->current_list); // Set the associated tracks to remove

	koto_action_bar_toggle_go_to_artist_visibility(self, FALSE);
	koto_action_bar_toggle_play_button_visibility(self, g_list_length(self->current_list) == 1);

	gtk_widget_show(GTK_WIDGET(self->playlists));
	gtk_widget_hide(GTK_WIDGET(self->remove_from_playlist));
}

void koto_action_bar_set_tracks_in_playlist_selection(
	KotoActionBar * self,
	gchar * playlist_uuid,
	GList * tracks
) {
	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	if (koto_utils_is_string_valid(self->current_album_uuid)) { // Album UUID currently set
		g_free(self->current_album_uuid);
	}

	if (koto_utils_is_string_valid(self->current_playlist_uuid)) { // Playlist UUID currently set
		g_free(self->current_playlist_uuid);
	}

	self->current_album_uuid = NULL;
	self->current_playlist_uuid = g_strdup(playlist_uuid);
	self->relative = KOTO_ACTION_BAR_IS_PLAYLIST_RELATIVE;

	g_list_free(self->current_list);
	self->current_list = g_list_copy(tracks);

	gboolean single_selected = g_list_length(tracks) == 1;

	koto_action_bar_toggle_go_to_artist_visibility(self, single_selected);
	koto_action_bar_toggle_play_button_visibility(self, single_selected);
	gtk_widget_hide(GTK_WIDGET(self->playlists));
	gtk_widget_show(GTK_WIDGET(self->remove_from_playlist));
}

void koto_action_bar_toggle_go_to_artist_visibility(
	KotoActionBar * self,
	gboolean visible
) {
	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	(visible) ? gtk_widget_show(GTK_WIDGET(self->go_to_artist)) : gtk_widget_hide(GTK_WIDGET(self->go_to_artist));
}

void koto_action_bar_toggle_play_button_visibility(
	KotoActionBar * self,
	gboolean visible
) {
	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	(visible) ? gtk_widget_show(GTK_WIDGET(self->play_track)) : gtk_widget_hide(GTK_WIDGET(self->play_track));
}

void koto_action_bar_toggle_reveal(
	KotoActionBar * self,
	gboolean state
) {
	if (!KOTO_IS_ACTION_BAR(self)) {
		return;
	}

	gtk_action_bar_set_revealed(self->main, state);
}

KotoActionBar * koto_action_bar_new() {
	return g_object_new(
		KOTO_TYPE_ACTION_BAR,
		NULL
	);
}