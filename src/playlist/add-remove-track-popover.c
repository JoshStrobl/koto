/* add-remove-track-popover.c
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
#include "../db/cartographer.h"
#include "../playback/engine.h"
#include "../playlist/playlist.h"
#include "add-remove-track-popover.h"

extern KotoCartographer * koto_maps;

struct _KotoAddRemoveTrackPopover {
	GtkPopover parent_instance;
	GtkWidget * list_box;
	GHashTable * checkbox_to_playlist_uuid;
	GHashTable * playlist_uuid_to_checkbox;
	GList * tracks;

	GHashTable * checkbox_to_signal_ids;
};

G_DEFINE_TYPE(KotoAddRemoveTrackPopover, koto_add_remove_track_popover, GTK_TYPE_POPOVER);

KotoAddRemoveTrackPopover * koto_add_remove_track_popup = NULL;

static void koto_add_remove_track_popover_class_init(KotoAddRemoveTrackPopoverClass * c) {
	(void) c;
}

static void koto_add_remove_track_popover_init(KotoAddRemoveTrackPopover * self) {
	self->list_box = gtk_list_box_new(); // Create our new GtkListBox
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->list_box), GTK_SELECTION_NONE);

	self->checkbox_to_playlist_uuid = g_hash_table_new(g_str_hash, g_str_equal);
	self->playlist_uuid_to_checkbox = g_hash_table_new(g_str_hash, g_str_equal);
	self->checkbox_to_signal_ids = g_hash_table_new(g_str_hash, g_str_equal),
	self->tracks = NULL; // Initialize our tracks

	gtk_popover_set_autohide(GTK_POPOVER(self), TRUE); // Ensure autohide is enabled
	gtk_popover_set_child(GTK_POPOVER(self), self->list_box);

	g_signal_connect(koto_maps, "playlist-added", G_CALLBACK(koto_add_remove_track_popover_handle_playlist_added), self);
	g_signal_connect(koto_maps, "playlist-removed", G_CALLBACK(koto_add_remove_track_popover_handle_playlist_removed), self);
}

void koto_add_remove_track_popover_add_playlist(
	KotoAddRemoveTrackPopover * self,
	KotoPlaylist * playlist
) {
	if (!KOTO_JS_ADD_REMOVE_TRACK_POPOVER(self)) {
		return;
	}

	if (!KOTO_IS_PLAYLIST(playlist)) { // Not a playlist
		return;
	}

	gchar * playlist_uuid = koto_playlist_get_uuid(playlist); // Get the UUID of the playlist


	if (GTK_IS_CHECK_BUTTON(g_hash_table_lookup(self->playlist_uuid_to_checkbox, playlist_uuid))) { // Already have a check button for this
		g_free(playlist_uuid);
		return;
	}

	GtkWidget * playlist_button = gtk_check_button_new_with_label(koto_playlist_get_name(playlist)); // Create our GtkCheckButton


	g_hash_table_insert(self->checkbox_to_playlist_uuid, playlist_button, playlist_uuid);
	g_hash_table_insert(self->playlist_uuid_to_checkbox, playlist_uuid, playlist_button);

	gulong playlist_sig_id = g_signal_connect(playlist_button, "toggled", G_CALLBACK(koto_add_remove_track_popover_handle_checkbutton_toggle), self);


	g_hash_table_insert(self->checkbox_to_signal_ids, playlist_button, GUINT_TO_POINTER(playlist_sig_id)); // Add our GSignal handler ID

	gtk_list_box_append(GTK_LIST_BOX(self->list_box), playlist_button); // Add the playlist to the list box
}

void koto_add_remove_track_popover_clear_tracks(KotoAddRemoveTrackPopover * self) {
	if (!KOTO_JS_ADD_REMOVE_TRACK_POPOVER(self)) {
		return;
	}

	if (self->tracks != NULL) { // Is a list
		g_list_free(self->tracks);
		self->tracks = NULL;
	}
}

void koto_add_remove_track_popover_remove_playlist(
	KotoAddRemoveTrackPopover * self,
	gchar * playlist_uuid
) {
	if (!KOTO_JS_ADD_REMOVE_TRACK_POPOVER(self)) {
		return;
	}

	if (!g_hash_table_contains(self->playlist_uuid_to_checkbox, playlist_uuid)) { // Playlist not added
		return;
	}

	GtkCheckButton * btn = GTK_CHECK_BUTTON(g_hash_table_lookup(self->playlist_uuid_to_checkbox, playlist_uuid)); // Get the check button


	if (GTK_IS_CHECK_BUTTON(btn)) { // Is a check button
		g_hash_table_remove(self->checkbox_to_playlist_uuid, btn); // Remove uuid based on btn
		gtk_list_box_remove(GTK_LIST_BOX(self->list_box), GTK_WIDGET(btn)); // Remove the button from the list box
	}

	g_hash_table_remove(self->playlist_uuid_to_checkbox, playlist_uuid);
}

void koto_add_remove_track_popover_handle_checkbutton_toggle(
	GtkCheckButton * btn,
	gpointer user_data
) {
	KotoAddRemoveTrackPopover * self = user_data;


	if (!KOTO_JS_ADD_REMOVE_TRACK_POPOVER(self)) {
		return;
	}

	gboolean should_add = gtk_check_button_get_active(btn); // Get the now active state
	gchar * playlist_uuid = g_hash_table_lookup(self->checkbox_to_playlist_uuid, btn); // Get the playlist UUID for this button

	KotoPlaylist * playlist = koto_cartographer_get_playlist_by_uuid(koto_maps, playlist_uuid); // Get the playlist


	if (!KOTO_IS_PLAYLIST(playlist)) { // Failed to get the playlist
		return;
	}

	GList * pos;


	for (pos = self->tracks; pos != NULL; pos = pos->next) { // Iterate over our KotoTracks
		KotoTrack * track = pos->data;

		if (!KOTO_TRACK(track)) { // Not a track
			continue; // Skip this
		}

		gchar * track_uuid = koto_track_get_uuid(track); // Get the track

		if (should_add) { // Should be adding
			koto_playlist_add_track_by_uuid(playlist, track_uuid, FALSE, TRUE); // Add the track to the playlist
		} else { // Should be removing
			koto_playlist_remove_track_by_uuid(playlist, track_uuid); // Remove the track from the playlist
		}
	}

	gtk_popover_popdown(GTK_POPOVER(self)); // Temporary to hopefully prevent a bork
}

void koto_add_remove_track_popover_handle_playlist_added(
	KotoCartographer * carto,
	KotoPlaylist * playlist,
	gpointer user_data
) {
	(void) carto;
	KotoAddRemoveTrackPopover * self = user_data;


	if (!KOTO_JS_ADD_REMOVE_TRACK_POPOVER(self)) {
		return;
	}

	koto_add_remove_track_popover_add_playlist(self, playlist);
}

void koto_add_remove_track_popover_handle_playlist_removed(
	KotoCartographer * carto,
	gchar * playlist_uuid,
	gpointer user_data
) {
	(void) carto;
	KotoAddRemoveTrackPopover * self = user_data;


	if (!KOTO_JS_ADD_REMOVE_TRACK_POPOVER(self)) {
		return;
	}

	koto_add_remove_track_popover_remove_playlist(self, playlist_uuid);
}

void koto_add_remove_track_popover_set_pointing_to_widget(
	KotoAddRemoveTrackPopover * self,
	GtkWidget * widget,
	GtkPositionType pos
) {
	if (!KOTO_JS_ADD_REMOVE_TRACK_POPOVER(self)) {
		return;
	}

	if (!GTK_IS_WIDGET(widget)) { // Not a widget
		return;
	}

	GtkWidget* existing_parent = gtk_widget_get_parent(GTK_WIDGET(self));


	if (existing_parent != NULL) {
		g_object_ref(GTK_WIDGET(self)); // Increment widget ref since unparent will do an unref
		gtk_widget_unparent(GTK_WIDGET(self)); // Unparent the popup
	}

	gtk_widget_insert_after(GTK_WIDGET(self), widget, gtk_widget_get_last_child(widget));
	gtk_popover_set_position(GTK_POPOVER(self), pos);
}

void koto_add_remove_track_popover_set_tracks(
	KotoAddRemoveTrackPopover * self,
	GList * tracks
) {
	if (!KOTO_JS_ADD_REMOVE_TRACK_POPOVER(self)) {
		return;
	}

	gint tracks_len = g_list_length(tracks);


	if (tracks_len == 0) { // No tracks
		return;
	}

	self->tracks = g_list_copy(tracks);
	GHashTable * playlists = koto_cartographer_get_playlists(koto_maps); // Get our playlists
	GHashTableIter playlists_iter;
	gpointer uuid, playlist_ptr;


	g_hash_table_iter_init(&playlists_iter, playlists); // Init our HashTable iterator

	while (g_hash_table_iter_next(&playlists_iter, &uuid, &playlist_ptr)) { // While we are iterating through our playlists
		KotoPlaylist * playlist = playlist_ptr;
		gboolean should_be_checked = FALSE;

		if (tracks_len > 1) { // More than one track
			GList * pos;
			for (pos = self->tracks; pos != NULL; pos = pos->next) { // Iterate over our tracks
				should_be_checked = (koto_playlist_get_position_of_track(playlist, pos->data) != -1);

				if (!should_be_checked) { // Should not be checked
					break;
				}
			}
		} else {
			KotoTrack * track = g_list_nth_data(self->tracks, 0); // Get the first track

			if (KOTO_IS_TRACK(track)) {
				gint pos = koto_playlist_get_position_of_track(playlist, track);
				should_be_checked = (pos != -1);
			}
		}

		GtkCheckButton * playlist_check = g_hash_table_lookup(self->playlist_uuid_to_checkbox, uuid); // Get the GtkCheckButton for this playlist

		if (GTK_IS_CHECK_BUTTON(playlist_check)) { // Is a checkbox
			gpointer sig_id_ptr = g_hash_table_lookup(self->checkbox_to_signal_ids, playlist_check);
			gulong check_button_sig_id = GPOINTER_TO_UINT(sig_id_ptr);
			g_signal_handler_block(playlist_check, check_button_sig_id); // Temporary ignore toggled signal, since set_active calls toggled
			gtk_check_button_set_active(playlist_check, should_be_checked); // Set active to our should_be_checked bool
			g_signal_handler_unblock(playlist_check, check_button_sig_id); // Unblock the signal
		}
	}
}

KotoAddRemoveTrackPopover * koto_add_remove_track_popover_new() {
	return g_object_new(KOTO_TYPE_ADD_REMOVE_TRACK_POPOVER, NULL);
}