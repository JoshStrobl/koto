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
#include "config/config.h"
#include "../../koto-utils.h"
#include "music-local.h"

extern KotoCartographer * koto_maps;

struct _KotoPageMusicLocal {
	GtkBox parent_instance;
	GtkWidget * scrolled_window;
	GtkWidget * artist_list;
	GtkWidget * stack;
	GHashTable * artist_name_to_buttons;

	gboolean constructed;
};

struct _KotoPageMusicLocalClass {
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(KotoPageMusicLocal, koto_page_music_local, GTK_TYPE_BOX);

KotoPageMusicLocal * music_local_page;

static void koto_page_music_local_constructed(GObject * obj);

static void koto_page_music_local_class_init(KotoPageMusicLocalClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->constructed = koto_page_music_local_constructed;
}

static void koto_page_music_local_init(KotoPageMusicLocal * self) {
	self->constructed = FALSE;
	self->artist_name_to_buttons = g_hash_table_new(g_str_hash, g_str_equal);

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

	g_signal_connect(koto_maps, "artist-added", G_CALLBACK(koto_page_music_local_handle_artist_added), self);
	g_signal_connect(koto_maps, "artist-removed", G_CALLBACK(koto_page_music_local_handle_artist_removed), self);
}

static void koto_page_music_local_constructed(GObject * obj) {
	KotoPageMusicLocal * self = KOTO_PAGE_MUSIC_LOCAL(obj);

	G_OBJECT_CLASS(koto_page_music_local_parent_class)->constructed(obj);
	self->constructed = TRUE;
}

void koto_page_music_local_add_artist(
	KotoPageMusicLocal * self,
	KotoArtist * artist
) {
	if (!KOTO_IS_PAGE_MUSIC_LOCAL(self)) { // Not the local page
		return;
	}

	if (!KOTO_IS_ARTIST(artist)) { // Not an artist
		return;
	}

	gchar * artist_name = koto_artist_get_name(artist); // Get the artist name

	if (!GTK_IS_STACK(self->stack)) {
		return;
	}

	GtkWidget * stack_child = gtk_stack_get_child_by_name(GTK_STACK(self->stack), artist_name);

	if (GTK_IS_WIDGET(stack_child)) { // Already have a page for this artist
		g_free(artist_name);
		return;
	}

	KotoButton * artist_button = koto_button_new_plain(artist_name);
	gtk_list_box_prepend(GTK_LIST_BOX(self->artist_list), GTK_WIDGET(artist_button));
	g_hash_table_replace(self->artist_name_to_buttons, artist_name, artist_button); // Add the KotoButton for this artist to the hash table, that way we can reference and remove it when we remove the artist

	KotoArtistView * artist_view = koto_artist_view_new(artist); // Create our new artist view

	gtk_stack_add_named(GTK_STACK(self->stack), koto_artist_view_get_main(artist_view), artist_name);
	g_free(artist_name);
}

void koto_page_music_local_go_to_artist_by_name(
	KotoPageMusicLocal * self,
	gchar * artist_name
) {
	gtk_stack_set_visible_child_name(GTK_STACK(self->stack), artist_name);
}

void koto_page_music_local_go_to_artist_by_uuid(
	KotoPageMusicLocal * self,
	gchar * artist_uuid
) {
	KotoArtist * artist = koto_cartographer_get_artist_by_uuid(koto_maps, artist_uuid); // Get the artist

	if (!KOTO_IS_ARTIST(artist)) { // No artist for this UUID
		return;
	}

	gchar * artist_name = NULL;

	g_object_get(
		artist,
		"name",
		&artist_name,
		NULL
	);

	if (!koto_utils_is_string_valid(artist_name)) { // Failed to get the artist name
		return;
	}

	koto_page_music_local_go_to_artist_by_name(self, artist_name);
}

void koto_page_music_local_handle_artist_click(
	GtkListBox * box,
	GtkListBoxRow * row,
	gpointer data
) {
	(void) box;
	KotoPageMusicLocal * self = (KotoPageMusicLocal*) data;
	KotoButton * btn = KOTO_BUTTON(gtk_list_box_row_get_child(row));

	gchar * artist_name;

	g_object_get(btn, "button-text", &artist_name, NULL);
	koto_page_music_local_go_to_artist_by_name(self, artist_name);
}

void koto_page_music_local_handle_artist_added(
	KotoCartographer * carto,
	KotoArtist * artist,
	gpointer user_data
) {
	(void) carto;
	KotoPageMusicLocal * self = user_data;

	if (!KOTO_IS_PAGE_MUSIC_LOCAL(self)) { // Not the page
		return;
	}

	if (!KOTO_IS_ARTIST(artist)) { // Not an artist
		return;
	}

	if (koto_artist_get_lib_type(artist) != KOTO_LIBRARY_TYPE_MUSIC) { // Not in our music library
		return;
	}

	koto_page_music_local_add_artist(self, artist); // Add the artist if needed
}

void koto_page_music_local_handle_artist_removed(
	KotoCartographer * carto,
	gchar * artist_uuid,
	gchar * artist_name,
	gpointer user_data
) {
	(void) carto;
	(void) artist_uuid;

	KotoPageMusicLocal * self = user_data;

	GtkWidget * existing_artist_page = gtk_stack_get_child_by_name(GTK_STACK(self->stack), artist_name);

	// TODO: Navigate away from artist if we are currently looking at it
	if (GTK_IS_WIDGET(existing_artist_page)) { // Page exists
		gtk_stack_remove(GTK_STACK(self->stack), existing_artist_page); // Remove the artist page
	}

	KotoButton * btn = g_hash_table_lookup(self->artist_name_to_buttons, artist_name);

	if (KOTO_IS_BUTTON(btn)) { // If we have a button for this artist
		gtk_list_box_remove(GTK_LIST_BOX(self->artist_list), GTK_WIDGET(btn)); // Remove the button
	}
}

int koto_page_music_local_sort_artists(
	GtkListBoxRow * artist1,
	GtkListBoxRow * artist2,
	gpointer user_data
) {
	(void) user_data;
	KotoButton * artist1_btn = KOTO_BUTTON(gtk_list_box_row_get_child(artist1));
	KotoButton * artist2_btn = KOTO_BUTTON(gtk_list_box_row_get_child(artist2));

	gchar * artist1_text;
	gchar * artist2_text;

	g_object_get(artist1_btn, "button-text", &artist1_text, NULL);
	g_object_get(artist2_btn, "button-text", &artist2_text, NULL);

	return g_utf8_collate(artist1_text, artist2_text);
}

KotoPageMusicLocal * koto_page_music_local_new() {
	return g_object_new(
		KOTO_TYPE_PAGE_MUSIC_LOCAL,
		"orientation",
		GTK_ORIENTATION_HORIZONTAL,
		NULL
	);
}
