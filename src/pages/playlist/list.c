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
#include "../../components/track-table.h"
#include "../../db/cartographer.h"
#include "../../indexer/structs.h"
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

	KotoTrackTable * table;
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

	self->table = koto_track_table_new(); // Create our new KotoTrackTable
	GtkWidget * track_main = koto_track_table_get_main(self->table);
	gtk_box_append(GTK_BOX(self->content), track_main);
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

GtkWidget * koto_playlist_page_get_main(KotoPlaylistPage * self) {
	return self->main;
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

	koto_current_playlist_set_playlist(current_playlist, self->playlist, TRUE); // Switch to this playlist and start playing immediately
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

void koto_playlist_page_set_playlist_uuid(
	KotoPlaylistPage * self,
	gchar * playlist_uuid
) {
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
	koto_playlist_page_update_header(self); // Update our header

	g_signal_connect(playlist, "modified", G_CALLBACK(koto_playlist_page_handle_playlist_modified), self); // Handle playlist modification
	koto_track_table_set_playlist(self->table, playlist); // Set our track table
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
}

KotoPlaylistPage * koto_playlist_page_new(gchar * playlist_uuid) {
	return g_object_new(
		KOTO_TYPE_PLAYLIST_PAGE,
		"uuid",
		playlist_uuid,
		NULL
	);
}
