/* album-view.c
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
#include "../../components/cover-art-button.h"
#include "../../db/cartographer.h"
#include "../../indexer/structs.h"
#include "album-view.h"
#include "disc-view.h"
#include "config/config.h"
#include "koto-utils.h"

extern KotoCartographer * koto_maps;

struct _KotoAlbumView {
	GObject parent_instance;
	KotoAlbum * album;
	GtkWidget * main;
	GtkWidget * album_tracks_box;
	GtkWidget * discs;

	KotoCoverArtButton * album_cover;

	KotoAlbumInfo * album_info;
	GHashTable * cd_to_disc_views;
};

G_DEFINE_TYPE(KotoAlbumView, koto_album_view, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_ALBUM,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};
static void koto_album_view_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_album_view_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_album_view_class_init(KotoAlbumViewClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_album_view_set_property;
	gobject_class->get_property = koto_album_view_get_property;

	props[PROP_ALBUM] = g_param_spec_object(
		"album",
		"Album",
		"Album",
		KOTO_TYPE_ALBUM,
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_album_view_init(KotoAlbumView * self) {
	self->cd_to_disc_views = g_hash_table_new(g_str_hash, g_str_equal);
	self->main = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_add_css_class(self->main, "album-view");
	gtk_widget_set_can_focus(self->main, FALSE);
	self->album_tracks_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	self->album_info = koto_album_info_new(KOTO_ALBUM_INFO_TYPE_ALBUM); // Create an Album-type Album Info
	gtk_box_prepend(GTK_BOX(self->album_tracks_box), GTK_WIDGET(self->album_info)); // Add our Album Info to the album_tracks box

	self->discs = gtk_list_box_new(); // Create our list of our tracks
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->discs), GTK_SELECTION_NONE);
	gtk_list_box_set_show_separators(GTK_LIST_BOX(self->discs), FALSE);
	gtk_list_box_set_sort_func(GTK_LIST_BOX(self->discs), koto_album_view_sort_discs, NULL, NULL); // Ensure we can sort our discs
	gtk_widget_add_css_class(self->discs, "discs-list");
	gtk_widget_set_can_focus(self->discs, FALSE);
	gtk_widget_set_focusable(self->discs, FALSE);
	gtk_widget_set_size_request(self->discs, 600, -1);

	gtk_box_append(GTK_BOX(self->main), self->album_tracks_box); // Add the tracks box to the art info combo box
	gtk_box_append(GTK_BOX(self->album_tracks_box), self->discs); // Add the discs list box to the albums tracks box

	self->album_cover = koto_cover_art_button_new(220, 220, NULL);
	GtkWidget * album_cover_main = koto_cover_art_button_get_main(self->album_cover);

	gtk_widget_set_valign(album_cover_main, GTK_ALIGN_START);

	gtk_box_prepend(GTK_BOX(self->main), album_cover_main);
	KotoButton * cover_art_button = koto_cover_art_button_get_button(self->album_cover); // Get the button for the cover art
	koto_button_add_click_handler(cover_art_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_album_view_toggle_album_playback), self);
}

GtkWidget * koto_album_view_get_main(KotoAlbumView * self) {
	return self->main;
}

static void koto_album_view_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoAlbumView * self = KOTO_ALBUM_VIEW(obj);

	switch (prop_id) {
		case PROP_ALBUM:
			g_value_set_object(val, self->album);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_album_view_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoAlbumView * self = KOTO_ALBUM_VIEW(obj);

	switch (prop_id) {
		case PROP_ALBUM:
			koto_album_view_set_album(self, (KotoAlbum*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_album_view_add_track(
	KotoAlbumView * self,
	KotoTrack * track
) {
	if (!KOTO_IS_ALBUM_VIEW(self)) { // Not an album view
		return;
	}

	if (!KOTO_IS_ALBUM(self->album)) { // Somehow don't have an album set
		return;
	}

	if (!KOTO_IS_TRACK(track)) { // Track doesn't exist
		return;
	}

	guint disc_number = koto_track_get_disc_number(track); // Get the disc number
	gchar * disc_num_as_str = g_strdup_printf("%u", disc_number);

	KotoDiscView * disc_view;

	if (g_hash_table_contains(self->cd_to_disc_views, disc_num_as_str)) { // Already have this added this disc and its disc view
		disc_view = g_hash_table_lookup(self->cd_to_disc_views, disc_num_as_str); // Get the disc view
	} else {
		disc_view = koto_disc_view_new(self->album, disc_number); // Build a new disc view
		gtk_list_box_append(GTK_LIST_BOX(self->discs), GTK_WIDGET(disc_view)); // Add the Disc View to the List Box
		g_hash_table_replace(self->cd_to_disc_views, disc_num_as_str, disc_view); // Add the new Disc View to our hash table
	}

	if (!KOTO_IS_DISC_VIEW(disc_view)) { // If this is not a Disc View
		return;
	}

	koto_disc_view_add_track(disc_view, track); // Add the track to the disc view
	koto_album_view_update_disc_labels(self); // Update our disc labels
}

void koto_album_view_handle_track_added(
	KotoAlbum * album,
	KotoTrack * track,
	gpointer user_data
) {
	if (!KOTO_IS_ALBUM(album)) { // If not an album
		return;
	}

	if (!KOTO_IS_TRACK(track)) { // If not a track
		return;
	}

	KotoAlbumView * self = KOTO_ALBUM_VIEW(user_data); // Define as an album view

	if (!KOTO_IS_ALBUM_VIEW(self)) {
		return;
	}

	koto_album_view_add_track(self, track); // Add the track
}

void koto_album_view_set_album(
	KotoAlbumView * self,
	KotoAlbum * album
) {
	if (!KOTO_IS_ALBUM_VIEW(self)) {
		return;
	}

	if (!KOTO_IS_ALBUM(album)) {
		return;
	}

	self->album = album;

	gchar * album_art = koto_album_get_art(self->album); // Get the art for the album
	koto_cover_art_button_set_art_path(self->album_cover, album_art);
	koto_album_info_set_album_uuid(self->album_info, koto_album_get_uuid(album)); // Set the album we should be displaying info about on the Album Info
	g_signal_connect(self->album, "track-added", G_CALLBACK(koto_album_view_handle_track_added), self); // Handle track added on our album
}

int koto_album_view_sort_discs(
	GtkListBoxRow * disc1,
	GtkListBoxRow * disc2,
	gpointer user_data
) {
	(void) user_data;
	KotoDiscView * disc1_item = KOTO_DISC_VIEW(gtk_list_box_row_get_child(disc1));
	KotoDiscView * disc2_item = KOTO_DISC_VIEW(gtk_list_box_row_get_child(disc2));

	guint disc1_num;
	guint disc2_num;

	g_object_get(disc1_item, "disc", &disc1_num, NULL);
	g_object_get(disc2_item, "disc", &disc2_num, NULL);

	if (disc1_num == disc2_num) { // Identical positions (like reported as 0)
		return 0;
	} else if (disc1_num < disc2_num) {
		return -1;
	} else {
		return 1;
	}
}

void koto_album_view_toggle_album_playback(
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
	KotoAlbumView * self = data;

	koto_album_set_as_current_playlist(self->album); // Set as the current playlist
}

void koto_album_view_update_disc_labels(KotoAlbumView * self) {
	gboolean show_disc_labels = g_hash_table_size(self->cd_to_disc_views) > 1;
	gpointer disc_num_ptr;
	gpointer disc_view_ptr;

	GHashTableIter disc_view_iter;
	g_hash_table_iter_init(&disc_view_iter, self->cd_to_disc_views);

	while (g_hash_table_iter_next(&disc_view_iter, &disc_num_ptr, &disc_view_ptr)) {
		(void) disc_num_ptr;
		KotoDiscView * view = (KotoDiscView*) disc_view_ptr;
		if (KOTO_IS_DISC_VIEW(view)) {
			koto_disc_view_set_disc_label_visible(view, show_disc_labels);
		}
	}
}

KotoAlbumView * koto_album_view_new(KotoAlbum * album) {
	return g_object_new(KOTO_TYPE_ALBUM_VIEW, "album", album, NULL);
}
