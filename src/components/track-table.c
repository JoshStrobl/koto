/* track-table.c
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
#include "../playlist/current.h"
#include "../playlist/playlist.h"
#include "../koto-button.h"
#include "../koto-utils.h"
#include "../koto-window.h"
#include "koto-action-bar.h"
#include "track-table.h"

extern KotoActionBar * action_bar;
extern KotoCartographer * koto_maps;
extern KotoCurrentPlaylist * current_playlist;
extern KotoPlaybackEngine * playback_engine;
extern KotoWindow * main_window;

struct _KotoTrackTable {
	GObject parent_instance;
	gchar * uuid;
	KotoPlaylist * playlist;

	GtkWidget * main;

	GtkListItemFactory * item_factory;
	GListModel * model;
	GtkSelectionModel * selection_model;

	GtkWidget * track_list_content;
	GtkWidget * track_list_header;
	GtkWidget * track_list_view;

	KotoButton * track_album_button;
	KotoButton * track_artist_button;
	KotoButton * track_num_button;
	KotoButton * track_title_button;

	GtkSizeGroup * track_pos_size_group;
	GtkSizeGroup * track_name_size_group;
	GtkSizeGroup * track_album_size_group;
	GtkSizeGroup * track_artist_size_group;
};

struct _KotoTrackTableClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE(KotoTrackTable, koto_track_table, G_TYPE_OBJECT);

static void koto_track_table_class_init(KotoTrackTableClass * c) {
	(void) c;
}

static void koto_track_table_init(KotoTrackTable * self) {
	self->main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->track_name_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	self->track_pos_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	self->track_album_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	self->track_artist_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	self->item_factory = gtk_signal_list_item_factory_new(); // Create a new signal list item factory
	g_signal_connect(self->item_factory, "setup", G_CALLBACK(koto_track_table_setup_track_item), self);
	g_signal_connect(self->item_factory, "bind", G_CALLBACK(koto_track_table_bind_track_item), self);

	self->track_list_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_css_class((self->track_list_content), "track-list-content");
	gtk_widget_set_hexpand(self->track_list_content, TRUE); // Expand horizontally
	gtk_widget_set_vexpand(self->track_list_content, TRUE); // Expand vertically

	self->track_list_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_add_css_class(self->track_list_header, "track-list-header");
	koto_track_table_create_tracks_header(self); // Create our tracks header content

	self->track_list_view = gtk_list_view_new(NULL, self->item_factory); // Create our list view with no model yet
	gtk_widget_add_css_class(self->track_list_view, "track-list-columned");
	gtk_widget_set_hexpand(self->track_list_view, TRUE); // Expand horizontally
	gtk_widget_set_vexpand(self->track_list_view, TRUE); // Expand vertically

	gtk_box_append(GTK_BOX(self->track_list_content), self->track_list_header);
	gtk_box_append(GTK_BOX(self->track_list_content), self->track_list_view);

	gtk_box_append(GTK_BOX(self->main), self->track_list_content);

	g_signal_connect(action_bar, "closed", G_CALLBACK(koto_track_table_handle_action_bar_closed), self); // Handle closed action bar
}

void koto_track_table_bind_track_item(
	GtkListItemFactory * factory,
	GtkListItem * item,
	KotoTrackTable * self
) {
	(void) factory;

	GtkWidget * box = gtk_list_item_get_child(item);
	GtkWidget * track_position_label = gtk_widget_get_first_child(box);
	GtkWidget * track_name_label = gtk_widget_get_next_sibling(track_position_label);
	GtkWidget * track_album_label = gtk_widget_get_next_sibling(track_name_label);
	GtkWidget * track_artist_label = gtk_widget_get_next_sibling(track_album_label);

	KotoTrack * track = gtk_list_item_get_item(item); // Get the track UUID from our model

	if (!KOTO_IS_TRACK(track)) {
		return;
	}

	gchar * track_name = NULL;
	gchar * album_uuid = NULL;
	gchar * artist_uuid = NULL;

	g_object_get(
		track,
		"parsed-name",
		&track_name,
		"album-uuid",
		&album_uuid,
		"artist-uuid",
		&artist_uuid,
		NULL
	);

	guint track_position = koto_playlist_get_position_of_track(self->playlist, track) + 1;

	gtk_label_set_label(GTK_LABEL(track_position_label), g_strdup_printf("%u", track_position)); // Set the track position
	gtk_label_set_label(GTK_LABEL(track_name_label), track_name); // Set our track name

	if (koto_utils_is_string_valid(album_uuid)) { // Is associated with an album
		KotoAlbum * album = koto_cartographer_get_album_by_uuid(koto_maps, album_uuid);

		if (KOTO_IS_ALBUM(album)) {
			gtk_label_set_label(GTK_LABEL(track_album_label), koto_album_get_name(album)); // Get the name of the album and set it to the label
		}
	}

	KotoArtist * artist = koto_cartographer_get_artist_by_uuid(koto_maps, artist_uuid);

	if (KOTO_IS_ARTIST(artist)) {
		gtk_label_set_label(GTK_LABEL(track_artist_label), koto_artist_get_name(artist)); // Get the name of the artist and set it to the label
	}

	GList * data = NULL;
	data = g_list_append(data, self); // Reference self first
	data = g_list_append(data, koto_track_get_uuid(track)); // Next reference the track UUID string

	g_object_set_data(G_OBJECT(box), "data", data); // Bind our list data
}

void koto_track_table_create_tracks_header(KotoTrackTable * self) {
	self->track_num_button = koto_button_new_with_icon("#", "pan-down-symbolic", "pan-up-symbolic", KOTO_BUTTON_PIXBUF_SIZE_SMALL);
	koto_button_add_click_handler(self->track_num_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_track_table_handle_track_num_clicked), self);
	koto_button_set_image_position(self->track_num_button, KOTO_BUTTON_IMAGE_POS_RIGHT); // Move the image to the right
	gtk_size_group_add_widget(self->track_pos_size_group, GTK_WIDGET(self->track_num_button));

	self->track_title_button = koto_button_new_plain("Title");
	koto_button_add_click_handler(self->track_title_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_track_table_handle_track_name_clicked), self);
	gtk_size_group_add_widget(self->track_name_size_group, GTK_WIDGET(self->track_title_button));

	self->track_album_button = koto_button_new_plain("Album");

	gtk_widget_set_margin_start(GTK_WIDGET(self->track_album_button), 50);
	koto_button_add_click_handler(self->track_album_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_track_table_handle_track_album_clicked), self);
	gtk_size_group_add_widget(self->track_album_size_group, GTK_WIDGET(self->track_album_button));

	self->track_artist_button = koto_button_new_plain("Artist");

	gtk_widget_set_margin_start(GTK_WIDGET(self->track_artist_button), 50);
	koto_button_add_click_handler(self->track_artist_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_track_table_handle_track_artist_clicked), self);
	gtk_size_group_add_widget(self->track_artist_size_group, GTK_WIDGET(self->track_artist_button));

	gtk_box_append(GTK_BOX(self->track_list_header), GTK_WIDGET(self->track_num_button));
	gtk_box_append(GTK_BOX(self->track_list_header), GTK_WIDGET(self->track_title_button));
	gtk_box_append(GTK_BOX(self->track_list_header), GTK_WIDGET(self->track_album_button));
	gtk_box_append(GTK_BOX(self->track_list_header), GTK_WIDGET(self->track_artist_button));
}

GtkWidget * koto_track_table_get_main(KotoTrackTable * self) {
	return self->main;
}

void koto_track_table_handle_action_bar_closed (
	KotoActionBar * bar,
	gpointer data
) {
	(void) bar;
	KotoTrackTable * self = data;

	if (!KOTO_IS_TRACK_TABLE(self)) { // Self is not a track table
		return;
	}

	if (!KOTO_IS_PLAYLIST(self->playlist)) { // No playlist set
		return;
	}

	gtk_selection_model_unselect_all(self->selection_model);
	gtk_widget_grab_focus(GTK_WIDGET(main_window)); // Focus on the window
}

void koto_track_table_handle_track_album_clicked (
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
) {
	(void) gesture;
	(void) n_press;
	(void) x;
	(void) y;
	KotoTrackTable * self = user_data;

	gtk_widget_add_css_class(GTK_WIDGET(self->track_album_button), "active");
	koto_button_hide_image(self->track_num_button); // Go back to hiding the image
	koto_track_table_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ALBUM);
}

void koto_track_table_handle_track_artist_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
) {
	(void) gesture;
	(void) n_press;
	(void) x;
	(void) y;
	KotoTrackTable * self = user_data;

	gtk_widget_add_css_class(GTK_WIDGET(self->track_artist_button), "active");
	koto_button_hide_image(self->track_num_button); // Go back to hiding the image
	koto_track_table_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ARTIST);
}

void koto_track_table_item_handle_clicked(
	GtkGesture * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
) {
	(void) gesture;
	(void) x;
	(void) y;

	if (n_press != 2) { // Not a double click or tap
		return;
	}

	GObject * track_item_as_object = G_OBJECT(user_data);

	if (!G_IS_OBJECT(track_item_as_object)) { // Not a GObject
		return;
	}

	GList * data = g_object_get_data(track_item_as_object, "data");

	KotoTrackTable * self = g_list_nth_data(data, 0);
	gchar * track_uuid = g_list_nth_data(data, 1);

	if (!koto_utils_is_string_valid(track_uuid)) { // Not a valid string
		return;
	}

	gtk_selection_model_unselect_all(self->selection_model);
	gtk_widget_grab_focus(GTK_WIDGET(main_window)); // Focus on the window
	koto_action_bar_toggle_reveal(action_bar, FALSE);
	koto_action_bar_close(action_bar); // Close the action bar
	koto_current_playlist_set_playlist(current_playlist, self->playlist, FALSE); // Set the current playlist to the artist's playlist but do not play immediately
	koto_playlist_set_track_as_current(self->playlist, track_uuid); // Set this track as the current one for the playlist
	koto_playback_engine_set_track_by_uuid(playback_engine, track_uuid, FALSE); // Tell our playback engine to start playing at this track
}

void koto_track_table_handle_track_name_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
) {
	(void) gesture;
	(void) n_press;
	(void) x;
	(void) y;
	KotoTrackTable * self = user_data;

	gtk_widget_add_css_class(GTK_WIDGET(self->track_title_button), "active");
	koto_button_hide_image(self->track_num_button); // Go back to hiding the image
	koto_track_table_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_SORT_BY_TRACK_NAME);
}

void koto_track_table_handle_track_num_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
) {
	(void) gesture;
	(void) n_press;
	(void) x;
	(void) y;
	KotoTrackTable * self = user_data;

	KotoPreferredModelType current_model = koto_playlist_get_current_model(self->playlist);

	if (current_model == KOTO_PREFERRED_MODEL_TYPE_DEFAULT) { // Set to newest currently
		koto_track_table_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_OLDEST_FIRST); // Sort reversed (oldest)
		koto_button_show_image(self->track_num_button, TRUE); // Use inverted value (pan-up-symbolic)
	} else {
		koto_track_table_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_DEFAULT); // Sort newest
		koto_button_show_image(self->track_num_button, FALSE); // Use pan down default
	}
}

void koto_track_table_handle_tracks_selected(
	GtkSelectionModel * model,
	guint position,
	guint n_items,
	gpointer user_data
) {
	(void) position;
	KotoTrackTable * self = user_data;

	if (!KOTO_IS_TRACK_TABLE(self)) { // Not a track table
		return;
	}

	if (n_items == 0) { // No items selected
		koto_action_bar_toggle_reveal(action_bar, FALSE); // Hide the action bar
		return;
	}

	GtkBitset * selected_items_bitset = gtk_selection_model_get_selection(model); // Get the selected items as a GtkBitSet
	GtkBitsetIter iter;
	GList * selected_tracks = NULL;
	GList * selected_tracks_pos = NULL;

	guint first_track_pos;

	if (!gtk_bitset_iter_init_first(&iter, selected_items_bitset, &first_track_pos)) { // Failed to get the first item
		return;
	}

	selected_tracks_pos = g_list_append(selected_tracks_pos, GUINT_TO_POINTER(first_track_pos));

	gboolean have_more_items = TRUE;

	while (have_more_items) { // While we are able to get selected items
		guint track_pos;
		have_more_items = gtk_bitset_iter_next(&iter, &track_pos);
		if (have_more_items) { // Got the next track
			selected_tracks_pos = g_list_append(selected_tracks_pos, GUINT_TO_POINTER(track_pos));
		}
	}

	GList * cur_pos_list;

	for (cur_pos_list = selected_tracks_pos; cur_pos_list != NULL; cur_pos_list = cur_pos_list->next) { // Iterate over every position that we accumulated
		KotoTrack * selected_track = g_list_model_get_item(self->model, GPOINTER_TO_UINT(cur_pos_list->data)); // Get the KotoTrack in the GListModel for this current position
		selected_tracks = g_list_append(selected_tracks, selected_track); // Add to selected tracks
	}

	koto_action_bar_set_tracks_in_playlist_selection(action_bar, koto_playlist_get_uuid(self->playlist), selected_tracks); // Set the tracks for the playlist selection
	koto_action_bar_toggle_reveal(action_bar, TRUE); // Show the items
}

void koto_track_table_set_model(
	KotoTrackTable * self,
	KotoPreferredModelType model
) {
	if (!KOTO_IS_TRACK_TABLE(self)) {
		return;
	}

	koto_playlist_apply_model(self->playlist, model); // Apply our new model
	self->model = G_LIST_MODEL(koto_playlist_get_store(self->playlist)); // Get the latest generated model / store and cast it as a GListModel

	if (model != KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ALBUM) { // Not sorting by album currently
		gtk_widget_remove_css_class(GTK_WIDGET(self->track_album_button), "active");
	}

	if (model != KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ARTIST) { // Not sorting by artist currently
		gtk_widget_remove_css_class(GTK_WIDGET(self->track_artist_button), "active");
	}

	if (model != KOTO_PREFERRED_MODEL_TYPE_SORT_BY_TRACK_NAME) { // Not sorting by track name
		gtk_widget_remove_css_class(GTK_WIDGET(self->track_title_button), "active");
	}

	self->selection_model = GTK_SELECTION_MODEL(gtk_multi_selection_new(self->model));
	g_signal_connect(self->selection_model, "selection-changed", G_CALLBACK(koto_track_table_handle_tracks_selected), self); // Bind to our selection changed

	gtk_list_view_set_model(GTK_LIST_VIEW(self->track_list_view), self->selection_model); // Set our multi selection model to our provided model
}

void koto_track_table_set_playlist_model (
	KotoTrackTable * self,
	KotoPreferredModelType model
) {
	if (!KOTO_IS_TRACK_TABLE(self)) { // Not a track table
		return;
	}

	if (!KOTO_IS_PLAYLIST(self->playlist)) { // Don't have a playlist yet
		return;
	}

	koto_playlist_apply_model(self->playlist, model); // Apply our new model
	self->model = G_LIST_MODEL(koto_playlist_get_store(self->playlist)); // Get the latest generated model / store and cast it as a GListModel

	if (model != KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ALBUM) { // Not sorting by album currently
		gtk_widget_remove_css_class(GTK_WIDGET(self->track_album_button), "active");
	}

	if (model != KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ARTIST) { // Not sorting by artist currently
		gtk_widget_remove_css_class(GTK_WIDGET(self->track_artist_button), "active");
	}

	if (model != KOTO_PREFERRED_MODEL_TYPE_SORT_BY_TRACK_NAME) { // Not sorting by track name
		gtk_widget_remove_css_class(GTK_WIDGET(self->track_title_button), "active");
	}

	self->selection_model = GTK_SELECTION_MODEL(gtk_multi_selection_new(self->model));
	g_signal_connect(self->selection_model, "selection-changed", G_CALLBACK(koto_track_table_handle_tracks_selected), self); // Bind to our selection changed

	gtk_list_view_set_model(GTK_LIST_VIEW(self->track_list_view), self->selection_model); // Set our multi selection model to our provided model

	KotoPreferredModelType current_model = koto_playlist_get_current_model(self->playlist); // Get the current model

	if (current_model == KOTO_PREFERRED_MODEL_TYPE_OLDEST_FIRST) {
		koto_button_show_image(self->track_num_button, TRUE); // Immediately use pan-up-symbolic
	}
}

void koto_track_table_set_playlist(
	KotoTrackTable * self,
	KotoPlaylist * playlist
) {
	if (!KOTO_IS_TRACK_TABLE(self)) {
		return;
	}

	if (!KOTO_IS_PLAYLIST(playlist)) { // Not a playlist
		return;
	}

	self->playlist = playlist;
	koto_track_table_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_DEFAULT); // TODO: Enable this to be changed
}

void koto_track_table_setup_track_item(
	GtkListItemFactory * factory,
	GtkListItem * item,
	gpointer user_data
) {
	(void) factory;
	KotoTrackTable * self = user_data;

	if (!KOTO_IS_TRACK_TABLE(self)) {
		return;
	}

	GtkWidget * item_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); // Have a horizontal box for our content

	gtk_widget_add_css_class(item_content, "track-list-columned-item");

	GtkWidget * track_number = gtk_label_new(NULL); // Our track number

	gtk_label_set_xalign(GTK_LABEL(track_number), 0);
	gtk_widget_add_css_class(track_number, "track-column-number");
	gtk_widget_set_halign(track_number, GTK_ALIGN_END);
	gtk_widget_set_hexpand(track_number, FALSE);
	gtk_widget_set_size_request(track_number, 150, -1);
	gtk_box_append(GTK_BOX(item_content), track_number);
	gtk_size_group_add_widget(self->track_pos_size_group, track_number);

	GtkWidget * track_name = gtk_label_new(NULL); // Our track name

	gtk_label_set_xalign(GTK_LABEL(track_name), 0);
	gtk_widget_add_css_class(track_name, "track-column-name");
	gtk_widget_set_halign(track_name, GTK_ALIGN_START);
	gtk_widget_set_hexpand(track_name, FALSE);
	gtk_widget_set_size_request(track_name, 350, -1);
	gtk_box_append(GTK_BOX(item_content), track_name);
	gtk_size_group_add_widget(self->track_name_size_group, track_name);

	GtkWidget * track_album = gtk_label_new(NULL); // Our track album

	gtk_label_set_xalign(GTK_LABEL(track_album), 0);
	gtk_widget_add_css_class(track_album, "track-column-album");
	gtk_widget_set_halign(track_album, GTK_ALIGN_START);
	gtk_widget_set_hexpand(track_album, FALSE);
	gtk_widget_set_margin_start(track_album, 50);
	gtk_widget_set_size_request(track_album, 350, -1);
	gtk_box_append(GTK_BOX(item_content), track_album);
	gtk_size_group_add_widget(self->track_album_size_group, track_album);

	GtkWidget * track_artist = gtk_label_new(NULL); // Our track artist

	gtk_label_set_xalign(GTK_LABEL(track_artist), 0);
	gtk_widget_add_css_class(track_artist, "track-column-artist");
	gtk_widget_set_halign(track_artist, GTK_ALIGN_START);
	gtk_widget_set_hexpand(track_artist, TRUE);
	gtk_widget_set_margin_start(track_artist, 50);
	gtk_widget_set_size_request(track_artist, 350, -1);
	gtk_box_append(GTK_BOX(item_content), track_artist);
	gtk_size_group_add_widget(self->track_artist_size_group, track_artist);

	gtk_list_item_set_child(item, item_content);

	GtkGesture * double_click_gesture = gtk_gesture_click_new(); // Create our new GtkGesture for double-click handling
	gtk_widget_add_controller(item_content, GTK_EVENT_CONTROLLER(double_click_gesture)); // Have our item handle double clicking
	g_signal_connect(double_click_gesture, "released", G_CALLBACK(koto_track_table_item_handle_clicked), item_content);
}

KotoTrackTable * koto_track_table_new() {
	return g_object_new(
		KOTO_TYPE_TRACK_TABLE,
		NULL
	);
}