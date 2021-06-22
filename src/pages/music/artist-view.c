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
#include "../../db/cartographer.h"
#include "../../indexer/structs.h"
#include "album-view.h"
#include "artist-view.h"
#include "config/config.h"
#include "koto-utils.h"

extern KotoCartographer * koto_maps;

struct _KotoArtistView {
	GObject parent_instance;
	KotoArtist * artist;
	GtkWidget * scrolled_window;
	GtkWidget * content;
	GtkWidget * album_list;

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
static void koto_artist_view_constructed(GObject * obj);

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
	gobject_class->constructed = koto_artist_view_constructed;
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

static void koto_artist_view_init(KotoArtistView * self) {
	self->artist = NULL;
	self->albums_to_component = g_hash_table_new(g_str_hash, g_str_equal);
}

static void koto_artist_view_constructed(GObject * obj) {
	KotoArtistView * self = KOTO_ARTIST_VIEW(obj);

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

	gtk_box_append(GTK_BOX(self->content), self->album_list); // Add the list

	gtk_widget_set_hexpand(GTK_WIDGET(self->album_list), TRUE);

	G_OBJECT_CLASS(koto_artist_view_parent_class)->constructed(obj);
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
	g_signal_connect(artist, "album-added", G_CALLBACK(koto_artist_view_handle_album_added), self);
	g_signal_connect(artist, "album-removed", G_CALLBACK(koto_artist_view_handle_album_removed), self);
}

GtkWidget * koto_artist_view_get_main(KotoArtistView * self) {
	return self->scrolled_window;
}

KotoArtistView * koto_artist_view_new(KotoArtist * artist) {
	return g_object_new(
		KOTO_TYPE_ARTIST_VIEW,
		"artist",
		artist,
		NULL
	);
}
