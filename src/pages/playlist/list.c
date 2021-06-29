/* list.c
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
#include "../../components/koto-action-bar.h"
#include "../../components/koto-cover-art-button.h"
#include "../../db/cartographer.h"
#include "../../playlist/current.h"
#include "../../playlist/playlist.h"
#include "../../koto-button.h"
#include "../../koto-window.h"
#include "../../playlist/create-modify-dialog.h"
#include "list.h"

extern KotoActionBar * action_bar;
extern KotoCartographer * koto_maps;
extern KotoCreateModifyPlaylistDialog * playlist_create_modify_dialog;
extern KotoCurrentPlaylist * current_playlist;
extern KotoWindow * main_window;

enum {
	PROP_0,
	PROP_PLAYLIST_UUID,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};

struct _KotoPlaylistPage {
	GObject parent_instance;
	KotoPlaylist * playlist;
	gchar * uuid;

	GtkWidget * main; // Our Scrolled Window
	GtkWidget * content; // Content inside scrolled window
	GtkWidget * header;

	KotoCoverArtButton * playlist_image;
	GtkWidget * name_label;
	GtkWidget * tracks_count_label;
	GtkWidget * type_label;
	KotoButton * favorite_button;
	KotoButton * edit_button;

	GtkListItemFactory * item_factory;
	GListModel * model;
	GtkSelectionModel * selection_model;

	GtkWidget * track_list_content;
	GtkWidget * track_list_header;
	GtkWidget * track_list_view;

	KotoButton * track_num_button;
	KotoButton * track_title_button;
	KotoButton * track_album_button;
	KotoButton * track_artist_button;

	GtkSizeGroup * track_pos_size_group;
	GtkSizeGroup * track_name_size_group;
	GtkSizeGroup * track_album_size_group;
	GtkSizeGroup * track_artist_size_group;
};

struct _KotoPlaylistPageClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE(KotoPlaylistPage, koto_playlist_page, G_TYPE_OBJECT);

static void koto_playlist_page_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_playlist_page_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_playlist_page_class_init(KotoPlaylistPageClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->get_property = koto_playlist_page_get_property;
	gobject_class->set_property = koto_playlist_page_set_property;

	props[PROP_PLAYLIST_UUID] = g_param_spec_string(
		"uuid",
		"UUID of associated Playlist",
		"UUID of associated Playlist",
		NULL,
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_playlist_page_init(KotoPlaylistPage * self) {
	self->track_name_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	self->track_pos_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	self->track_album_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	self->track_artist_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	self->main = gtk_scrolled_window_new(); // Create our scrolled window
	self->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_css_class(self->content, "playlist-page");

	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->main), self->content);

	self->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); // Create a horizontal box
	gtk_widget_add_css_class(self->header, "playlist-page-header");
	gtk_box_prepend(GTK_BOX(self->content), self->header);

	self->playlist_image = koto_cover_art_button_new(220, 220, NULL); // Create our Cover Art Button with no art by default
	KotoButton * cover_art_button = koto_cover_art_button_get_button(self->playlist_image); // Get the button for the cover art

	koto_button_add_click_handler(cover_art_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_playlist_page_handle_cover_art_clicked), self);

	GtkWidget * info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	gtk_widget_set_size_request(info_box, -1, 220);
	gtk_widget_add_css_class(info_box, "playlist-page-header-info");
	gtk_widget_set_hexpand(info_box, TRUE);

	self->type_label = gtk_label_new(NULL); // Create our type label
	gtk_widget_set_halign(self->type_label, GTK_ALIGN_START);

	self->name_label = gtk_label_new(NULL);
	gtk_widget_set_halign(self->name_label, GTK_ALIGN_START);

	self->tracks_count_label = gtk_label_new(NULL);
	gtk_widget_set_halign(self->tracks_count_label, GTK_ALIGN_START);
	gtk_widget_set_valign(self->tracks_count_label, GTK_ALIGN_END);

	gtk_box_append(GTK_BOX(info_box), self->type_label);
	gtk_box_append(GTK_BOX(info_box), self->name_label);
	gtk_box_append(GTK_BOX(info_box), self->tracks_count_label);

	self->favorite_button = koto_button_new_with_icon(NULL, "emblem-favorite-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	self->edit_button = koto_button_new_with_icon(NULL, "emblem-system-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	koto_button_add_click_handler(self->edit_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_playlist_page_handle_edit_button_clicked), self); // Set up our binding

	gtk_box_append(GTK_BOX(self->header), koto_cover_art_button_get_main(self->playlist_image)); // Add the Cover Art Button main overlay
	gtk_box_append(GTK_BOX(self->header), info_box); // Add our info box
	gtk_box_append(GTK_BOX(self->header), GTK_WIDGET(self->favorite_button)); // Add the favorite button
	gtk_box_append(GTK_BOX(self->header), GTK_WIDGET(self->edit_button)); // Add the edit button

	self->item_factory = gtk_signal_list_item_factory_new(); // Create a new signal list item factory
	g_signal_connect(self->item_factory, "setup", G_CALLBACK(koto_playlist_page_setup_track_item), self);
	g_signal_connect(self->item_factory, "bind", G_CALLBACK(koto_playlist_page_bind_track_item), self);

	self->track_list_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_css_class((self->track_list_content), "track-list-content");
	gtk_widget_set_hexpand(self->track_list_content, TRUE); // Expand horizontally
	gtk_widget_set_vexpand(self->track_list_content, TRUE); // Expand vertically

	self->track_list_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_add_css_class(self->track_list_header, "track-list-header");
	koto_playlist_page_create_tracks_header(self); // Create our tracks header content

	self->track_list_view = gtk_list_view_new(NULL, self->item_factory); // Create our list view with no model yet
	gtk_widget_add_css_class(self->track_list_view, "track-list-columned");
	gtk_widget_set_hexpand(self->track_list_view, TRUE); // Expand horizontally
	gtk_widget_set_vexpand(self->track_list_view, TRUE); // Expand vertically

	gtk_box_append(GTK_BOX(self->track_list_content), self->track_list_header);
	gtk_box_append(GTK_BOX(self->track_list_content), self->track_list_view);

	gtk_box_append(GTK_BOX(self->content), self->track_list_content);

	g_signal_connect(action_bar, "closed", G_CALLBACK(koto_playlist_page_handle_action_bar_closed), self); // Handle closed action bar
}

static void koto_playlist_page_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoPlaylistPage * self = KOTO_PLAYLIST_PAGE(obj);

	switch (prop_id) {
		case PROP_PLAYLIST_UUID:
			g_value_set_string(val, self->uuid);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_playlist_page_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoPlaylistPage * self = KOTO_PLAYLIST_PAGE(obj);

	switch (prop_id) {
		case PROP_PLAYLIST_UUID:
			koto_playlist_page_set_playlist_uuid(self, g_strdup(g_value_get_string(val))); // Call to our playlist UUID set function
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_playlist_page_bind_track_item(
	GtkListItemFactory * factory,
	GtkListItem * item,
	KotoPlaylistPage * self
) {
	(void) factory;

	GtkWidget * track_position_label = gtk_widget_get_first_child(gtk_list_item_get_child(item));
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

	KotoAlbum * album = koto_cartographer_get_album_by_uuid(koto_maps, album_uuid);

	if (KOTO_IS_ALBUM(album)) {
		gtk_label_set_label(GTK_LABEL(track_album_label), koto_album_get_name(album)); // Get the name of the album and set it to the label
	}

	KotoArtist * artist = koto_cartographer_get_artist_by_uuid(koto_maps, artist_uuid);

	if (KOTO_IS_ARTIST(artist)) {
		gtk_label_set_label(GTK_LABEL(track_artist_label), koto_artist_get_name(artist)); // Get the name of the artist and set it to the label
	}
}

void koto_playlist_page_create_tracks_header(KotoPlaylistPage * self) {
	self->track_num_button = koto_button_new_with_icon("#", "pan-down-symbolic", "pan-up-symbolic", KOTO_BUTTON_PIXBUF_SIZE_SMALL);
	koto_button_add_click_handler(self->track_num_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_playlist_page_handle_track_num_clicked), self);
	koto_button_set_image_position(self->track_num_button, KOTO_BUTTON_IMAGE_POS_RIGHT); // Move the image to the right
	gtk_size_group_add_widget(self->track_pos_size_group, GTK_WIDGET(self->track_num_button));

	self->track_title_button = koto_button_new_plain("Title");
	koto_button_add_click_handler(self->track_title_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_playlist_page_handle_track_name_clicked), self);
	gtk_size_group_add_widget(self->track_name_size_group, GTK_WIDGET(self->track_title_button));

	self->track_album_button = koto_button_new_plain("Album");

	gtk_widget_set_margin_start(GTK_WIDGET(self->track_album_button), 50);
	koto_button_add_click_handler(self->track_album_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_playlist_page_handle_track_album_clicked), self);
	gtk_size_group_add_widget(self->track_album_size_group, GTK_WIDGET(self->track_album_button));

	self->track_artist_button = koto_button_new_plain("Artist");

	gtk_widget_set_margin_start(GTK_WIDGET(self->track_artist_button), 50);
	koto_button_add_click_handler(self->track_artist_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_playlist_page_handle_track_artist_clicked), self);
	gtk_size_group_add_widget(self->track_artist_size_group, GTK_WIDGET(self->track_artist_button));

	gtk_box_append(GTK_BOX(self->track_list_header), GTK_WIDGET(self->track_num_button));
	gtk_box_append(GTK_BOX(self->track_list_header), GTK_WIDGET(self->track_title_button));
	gtk_box_append(GTK_BOX(self->track_list_header), GTK_WIDGET(self->track_album_button));
	gtk_box_append(GTK_BOX(self->track_list_header), GTK_WIDGET(self->track_artist_button));
}

GtkWidget * koto_playlist_page_get_main(KotoPlaylistPage * self) {
	return self->main;
}

void koto_playlist_page_handle_action_bar_closed(
	KotoActionBar * bar,
	gpointer data
) {
	(void) bar;
	KotoPlaylistPage * self = data;

	if (!KOTO_IS_PLAYLIST(self->playlist)) { // No playlist set
		return;
	}

	gtk_selection_model_unselect_all(self->selection_model);
	gtk_widget_grab_focus(GTK_WIDGET(main_window)); // Focus on the window
}

void koto_playlist_page_handle_cover_art_clicked(
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
	KotoPlaylistPage * self = user_data;

	if (!KOTO_IS_PLAYLIST(self->playlist)) { // No playlist set
		return;
	}

	koto_current_playlist_set_playlist(current_playlist, self->playlist);
}

void koto_playlist_page_handle_edit_button_clicked(
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
	KotoPlaylistPage * self = user_data;

	koto_create_modify_playlist_dialog_set_playlist_uuid(playlist_create_modify_dialog, koto_playlist_get_uuid(self->playlist));
	koto_window_show_dialog(main_window, "create-modify-playlist");
}

void koto_playlist_page_handle_playlist_modified(
	KotoPlaylist * playlist,
	gpointer user_data
) {
	if (!KOTO_IS_PLAYLIST(playlist)) {
		return;
	}

	KotoPlaylistPage * self = user_data;

	if (!KOTO_IS_PLAYLIST_PAGE(self)) {
		return;
	}

	gchar * artwork = koto_playlist_get_artwork(playlist); // Get the artwork

	if (koto_utils_is_string_valid(artwork)) { // Have valid artwork
		koto_cover_art_button_set_art_path(self->playlist_image, artwork); // Update our Koto Cover Art Button
	}

	gchar * name = koto_playlist_get_name(playlist); // Get the name

	if (koto_utils_is_string_valid(name)) { // Have valid name
		gtk_label_set_label(GTK_LABEL(self->name_label), name); // Update the name label
	}
}

void koto_playlist_page_handle_track_album_clicked(
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
	KotoPlaylistPage * self = user_data;

	gtk_widget_add_css_class(GTK_WIDGET(self->track_album_button), "active");
	koto_button_hide_image(self->track_num_button); // Go back to hiding the image
	koto_playlist_page_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ALBUM);
}

void koto_playlist_page_handle_track_artist_clicked(
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
	KotoPlaylistPage * self = user_data;

	gtk_widget_add_css_class(GTK_WIDGET(self->track_artist_button), "active");
	koto_button_hide_image(self->track_num_button); // Go back to hiding the image
	koto_playlist_page_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ARTIST);
}

void koto_playlist_page_handle_track_name_clicked(
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
	KotoPlaylistPage * self = user_data;

	gtk_widget_add_css_class(GTK_WIDGET(self->track_title_button), "active");
	koto_button_hide_image(self->track_num_button); // Go back to hiding the image
	koto_playlist_page_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_SORT_BY_TRACK_NAME);
}

void koto_playlist_page_handle_track_num_clicked(
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
	KotoPlaylistPage * self = user_data;

	KotoPreferredModelType current_model = koto_playlist_get_current_model(self->playlist);

	if (current_model == KOTO_PREFERRED_MODEL_TYPE_DEFAULT) { // Set to newest currently
		koto_playlist_page_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_OLDEST_FIRST); // Sort reversed (oldest)
		koto_button_show_image(self->track_num_button, TRUE); // Use inverted value (pan-up-symbolic)
	} else {
		koto_playlist_page_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_DEFAULT); // Sort newest
		koto_button_show_image(self->track_num_button, FALSE); // Use pan down default
	}
}

void koto_playlist_page_handle_tracks_selected(
	GtkSelectionModel * model,
	guint position,
	guint n_items,
	gpointer user_data
) {
	(void) position;
	KotoPlaylistPage * self = user_data;

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

	koto_action_bar_set_tracks_in_playlist_selection(action_bar, self->uuid, selected_tracks); // Set the tracks for the playlist selection
	koto_action_bar_toggle_reveal(action_bar, TRUE); // Show the items
}

void koto_playlist_page_set_playlist_uuid(
	KotoPlaylistPage * self,
	gchar * playlist_uuid
) {
	if (!KOTO_IS_PLAYLIST_PAGE(self)) {
		return;
	}

	if (!KOTO_IS_PLAYLIST_PAGE(self)) {
		return;
	}

	if (!koto_utils_is_string_valid(playlist_uuid)) { // Provided UUID string is not valid
		return;
	}

	if (!koto_cartographer_has_playlist_by_uuid(koto_maps, playlist_uuid)) { // If we don't have this playlist
		return;
	}

	self->uuid = g_strdup(playlist_uuid); // Duplicate the playlist UUID
	KotoPlaylist * playlist = koto_cartographer_get_playlist_by_uuid(koto_maps, self->uuid);

	self->playlist = playlist;
	koto_playlist_page_set_playlist_model(self, KOTO_PREFERRED_MODEL_TYPE_DEFAULT); // TODO: Enable this to be changed
	koto_playlist_page_update_header(self); // Update our header

	g_signal_connect(playlist, "modified", G_CALLBACK(koto_playlist_page_handle_playlist_modified), self); // Handle playlist modification
}

void koto_playlist_page_set_playlist_model(
	KotoPlaylistPage * self,
	KotoPreferredModelType model
) {
	if (!KOTO_IS_PLAYLIST_PAGE(self)) {
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
	g_signal_connect(self->selection_model, "selection-changed", G_CALLBACK(koto_playlist_page_handle_tracks_selected), self); // Bind to our selection changed

	gtk_list_view_set_model(GTK_LIST_VIEW(self->track_list_view), self->selection_model); // Set our multi selection model to our provided model
}

void koto_playlist_page_setup_track_item(
	GtkListItemFactory * factory,
	GtkListItem * item,
	gpointer user_data
) {
	(void) factory;
	KotoPlaylistPage * self = user_data;

	if (!KOTO_IS_PLAYLIST_PAGE(self)) {
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
}

void koto_playlist_page_update_header(KotoPlaylistPage * self) {
	if (!KOTO_IS_PLAYLIST_PAGE(self)) {
		return;
	}

	if (!KOTO_IS_PLAYLIST(self->playlist)) { // Not a playlist
		return;
	}

	gboolean ephemeral = TRUE;

	g_object_get(
		self->playlist,
		"ephemeral",
		&ephemeral,
		NULL
	);

	gtk_label_set_text(GTK_LABEL(self->type_label), ephemeral ? "Generated playlist" : "Curated playlist");

	gtk_label_set_text(GTK_LABEL(self->name_label), koto_playlist_get_name(self->playlist)); // Set the name label to our playlist name
	guint track_count = koto_playlist_get_length(self->playlist); // Get the number of tracks

	gtk_label_set_text(GTK_LABEL(self->tracks_count_label), g_strdup_printf(track_count != 1 ? "%u tracks" : "%u track", track_count)); // Set the text to "N tracks" where N is the number

	gchar * artwork = koto_playlist_get_artwork(self->playlist);

	if (koto_utils_is_string_valid(artwork)) { // Have artwork
		koto_cover_art_button_set_art_path(self->playlist_image, artwork); // Update our artwork
	}

	KotoPreferredModelType current_model = koto_playlist_get_current_model(self->playlist); // Get the current model

	if (current_model == KOTO_PREFERRED_MODEL_TYPE_OLDEST_FIRST) {
		koto_button_show_image(self->track_num_button, TRUE); // Immediately use pan-up-symbolic
	}
}

KotoPlaylistPage * koto_playlist_page_new(gchar * playlist_uuid) {
	return g_object_new(
		KOTO_TYPE_PLAYLIST_PAGE,
		"uuid",
		playlist_uuid,
		NULL
	);
}
