/* artist-view.c
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
#include "../../components/cover-art-button.h"
#include "../../components/track-table.h"
#include "../../db/cartographer.h"
#include "../../indexer/artist-playlist-funcs.h"
#include "../../indexer/structs.h"
#include "../../playlist/current.h"
#include "album-view.h"
#include "artist-view.h"
#include "config/config.h"
#include "koto-utils.h"

extern KotoCurrentPlaylist * current_playlist;
extern KotoCartographer * koto_maps;

struct _KotoArtistView {
	GObject parent_instance;
	KotoArtist * artist;
	GtkWidget * scrolled_window;
	GtkWidget * content;
	GtkWidget * album_list;

	GtkWidget * no_albums_view;
	GtkWidget * no_albums_artist_header;
	KotoCoverArtButton * no_albums_artist_button;
	GtkWidget * no_albums_artist_label;

	KotoTrackTable * table;
	GHashTable * albums_to_component;
};

G_DEFINE_TYPE(KotoArtistView, koto_artist_view, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_ARTIST,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};

static void koto_artist_view_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_artist_view_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_artist_view_class_init(KotoArtistViewClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_artist_view_set_property;
	gobject_class->get_property = koto_artist_view_get_property;

	props[PROP_ARTIST] = g_param_spec_object(
		"artist",
		"Artist",
		"Artist",
		KOTO_TYPE_ARTIST,
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_artist_view_init(KotoArtistView * self) {
	self->artist = NULL;
	self->albums_to_component = g_hash_table_new(g_str_hash, g_str_equal);

	self->scrolled_window = gtk_scrolled_window_new(); // Create our scrolled window
	self->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); // Create our content as a GtkBox

	gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(self->scrolled_window), TRUE);
	gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(self->scrolled_window), TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->scrolled_window), self->content); // Add the content as the widget for the scrolled window
	gtk_widget_add_css_class(GTK_WIDGET(self->scrolled_window), "artist-view");
	gtk_widget_add_css_class(GTK_WIDGET(self->content), "artist-view-content");

	self->album_list = gtk_flow_box_new(); // Create our list of our albums
	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(self->album_list), GTK_SELECTION_NONE);
	gtk_widget_add_css_class(self->album_list, "album-list");

	self->no_albums_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_css_class(self->no_albums_view, "no-albums-view");

	self->no_albums_artist_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_add_css_class(self->no_albums_artist_header, "no-albums-view-header"),

	self->no_albums_artist_button = koto_cover_art_button_new(220, 220, NULL);
	KotoButton * cover_art_button = koto_cover_art_button_get_button(self->no_albums_artist_button); // Get the button for the KotoCoverArt
	koto_button_add_click_handler(cover_art_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_artist_view_toggle_playback), self); // Handle clicking the button

	self->no_albums_artist_label = gtk_label_new(NULL); // Create a label without any artist name

	gtk_box_append(GTK_BOX(self->no_albums_artist_header), koto_cover_art_button_get_main(self->no_albums_artist_button)); // Add our button to the header
	gtk_box_append(GTK_BOX(self->no_albums_artist_header), self->no_albums_artist_label);

	gtk_box_append(GTK_BOX(self->no_albums_view), self->no_albums_artist_header); // Add the header to the no albums view

	self->table = koto_track_table_new(); //Create our track table
	gtk_box_append(GTK_BOX(self->no_albums_view), koto_track_table_get_main(self->table)); // Add the table to the no albums view

	gtk_box_append(GTK_BOX(self->content), self->album_list); // Add the album flowbox
	gtk_box_append(GTK_BOX(self->content), self->no_albums_view); // Add the no albums view just in case we do not have any albums

	gtk_widget_set_hexpand(GTK_WIDGET(self->album_list), TRUE);
	gtk_widget_set_hexpand(GTK_WIDGET(self->no_albums_view), TRUE);
}

static void koto_artist_view_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoArtistView * self = KOTO_ARTIST_VIEW(obj);

	switch (prop_id) {
		case PROP_ARTIST:
			g_value_set_object(val, self->artist);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_artist_view_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoArtistView * self = KOTO_ARTIST_VIEW(obj);

	switch (prop_id) {
		case PROP_ARTIST:
			koto_artist_view_set_artist(self, (KotoArtist*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_artist_view_add_album(
	KotoArtistView * self,
	KotoAlbum * album
) {
	if (!KOTO_IS_ARTIST_VIEW(self)) {
		return;
	}

	if (!KOTO_IS_ALBUM(album)) {
		return;
	}

	gchar * album_uuid = koto_album_get_uuid(album); // Get the album UUID

	if (g_hash_table_contains(self->albums_to_component, album_uuid)) { // Already added this album
		return;
	}

	KotoAlbumView * album_view = koto_album_view_new(album); // Create our new album view
	GtkWidget * album_view_main = koto_album_view_get_main(album_view);

	gtk_flow_box_insert(GTK_FLOW_BOX(self->album_list), album_view_main, -1); // Append the album view to the album list
	g_hash_table_replace(self->albums_to_component, album_uuid, album_view_main);

	gtk_widget_hide(self->no_albums_view); // Hide the no album view
	gtk_widget_show(self->album_list); // Show the album list now that it has albums
}

GtkWidget * koto_artist_view_get_main(KotoArtistView * self) {
	return self->scrolled_window;
}

void koto_artist_view_handle_album_added(
	KotoArtist * artist,
	KotoAlbum * album,
	gpointer user_data
) {
	KotoArtistView * self = user_data;

	if (!KOTO_IS_ARTIST_VIEW(self)) {
		return;
	}

	if (!KOTO_IS_ARTIST(artist)) {
		return;
	}

	if (!KOTO_IS_ALBUM(album)) {
		return;
	}

	if (g_strcmp0(koto_artist_get_uuid(self->artist), koto_artist_get_uuid(artist)) != 0) { // Not the same artist
		return;
	}

	koto_artist_view_add_album(self, album); // Add the album if necessary
}

void koto_artist_view_handle_album_removed(
	KotoArtist * artist,
	gchar * album_uuid,
	gpointer user_data
) {
	KotoArtistView * self = user_data;

	if (!KOTO_IS_ARTIST_VIEW(self)) {
		return;
	}

	if (g_strcmp0(koto_artist_get_uuid(self->artist), koto_artist_get_uuid(artist)) != 0) { // Not the same artist
		return;
	}

	GtkWidget * album_view = g_hash_table_lookup(self->albums_to_component, album_uuid); // Get the album view if it exists

	if (!GTK_IS_WIDGET(album_view)) { // Not a widget
		return;
	}

	gtk_flow_box_remove(GTK_FLOW_BOX(self->album_list), album_view); // Remove the album
	g_hash_table_remove(self->albums_to_component, album_uuid); // Remove the album from our hash table
}

void koto_artist_view_handle_artist_name_changed(
	KotoArtist * artist,
	guint prop_id,
	KotoArtistView * self
) {
	(void) prop_id;
	gtk_label_set_text(GTK_LABEL(self->no_albums_artist_label), koto_artist_get_name(artist)); // Update the label for the artist now that it has changed
}

void koto_artist_view_handle_has_no_albums(
	KotoArtist * artist,
	gpointer user_data
) {
	(void) artist;
	KotoArtistView * self = user_data;

	if (!KOTO_IS_ARTIST_VIEW(self)) {
		return;
	}

	gtk_widget_hide(self->album_list); // Hide our album list flowbox
	gtk_widget_show(self->no_albums_view); // Show the no albums view
}

void koto_artist_view_set_artist(
	KotoArtistView * self,
	KotoArtist * artist
) {
	if (!KOTO_IS_ARTIST_VIEW(self)) { // Not an Artist view
		return;
	}

	if (!KOTO_IS_ARTIST(artist)) {
		return;
	}

	self->artist = artist;
	koto_track_table_set_playlist(self->table, koto_artist_get_playlist(self->artist)); // Set our track table to the artist's playlist
	gtk_label_set_text(GTK_LABEL(self->no_albums_artist_label), koto_artist_get_name(self->artist)); // Update our label with the name of the artist

	g_signal_connect(artist, "album-added", G_CALLBACK(koto_artist_view_handle_album_added), self);
	g_signal_connect(artist, "album-removed", G_CALLBACK(koto_artist_view_handle_album_removed), self);
	g_signal_connect(artist, "has-no-albums", G_CALLBACK(koto_artist_view_handle_has_no_albums), self);
	g_signal_connect(artist, "notify::name", G_CALLBACK(koto_artist_view_handle_artist_name_changed), self);
}

void koto_artist_view_toggle_playback(
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
	KotoArtistView * self = data;

	if (!KOTO_IS_ARTIST_VIEW(self)) {
		return;
	}

	KotoPlaylist * artist_playlist = koto_artist_get_playlist(self->artist);

	if (!KOTO_IS_PLAYLIST(artist_playlist)) { // Failed our is playlist check for the artist playlist
		return;
	}

	koto_current_playlist_set_playlist(current_playlist, artist_playlist, TRUE, FALSE); // Set our playlist to the one associated with the Artist and start playback immediately
}

KotoArtistView * koto_artist_view_new(KotoArtist * artist) {
	return g_object_new(
		KOTO_TYPE_ARTIST_VIEW,
		"artist",
		artist,
		NULL
	);
}
