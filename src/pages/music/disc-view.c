/* disc-view.c
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

#include <gtk-4.0/gtk/gtk.h>
#include "../../db/cartographer.h"
#include "../../indexer/structs.h"
#include "../../koto-track-item.h"
#include "disc-view.h"

extern KotoCartographer *koto_maps;

struct _KotoDiscView {
	GtkBox parent_instance;
	KotoIndexedAlbum *album;
	GtkWidget *header;
	GtkWidget *label;
	GtkWidget *list;

	guint *disc_number;
};

G_DEFINE_TYPE(KotoDiscView, koto_disc_view, GTK_TYPE_BOX);

enum {
	PROP_0,
	PROP_DISC,
	PROP_ALBUM,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };
static void koto_disc_view_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_disc_view_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_disc_view_class_init(KotoDiscViewClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_disc_view_set_property;
	gobject_class->get_property = koto_disc_view_get_property;

	props[PROP_DISC] = g_param_spec_uint(
		"disc",
		"Disc",
		"Disc",
		0,
		G_MAXUINT16,
		1,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ALBUM] = g_param_spec_object(
		"album",
		"Album",
		"Album",
		KOTO_TYPE_INDEXED_ALBUM,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_disc_view_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoDiscView *self = KOTO_DISC_VIEW(obj);

	switch (prop_id) {
		case PROP_DISC:
			g_value_set_uint(val, GPOINTER_TO_UINT(self->disc_number));
			break;
		case PROP_ALBUM:
			g_value_set_object(val, self->album);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_disc_view_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoDiscView *self = KOTO_DISC_VIEW(obj);

	switch (prop_id) {
		case PROP_DISC:
			koto_disc_view_set_disc_number(self, g_value_get_uint(val));
			break;
		case PROP_ALBUM:
			koto_disc_view_set_album(self, (KotoIndexedAlbum*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_disc_view_init(KotoDiscView *self) {
	gtk_widget_add_css_class(GTK_WIDGET(self), "disc-view");
	gtk_widget_set_can_focus(GTK_WIDGET(self), FALSE);
	self->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand(self->header, TRUE);
	gtk_widget_set_size_request(self->header, 16, -1);

	GtkWidget *ico = gtk_image_new_from_icon_name("drive-optical-symbolic");
	gtk_box_prepend(GTK_BOX(self->header), ico);

	self->label = gtk_label_new(NULL); // Create an empty label
	gtk_widget_set_halign(self->label, GTK_ALIGN_START);
	gtk_widget_set_valign(self->label, GTK_ALIGN_CENTER);
	gtk_box_append(GTK_BOX(self->header), self->label);

	gtk_box_append(GTK_BOX(self), self->header);
}

void koto_disc_view_set_album(KotoDiscView *self, KotoIndexedAlbum *album) {
	if (album == NULL) {
		return;
	}

	if (self->album != NULL) {
		g_free(self->album);
	}

	self->album = album;

	if (GTK_IS_LIST_BOX(self->list)) { // Already have a listbox
		gtk_box_remove(GTK_BOX(self), self->list); // Remove the box
		g_object_unref(self->list); // Unref the list
	}

	self->list = gtk_list_box_new(); // Create our list of our tracks
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->list), GTK_SELECTION_MULTIPLE);
	gtk_widget_add_css_class(self->list, "track-list");
	gtk_widget_set_size_request(self->list, 600, -1);
	gtk_box_append(GTK_BOX(self), self->list);

	g_list_foreach(koto_indexed_album_get_tracks(self->album), koto_disc_view_list_tracks, self);
}

void koto_disc_view_list_tracks(gpointer data, gpointer selfptr) {
	KotoDiscView *self = (KotoDiscView*) selfptr;
	KotoIndexedTrack *track = koto_cartographer_get_track_by_uuid(koto_maps, (gchar*) data); // Get the track by its UUID

	guint *disc_number;
	g_object_get(track, "cd", &disc_number, NULL); // get the disc number

	if (GPOINTER_TO_UINT(self->disc_number) != GPOINTER_TO_UINT(disc_number)) { // Track does not belong to this CD
		return;
	}

	KotoTrackItem *track_item = koto_track_item_new(track); // Create our new track item
	gtk_list_box_append(GTK_LIST_BOX(self->list), GTK_WIDGET(track_item)); // Add to our tracks list box
}

void koto_disc_view_set_disc_number(KotoDiscView *self, guint disc_number) {
	if (disc_number == 0) {
		return;
	}

	self->disc_number = GUINT_TO_POINTER(disc_number);

	gchar *disc_label = g_strdup_printf("Disc %u", disc_number);
	gtk_label_set_text(GTK_LABEL(self->label), disc_label);  // Set the label

	g_free(disc_label);
}

void koto_disc_view_set_disc_label_visible(KotoDiscView *self, gboolean visible) {
	(visible) ? gtk_widget_show(self->header) : gtk_widget_hide(self->header);
}

KotoDiscView* koto_disc_view_new(KotoIndexedAlbum *album, guint *disc_number) {
	return g_object_new(KOTO_TYPE_DISC_VIEW,
		"disc", disc_number,
		"album", album,
		"orientation", GTK_ORIENTATION_VERTICAL,
		NULL
	);
}
