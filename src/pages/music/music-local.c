/* music-local.c
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
#include "koto-button.h"
#include "koto-config.h"
#include "music-local.h"

extern KotoCartographer *koto_maps;

enum {
	PROP_0,
	PROP_LIB,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

struct _KotoPageMusicLocal {
	GtkBox parent_instance;
	GtkWidget *scrolled_window;
	GtkWidget *artist_list;
	GtkWidget *stack;

	KotoIndexedLibrary *lib;
	gboolean constructed;
};

struct _KotoPageMusicLocalClass {
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(KotoPageMusicLocal, koto_page_music_local, GTK_TYPE_BOX);

KotoPageMusicLocal *music_local_page;

static void koto_page_music_local_constructed(GObject *obj);
static void koto_page_music_local_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_page_music_local_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_page_music_local_class_init(KotoPageMusicLocalClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->constructed = koto_page_music_local_constructed;
	gobject_class->set_property = koto_page_music_local_set_property;
	gobject_class->get_property = koto_page_music_local_get_property;

	props[PROP_LIB] = g_param_spec_object(
		"lib",
		"Library",
		"Library",
		KOTO_TYPE_INDEXED_LIBRARY,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_page_music_local_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoPageMusicLocal *self = KOTO_PAGE_MUSIC_LOCAL(obj);

	switch (prop_id) {
		case PROP_LIB:
			g_value_set_object(val, self->lib);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_page_music_local_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoPageMusicLocal *self = KOTO_PAGE_MUSIC_LOCAL(obj);

	switch (prop_id) {
		case PROP_LIB:
			koto_page_music_local_set_library(self, (KotoIndexedLibrary*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_page_music_local_init(KotoPageMusicLocal *self) {
	self->lib = NULL;
	self->constructed = FALSE;

	gtk_widget_add_css_class(GTK_WIDGET(self), "page-music-local");
	gtk_widget_set_hexpand(GTK_WIDGET(self), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(self), TRUE);

	self->scrolled_window = gtk_scrolled_window_new();
	gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(self->scrolled_window), 300);
	gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(self->scrolled_window), FALSE);
	gtk_widget_add_css_class(self->scrolled_window, "artist-list");
	gtk_widget_set_vexpand(self->scrolled_window, TRUE); // Expand our scrolled window
	gtk_box_prepend(GTK_BOX(self), self->scrolled_window);

	self->artist_list = gtk_list_box_new(); // Create our artist list
	g_signal_connect(GTK_LIST_BOX(self->artist_list), "row-activated", G_CALLBACK(koto_page_music_local_handle_artist_click), self);
	gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(self->artist_list), TRUE);
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->artist_list), GTK_SELECTION_BROWSE);
	gtk_list_box_set_sort_func(GTK_LIST_BOX(self->artist_list), koto_page_music_local_sort_artists, NULL, NULL); // Add our sort function
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->scrolled_window), self->artist_list);

	self->stack = gtk_stack_new(); // Create a new stack
	gtk_stack_set_transition_duration(GTK_STACK(self->stack), 400);
	gtk_stack_set_transition_type(GTK_STACK(self->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT);
	gtk_widget_set_hexpand(self->stack, TRUE);
	gtk_widget_set_vexpand(self->stack, TRUE);
	gtk_box_append(GTK_BOX(self), self->stack);
}

static void koto_page_music_local_constructed(GObject *obj) {
	KotoPageMusicLocal *self = KOTO_PAGE_MUSIC_LOCAL(obj);

	G_OBJECT_CLASS (koto_page_music_local_parent_class)->constructed (obj);
	self->constructed = TRUE;
}

void koto_page_music_local_add_artist(KotoPageMusicLocal *self, KotoIndexedArtist *artist) {
	gchar *artist_name;
	g_object_get(artist, "name", &artist_name, NULL);
	KotoButton *artist_button = koto_button_new_plain(artist_name);
	gtk_list_box_prepend(GTK_LIST_BOX(self->artist_list), GTK_WIDGET(artist_button));

	KotoArtistView *artist_view = koto_artist_view_new(); // Create our new artist view
	koto_artist_view_add_artist(artist_view, artist); // Add the artist
	gtk_stack_add_named(GTK_STACK(self->stack), koto_artist_view_get_main(artist_view), artist_name);
}

void koto_page_music_local_go_to_artist_by_name(KotoPageMusicLocal *self, gchar *artist_name) {
	gtk_stack_set_visible_child_name(GTK_STACK(self->stack), artist_name);
}

void koto_page_music_local_go_to_artist_by_uuid(KotoPageMusicLocal *self, gchar *artist_uuid) {
	KotoIndexedArtist *artist = koto_cartographer_get_artist_by_uuid(koto_maps, artist_uuid); // Get the artist

	if (!KOTO_IS_INDEXED_ARTIST(artist)) { // No artist for this UUID
		return;
	}

	gchar *artist_name = NULL;
	g_object_get(
		artist,
		"name",
		&artist_name,
		NULL
	);

	if (artist_name == NULL || (g_strcmp0(artist_name, "") == 0)) { // Failed to get the artist name
		return;
	}

	koto_page_music_local_go_to_artist_by_name(self, artist_name);
}

void koto_page_music_local_handle_artist_click(GtkListBox *box, GtkListBoxRow *row, gpointer data) {
	(void) box;
	KotoPageMusicLocal *self = (KotoPageMusicLocal*) data;
	KotoButton *btn = KOTO_BUTTON(gtk_list_box_row_get_child(row));

	gchar *artist_name;
	g_object_get(btn, "button-text", &artist_name, NULL);
	koto_page_music_local_go_to_artist_by_name(self, artist_name);
}

void koto_page_music_local_set_library(KotoPageMusicLocal *self, KotoIndexedLibrary *lib) {
	if (lib == NULL) {
		return;
	}

	if (self->lib != NULL) { // If lib is already set
		g_free(self->lib);
	}

	self->lib = lib;

	if (!self->constructed) {
		return;
	}

	GHashTableIter artist_list_iter;
	gpointer artist_key;
	gpointer artist_data;

	GHashTable *artists = koto_indexed_library_get_artists(self->lib); // Get the artists

	g_hash_table_iter_init(&artist_list_iter, artists);
	while (g_hash_table_iter_next(&artist_list_iter, &artist_key, &artist_data)) { // For each of the music artists
		KotoIndexedArtist *artist = koto_cartographer_get_artist_by_uuid(koto_maps, (gchar*) artist_data); // Cast our data as a KotoIndexedArtist

		if (artist != NULL) {
			koto_page_music_local_add_artist(self, artist);
		}
	}
}

int koto_page_music_local_sort_artists(GtkListBoxRow *artist1, GtkListBoxRow *artist2, gpointer user_data) {
	(void) user_data;
	KotoButton *artist1_btn = KOTO_BUTTON(gtk_list_box_row_get_child(artist1));
	KotoButton *artist2_btn = KOTO_BUTTON(gtk_list_box_row_get_child(artist2));

	gchar *artist1_text;
	gchar *artist2_text;

	g_object_get(artist1_btn, "button-text", &artist1_text, NULL);
	g_object_get(artist2_btn, "button-text", &artist2_text, NULL);

	return g_utf8_collate(artist1_text, artist2_text);
}

KotoPageMusicLocal* koto_page_music_local_new() {
	return g_object_new(KOTO_TYPE_PAGE_MUSIC_LOCAL,
		"orientation", GTK_ORIENTATION_HORIZONTAL,
		NULL
	);
}
