/* writers-page.c
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
#include "../../koto-utils.h"
#include "../../koto-window.h"
#include "audiobook-view.h"
#include "writer-page.h"

extern KotoCartographer * koto_maps;
extern KotoWindow * main_window;

enum {
	PROP_0,
	PROP_ARTIST,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};

struct _KotoWriterPage {
	GObject parent_instance;

	KotoArtist * artist;

	GtkWidget * main; // Our main content will be a scrolled Scrolled Window
	GtkWidget * content; // Content inside scrolled window

	GtkWidget * writers_header; // Header GtkLabel for the writer
	GListModel * model;

	GtkWidget * audiobooks_flow;
};

struct _KotoWriterPageClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE(KotoWriterPage, koto_writer_page, G_TYPE_OBJECT);

static void koto_writer_page_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_writer_page_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_writer_page_class_init(KotoWriterPageClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_writer_page_set_property;
	gobject_class->get_property = koto_writer_page_get_property;

	props[PROP_ARTIST] = g_param_spec_object(
		"artist",
		"Artist",
		"Artist",
		KOTO_TYPE_ARTIST,
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_writer_page_init(KotoWriterPage * self) {
	self->main = gtk_scrolled_window_new(); // Set main to be a scrolled window
	self->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_css_class(self->content, "writer-page");

	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->main), self->content); // Set content to be the child of the scrolled window

	self->writers_header = gtk_label_new(NULL); // Create an empty label
	gtk_widget_add_css_class(self->writers_header, "writer-header");
	gtk_widget_set_halign(self->writers_header, GTK_ALIGN_START);

	self->audiobooks_flow = gtk_flow_box_new(); // Create our flowbox of the audiobooks views
	gtk_widget_add_css_class(self->audiobooks_flow, "audiobooks-flow");

	gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(self->audiobooks_flow), 2); // Allow 2 to ensure adequate spacing for description
	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(self->audiobooks_flow), GTK_SELECTION_NONE); // Do not allow selection

	gtk_widget_set_hexpand(self->audiobooks_flow, TRUE); // Expand horizontally
	gtk_widget_set_vexpand(self->audiobooks_flow, TRUE); // Expand vertically

	gtk_box_append(GTK_BOX(self->content), self->writers_header);
	gtk_box_append(GTK_BOX(self->content), self->audiobooks_flow); // Add the audiobooks flow to the content
}

static void koto_writer_page_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoWriterPage * self = KOTO_WRITER_PAGE(obj);

	switch (prop_id) {
		case PROP_ARTIST:
			g_value_set_object(val, self->artist);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_writer_page_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoWriterPage * self = KOTO_WRITER_PAGE(obj);

	switch (prop_id) {
		case PROP_ARTIST:
			koto_writer_page_set_artist(self, (KotoArtist*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

GtkWidget * koto_writer_page_create_item(
	gpointer item,
	gpointer user_data
) {
	(void) user_data;

	KotoAlbum * album = KOTO_ALBUM(item); // Cast the model data as an album

	if (!KOTO_IS_ALBUM(album)) { // Fetched item from list is not album
		return NULL;
	}

	KotoAudiobookView * view = koto_audiobook_view_new(); // Create our KotoAudiobookView
	koto_audiobook_view_set_album(view, album); // Set the item for this set up audiobook view
	return GTK_WIDGET(view);
}

GtkWidget * koto_writer_page_get_main(KotoWriterPage * self) {
	return self->main;
}

void koto_writer_page_set_artist(
	KotoWriterPage * self,
	KotoArtist * artist
) {
	if (!KOTO_IS_WRITER_PAGE(self)) { // Not a writers page
		return;
	}

	if (!KOTO_IS_ARTIST(artist)) { // Not an artist
		return;
	}

	self->artist = artist;
	gtk_label_set_text(GTK_LABEL(self->writers_header), koto_artist_get_name(self->artist)); // Get the label for the writers header

	self->model = G_LIST_MODEL(koto_artist_get_albums_store(artist)); // Get the store and cast it as a list model for our model

	gtk_flow_box_bind_model(
		GTK_FLOW_BOX(self->audiobooks_flow),
		self->model,
		koto_writer_page_create_item,
		NULL,
		NULL
	);
}

KotoWriterPage * koto_writer_page_new(KotoArtist * artist) {
	return g_object_new(
		KOTO_TYPE_WRITER_PAGE,
		"artist",
		artist,
		NULL
	);
}