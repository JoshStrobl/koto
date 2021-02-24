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
#include "../../koto-track-item.h"
#include "album-view.h"
#include "koto-config.h"
#include "koto-utils.h"

struct _KotoAlbumView {
	GObject parent_instance;
	KotoIndexedAlbum *album;
	GtkWidget *main;
	GtkWidget *album_tracks_box;
	GtkWidget *tracks;

	GtkWidget *album_label;
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
	self->main = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_can_focus(self->main, FALSE);
	self->album_tracks_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->tracks = gtk_list_box_new(); // Create our list of our tracks
	gtk_list_box_set_sort_func(GTK_LIST_BOX(self->tracks), koto_album_view_sort_tracks, NULL, NULL); // Ensure we can sort our tracks
	gtk_widget_set_size_request(self->tracks, 600, -1);

	gtk_box_append(GTK_BOX(self->main), self->album_tracks_box); // Add the tracks box to the art info combo box
	gtk_box_append(GTK_BOX(self->album_tracks_box), self->tracks); // Add the tracks list box to the albums tracks box
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

void koto_album_view_set_album(KotoAlbumView *self, KotoIndexedAlbum *album) {
	if (album == NULL) {
		return;
	}

	self->album = album;

	gchar *album_art = koto_indexed_album_get_album_art(self->album); // Get the art for the album
	GtkWidget *art_image = koto_utils_create_image_from_filepath(album_art, "audio-x-generic-symbolic", 220, 220);

	gtk_box_prepend(GTK_BOX(self->main), art_image); // Prepend the image to the art info box

	gchar *album_name;
	g_object_get(album, "name", &album_name, NULL); // Get the album name

	self->album_label = gtk_label_new(album_name);
	gtk_box_prepend(GTK_BOX(self->album_tracks_box), self->album_label); // Prepend our new label to the album + tracks box

	GList *t;
	for (t = koto_indexed_album_get_files(self->album); t != NULL; t = t->next) { // For each file / track
		KotoIndexedFile *file = (KotoIndexedFile*) t->data;
		KotoTrackItem *track_item = koto_track_item_new(file); // Create our new track item
		gtk_list_box_prepend(GTK_LIST_BOX(self->tracks), GTK_WIDGET(track_item)); // Add to our tracks list box
	}
}

int koto_album_view_sort_tracks(GtkListBoxRow *track1, GtkListBoxRow *track2, gpointer user_data) {
	(void) user_data;
	KotoTrackItem *track1_item = KOTO_TRACK_ITEM(gtk_list_box_row_get_child(track1));
	KotoTrackItem *track2_item = KOTO_TRACK_ITEM(gtk_list_box_row_get_child(track2));

	KotoIndexedFile *track1_file;
	KotoIndexedFile *track2_file;

	g_object_get(track1_item, "track", &track1_file, NULL);
	g_object_get(track2_item, "track", &track2_file, NULL);

	guint16 *track1_pos;
	guint16 *track2_pos;

	g_object_get(track1_file, "position", &track1_pos, NULL);
	g_object_get(track2_file, "position", &track2_pos, NULL);

	if (track1_pos == track2_pos) { // Identical positions (like reported as 0)
		gchar *track1_name;
		gchar *track2_name;

		g_object_get(track1_file, "parsed-name", &track1_name, NULL);
		g_object_get(track2_file, "parsed-name", &track2_name, NULL);

		return g_utf8_collate(track1_name, track2_name);
	} else if (track1_pos < track2_pos) {
		return -1;
	} else {
		return 1;
	}
}

KotoAlbumView* koto_album_view_new(KotoIndexedAlbum *album) {
	return g_object_new(KOTO_TYPE_ALBUM_VIEW, "album", album, NULL);
}
