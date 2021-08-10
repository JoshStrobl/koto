/* audiobook-view.c
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
#include "../../components/album-info.h"
#include "../../components/button.h"
#include "../../components/track-item.h"
#include "../../db/cartographer.h"
#include "../../indexer/album-playlist-funcs.h"
#include "../../indexer/structs.h"
#include "../../playlist/current.h"
#include "../../koto-utils.h"
#include "../../koto-window.h"
#include "audiobook-view.h"

extern KotoCartographer * koto_maps;
extern KotoCurrentPlaylist * current_playlist;
extern KotoWindow * main_window;

struct _KotoAudiobookView {
	GtkBox parent_instance;

	KotoAlbum * album; // Album associated with this view

	GtkWidget * side_info; // Our side info (artwork, playback button, position)
	GtkWidget * info_contents; // Our info and contents vertical box

	KotoAlbumInfo * album_info; // Our "Album" (Audiobook) info

	GtkWidget * audiobook_art; // Our GtkImage for the audiobook art
	GtkWidget * playback_button; // Our GtkButton for playback
	GtkWidget * chapter_info; // Our GtkLabel of the position, effectively
	GtkWidget * playback_position; // Our GtkLabel for the track playback position

	GtkWidget * tracks_list; // GtkListBox of tracks
};

struct _KotoAudiobookViewClass {
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(KotoAudiobookView, koto_audiobook_view, GTK_TYPE_BOX);

static void koto_audiobook_view_class_init(KotoAudiobookViewClass * c) {
	(void) c;
}

static void koto_audiobook_view_init(KotoAudiobookView * self) {
	gtk_widget_add_css_class(GTK_WIDGET(self), "audiobook-view");

	self->side_info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_css_class(self->side_info, "side-info");
	gtk_widget_set_size_request(self->side_info, 220, -1); // Match audiobook art size

	self->info_contents = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	self->audiobook_art = gtk_image_new_from_icon_name("audio-x-generic-symbolic"); // Create an image with a symbolic icon
	gtk_widget_set_size_request(self->audiobook_art, 220, 220); // Set to 220 like our cover art button

	self->playback_button = gtk_button_new_with_label("Play"); // Default to our button saying Play instead of continue
	gtk_widget_add_css_class(self->playback_button, "suggested-action");
	g_signal_connect(self->playback_button, "clicked", G_CALLBACK(koto_audiobook_view_handle_play_clicked), self);

	self->chapter_info = gtk_label_new(NULL);
	self->playback_position = gtk_label_new(NULL);

	gtk_widget_set_halign(self->chapter_info, GTK_ALIGN_START);
	gtk_widget_set_halign(self->playback_position, GTK_ALIGN_START);

	gtk_box_append(GTK_BOX(self->side_info), self->audiobook_art); // Add our audiobook art to the side info
	gtk_box_append(GTK_BOX(self->side_info), GTK_WIDGET(self->playback_button)); // Add our playback button to the side info
	gtk_box_append(GTK_BOX(self->side_info), self->chapter_info);
	gtk_box_append(GTK_BOX(self->side_info), self->playback_position);

	self->album_info = koto_album_info_new("audiobook"); // Create an "audiobook" type KotoAlbumInfo
	gtk_box_append(GTK_BOX(self->info_contents), GTK_WIDGET(self->album_info)); // Add the album info the info contents

	GtkWidget * chapters_label = gtk_label_new("Chapters");
	gtk_widget_add_css_class(chapters_label, "chapters-label");
	gtk_widget_set_halign(chapters_label, GTK_ALIGN_START); // Align to the start

	gtk_box_append(GTK_BOX(self->info_contents), chapters_label);

	self->tracks_list = gtk_list_box_new(); // Create our list of our tracks
	gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(self->tracks_list), FALSE);
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->tracks_list), GTK_SELECTION_MULTIPLE);
	gtk_widget_add_css_class(self->tracks_list, "track-list");
	gtk_box_append(GTK_BOX(self->info_contents), self->tracks_list); // Add our listbox to the info contents

	gtk_box_append(GTK_BOX(self), self->side_info); // Add our side info to the box
	gtk_box_append(GTK_BOX(self), self->info_contents); // Add our info contents to the box
}

GtkWidget * koto_audiobook_view_create_track_item(
	gpointer item,
	gpointer user_data
) {
	(void) user_data;

	if (!KOTO_IS_TRACK(item)) { // Not actually a track
		return NULL;
	}

	KotoTrack * track = KOTO_TRACK(item); // Cast our item as a track
	KotoTrackItem * track_item = koto_track_item_new(track); // Create our track item

	if (!KOTO_IS_TRACK_ITEM(track_item)) { // Not a track item
		return NULL;
	}

	return GTK_WIDGET(track_item); // Cast as a widget and return the track item
}

void koto_audiobook_view_handle_play_clicked(
	GtkButton * button,
	gpointer user_data
) {
	(void) button;
	KotoAudiobookView * self = user_data;

	if (!KOTO_IS_AUDIOBOOK_VIEW(self)) {
		return;
	}

	if (!KOTO_IS_ALBUM(self->album)) { // Don't have an album associated with this view
		return;
	}

	koto_playlist_commit(koto_album_get_playlist(self->album)); // Ensure we commit the current playlist to the database. This is needed to ensure we are able to save the playlist state going forward
	koto_current_playlist_set_playlist(current_playlist, koto_album_get_playlist(self->album), TRUE, TRUE); // Set this playlist to be played immediately, play current track
}

void koto_audiobook_view_handle_playlist_updated(
	KotoPlaylist * playlist,
	gpointer user_data
) {
	(void) playlist;

	KotoAudiobookView * self = user_data;

	if (!KOTO_IS_AUDIOBOOK_VIEW(self)) {
		return;
	}

	koto_audiobook_view_update_side_info(self); // Update the side info based on the playlist modification
}

void koto_audiobook_view_set_album(
	KotoAudiobookView * self,
	KotoAlbum * album
) {
	if (!KOTO_IS_AUDIOBOOK_VIEW(self)) {
		return;
	}

	if (!KOTO_IS_ALBUM(album)) { // Not an album
		return;
	}

	self->album = album;

	gchar * album_art_path = koto_album_get_art(album); // Get any artwork

	if (koto_utils_string_is_valid(album_art_path)) { // Have album art
		gtk_image_set_from_file(GTK_IMAGE(self->audiobook_art), album_art_path); // Set our album art
	}

	koto_album_info_set_album_uuid(self->album_info, koto_album_get_uuid(album)); // Apply our album info

	gtk_list_box_bind_model(
		// Apply our binding for the GtkListBox
		GTK_LIST_BOX(self->tracks_list),
		G_LIST_MODEL(koto_album_get_store(album)),
		koto_audiobook_view_create_track_item,
		NULL,
		koto_audiobook_view_destroy_associated_user_data
	);


	KotoPlaylist * album_playlist = koto_album_get_playlist(self->album); // Get the album playlist
	if (!KOTO_IS_PLAYLIST(album_playlist)) { // Not a playlist
		return;
	}

	g_signal_connect(album_playlist, "modified", G_CALLBACK(koto_audiobook_view_handle_playlist_updated), self); // Handle modifications of a playlist
	g_signal_connect(album_playlist, "track-load-finalized", G_CALLBACK(koto_audiobook_view_handle_playlist_updated), self); // Handle when a playlist is finalized
	koto_audiobook_view_update_side_info(self); // Update our side info
}

void koto_audiobook_view_update_side_info(KotoAudiobookView * self) {
	if (!KOTO_IS_AUDIOBOOK_VIEW(self)) {
		return;
	}

	if (!KOTO_IS_ALBUM(self->album)) { // Not an album
		return;
	}

	KotoPlaylist * album_playlist = koto_album_get_playlist(self->album); // Get the album playlist
	if (!KOTO_IS_PLAYLIST(album_playlist)) { // Not a playlist
		return;
	}

	gint playlist_position = koto_playlist_get_current_position(album_playlist); // Get the current position in the playlist
	gint playlist_length = koto_playlist_get_length(album_playlist); // Get the length of the playlist

	KotoTrack * current_track = koto_playlist_get_current_track(album_playlist); // Get the track for the playlist

	if (!KOTO_IS_TRACK(current_track)) { // Not a track
		return;
	}

	guint playback_position = koto_track_get_playback_position(current_track);
	guint duration = koto_track_get_duration(current_track);
	gboolean continuing = (playlist_position >= 1) || ((playlist_position <= 0) && (playback_position != 0));

	gtk_button_set_label(GTK_BUTTON(self->playback_button), continuing ? "Resume" : "Play");

	if (continuing) { // Have been playing track
		gtk_label_set_text(GTK_LABEL(self->chapter_info), g_strdup_printf("Track %i of %i", (playlist_position <= 0) ? 1 : (playlist_position + 1), playlist_length));
		gtk_label_set_text(GTK_LABEL(self->playback_position), g_strdup_printf("%s / %s", koto_utils_seconds_to_time_format(playback_position), koto_utils_seconds_to_time_format(duration)));
		gtk_widget_show(self->chapter_info);
		gtk_widget_show(self->playback_position);
	} else {
		gtk_widget_hide(self->chapter_info); // Hide by default
		gtk_widget_hide(self->playback_position); // Hide by default
	}
}

void koto_audiobook_view_destroy_associated_user_data(gpointer user_data) {
	(void) user_data;
}

KotoAudiobookView * koto_audiobook_view_new() {
	return g_object_new(
		KOTO_TYPE_AUDIOBOOK_VIEW,
		"orientation",
		GTK_ORIENTATION_HORIZONTAL,
		NULL
	);
}