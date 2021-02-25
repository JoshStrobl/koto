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
#include "../../indexer/album.h"
#include "../../indexer/artist.h"
#include "album-view.h"
#include "artist-view.h"
#include "koto-config.h"
#include "koto-utils.h"

struct _KotoArtistView {
	GObject parent_instance;
	KotoIndexedArtist *artist;
	GtkWidget *scrolled_window;
	GtkWidget *content;
	GtkWidget *favorites_list;
	GtkWidget *album_list;

	GHashTable *albums_to_component;

	gboolean constructed;
};

G_DEFINE_TYPE(KotoArtistView, koto_artist_view, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_ARTIST,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };
static void koto_artist_view_constructed(GObject *obj);
static void koto_artist_view_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_artist_view_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_artist_view_class_init(KotoArtistViewClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->constructed = koto_artist_view_constructed;
	gobject_class->set_property = koto_artist_view_set_property;
	gobject_class->get_property = koto_artist_view_get_property;

	props[PROP_ARTIST] = g_param_spec_object(
		"artist",
		"Artist",
		"Artist",
		KOTO_TYPE_INDEXED_ARTIST,
		G_PARAM_CONSTRUCT_ONLY|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_artist_view_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoArtistView *self = KOTO_ARTIST_VIEW(obj);

	switch (prop_id) {
		case PROP_ARTIST:
			g_value_set_object(val, self->artist);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_artist_view_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoArtistView *self = KOTO_ARTIST_VIEW(obj);

	switch (prop_id) {
		case PROP_ARTIST:
			koto_artist_view_add_artist(self, (KotoIndexedArtist*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_artist_view_init(KotoArtistView *self) {
	self->artist = NULL;
	self->constructed = FALSE;
}

static void koto_artist_view_constructed(GObject *obj) {
	KotoArtistView *self = KOTO_ARTIST_VIEW(obj);
	self->albums_to_component = g_hash_table_new(g_str_hash, g_str_equal);
	self->scrolled_window = gtk_scrolled_window_new(); // Create our scrolled window
	self->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); // Create our content as a GtkBox

	gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(self->scrolled_window), TRUE);
	gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(self->scrolled_window), TRUE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->scrolled_window), self->content); // Add the content as the widget for the scrolled window
	gtk_widget_add_css_class(GTK_WIDGET(self->scrolled_window), "artist-view");
	gtk_widget_add_css_class(GTK_WIDGET(self->content), "artist-view-content");

	self->favorites_list = gtk_flow_box_new(); // Create our favorites list
	gtk_flow_box_set_activate_on_single_click(GTK_FLOW_BOX(self->favorites_list), TRUE);
	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(self->favorites_list), GTK_SELECTION_NONE);
	gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(self->favorites_list), 6);
	gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(self->favorites_list), 6);
	gtk_widget_add_css_class(GTK_WIDGET(self->favorites_list), "album-strip");
	gtk_widget_set_halign(self->favorites_list, GTK_ALIGN_START);

	self->album_list = gtk_flow_box_new(); // Create our list of our albums
	gtk_flow_box_set_activate_on_single_click(GTK_FLOW_BOX(self->album_list), FALSE);
	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(self->album_list), GTK_SELECTION_NONE);
	gtk_widget_add_css_class(self->album_list, "album-list");

	gtk_box_prepend(GTK_BOX(self->content), self->favorites_list); // Add the strip
	gtk_box_append(GTK_BOX(self->content), self->album_list); // Add the list

	gtk_widget_set_hexpand(GTK_WIDGET(self->favorites_list), TRUE);
	gtk_widget_set_hexpand(GTK_WIDGET(self->album_list), TRUE);

	G_OBJECT_CLASS (koto_artist_view_parent_class)->constructed (obj);
	self->constructed = TRUE;
}

void koto_artist_view_add_album(KotoArtistView *self, KotoIndexedAlbum *album) {
	gchar *album_art = koto_indexed_album_get_album_art(album); // Get the art for the album

	GtkWidget *art_image = koto_utils_create_image_from_filepath(album_art, "audio-x-generic-symbolic", 220, 220);
	gtk_widget_set_halign(art_image, GTK_ALIGN_START); // Align to start
	gtk_flow_box_insert(GTK_FLOW_BOX(self->favorites_list), art_image, -1); // Append the album art

	KotoAlbumView* album_view = koto_album_view_new(album); // Create our new album view
	GtkWidget* album_view_main = koto_album_view_get_main(album_view);
	gtk_flow_box_insert(GTK_FLOW_BOX(self->album_list), album_view_main, -1); // Append the album view to the album list
}

void koto_artist_view_add_artist(KotoArtistView *self, KotoIndexedArtist *artist) {
	if (artist == NULL) {
		return;
	}

	self->artist = artist;

	if (!self->constructed) {
		return;
	}

	GList *albums = koto_indexed_artist_get_albums(self->artist); // Get the albums

	GList *a;
	for (a = albums; a != NULL; a = a->next) {
		KotoIndexedAlbum *album = (KotoIndexedAlbum*) a->data;
		koto_artist_view_add_album(self, album); // Add the album
	}
}

GtkWidget* koto_artist_view_get_main(KotoArtistView *self) {
	return self->scrolled_window;
}

KotoArtistView* koto_artist_view_new() {
	return g_object_new(KOTO_TYPE_ARTIST_VIEW, NULL);
}
