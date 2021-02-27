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
#include "../../indexer/album.h"
#include "../../indexer/artist.h"
#include "album-view.h"
#include "disc-view.h"
#include "koto-config.h"
#include "koto-utils.h"

struct _KotoAlbumView {
	GObject parent_instance;
	KotoIndexedAlbum *album;
	GtkWidget *main;
	GtkWidget *album_tracks_box;
	GtkWidget *discs;

	GtkWidget *album_label;
	GHashTable *cd_to_track_listbox;
};

G_DEFINE_TYPE(KotoAlbumView, koto_album_view, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_ALBUM,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };
static void koto_album_view_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_album_view_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_album_view_class_init(KotoAlbumViewClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_album_view_set_property;
	gobject_class->get_property = koto_album_view_get_property;

	props[PROP_ALBUM] = g_param_spec_object(
		"album",
		"Album",
		"Album",
		KOTO_TYPE_INDEXED_ALBUM,
		G_PARAM_CONSTRUCT_ONLY|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_album_view_init(KotoAlbumView *self) {
	self->cd_to_track_listbox = g_hash_table_new(g_str_hash, g_str_equal);
	self->main = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_add_css_class(self->main, "album-view");
	gtk_widget_set_can_focus(self->main, FALSE);
	self->album_tracks_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->discs = gtk_list_box_new(); // Create our list of our tracks
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->discs), GTK_SELECTION_NONE);
	gtk_list_box_set_sort_func(GTK_LIST_BOX(self->discs), koto_album_view_sort_discs, NULL, NULL); // Ensure we can sort our discs
	gtk_widget_add_css_class(self->discs, "discs-list");
	gtk_widget_set_can_focus(self->discs, FALSE);
	gtk_widget_set_size_request(self->discs, 600, -1);

	gtk_box_append(GTK_BOX(self->main), self->album_tracks_box); // Add the tracks box to the art info combo box
	gtk_box_append(GTK_BOX(self->album_tracks_box), self->discs); // Add the discs list box to the albums tracks box
}

GtkWidget* koto_album_view_get_main(KotoAlbumView *self) {
	return self->main;
}

static void koto_album_view_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoAlbumView *self = KOTO_ALBUM_VIEW(obj);

	switch (prop_id) {
		case PROP_ALBUM:
			g_value_set_object(val, self->album);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_album_view_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoAlbumView *self = KOTO_ALBUM_VIEW(obj);

	switch (prop_id) {
		case PROP_ALBUM:
			koto_album_view_set_album(self, (KotoIndexedAlbum*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_album_view_add_track_to_listbox(KotoIndexedAlbum *self, KotoIndexedFile *file) {
	(void) self; (void) file;
}

void koto_album_view_set_album(KotoAlbumView *self, KotoIndexedAlbum *album) {
	if (album == NULL) {
		return;
	}

	self->album = album;

	gchar *album_art = koto_indexed_album_get_album_art(self->album); // Get the art for the album
	GtkWidget *art_image = koto_utils_create_image_from_filepath(album_art, "audio-x-generic-symbolic", 220, 220);
	gtk_widget_set_valign(art_image, GTK_ALIGN_START); // Align to top of list for album

	gtk_box_prepend(GTK_BOX(self->main), art_image); // Prepend the image to the art info box

	gchar *album_name;
	g_object_get(album, "name", &album_name, NULL); // Get the album name

	self->album_label = gtk_label_new(album_name);
	gtk_widget_set_halign(self->album_label, GTK_ALIGN_START);
	gtk_box_prepend(GTK_BOX(self->album_tracks_box), self->album_label); // Prepend our new label to the album + tracks box

	GHashTable *discs = g_hash_table_new(g_str_hash, g_str_equal);
	GList *tracks = koto_indexed_album_get_files(album); // Get the tracks for this album

	for (guint i = 0; i < g_list_length(tracks); i++) {
		KotoIndexedFile *file = (KotoIndexedFile*) g_list_nth_data(tracks, i); // Get the
		guint *disc_number;
		g_object_get(file, "cd", &disc_number, NULL);
		gchar *disc_num_as_str = g_strdup_printf("%u", GPOINTER_TO_UINT(disc_number));

		if (g_hash_table_contains(discs, disc_num_as_str)) { // Already have this added
			continue; // Skip
		}

		g_hash_table_insert(discs, disc_num_as_str, "0"); // Mark this disc number in the hash table
		KotoDiscView *disc_view = koto_disc_view_new(album, disc_number);
		gtk_list_box_append(GTK_LIST_BOX(self->discs), GTK_WIDGET(disc_view)); // Add the Disc View to the List Box
	}

	if (g_list_length(g_hash_table_get_keys(discs)) == 1) { // Only have one album
		GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->discs), 0); // Get the first item

		if (GTK_IS_LIST_BOX_ROW(row)) { // Is a row
			koto_disc_view_set_disc_label_visible((KotoDiscView*) gtk_list_box_row_get_child(row), FALSE); // Get
		}
	}

	g_hash_table_destroy(discs);
}

int koto_album_view_sort_discs(GtkListBoxRow *disc1, GtkListBoxRow *disc2, gpointer user_data) {
	(void) user_data;
	KotoDiscView *disc1_item = KOTO_DISC_VIEW(gtk_list_box_row_get_child(disc1));
	KotoDiscView *disc2_item = KOTO_DISC_VIEW(gtk_list_box_row_get_child(disc2));

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

KotoAlbumView* koto_album_view_new(KotoIndexedAlbum *album) {
	return g_object_new(KOTO_TYPE_ALBUM_VIEW, "album", album, NULL);
}
