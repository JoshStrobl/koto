/* genres-banner.c
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
#include "../../components/button.h"
#include "../../koto-utils.h"
#include "genre-button.h"
#include "genres-banner.h"

enum {
	SIGNAL_GENRE_CLICKED,
	N_SIGNALS
};

static guint banner_signals[N_SIGNALS] = {
	0
};

struct _KotoAudiobooksGenresBanner {
	GObject parent_instance;

	GtkWidget * main; // Our main content

	GtkWidget * banner_revealer; // Our GtkRevealer for the strip
	GtkWidget * banner_viewport; // Our banner viewport
	GtkWidget * banner_content; // Our banner content as a horizontal box

	GtkWidget * strip_revealer; // Our GtkRevealer for the strip
	GtkWidget * strip_viewport; // Our strip viewport
	GtkWidget * strip_content; // Our strip content

	GHashTable * genre_ids_to_names; // Our genre "ids" to the name
	GHashTable * genre_ids_to_ptrs; // Our HashTable of genre IDs to our GPtrArray with pointers to banner and strip buttons
};

struct _KotoAudiobooksGenresBannerClass {
	GObjectClass parent_instance;
	void (* genre_clicked) (gchar * genre);
};

G_DEFINE_TYPE(KotoAudiobooksGenresBanner, koto_audiobooks_genres_banner, G_TYPE_OBJECT);

static void koto_audiobooks_genres_banner_class_init(KotoAudiobooksGenresBannerClass * c) {
	GObjectClass * gobject_class = G_OBJECT_CLASS(c);

	banner_signals[SIGNAL_GENRE_CLICKED] = g_signal_new(
		"genre-clicked",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoAudiobooksGenresBannerClass, genre_clicked),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		G_TYPE_CHAR
	);
}

static void koto_audiobooks_genres_banner_init(KotoAudiobooksGenresBanner * self) {
	self->genre_ids_to_ptrs = g_hash_table_new(g_str_hash, g_str_equal);
	self->genre_ids_to_names = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(self->genre_ids_to_names, g_strdup("hip-hop"), g_strdup("Hip-hop"));
	g_hash_table_insert(self->genre_ids_to_names, g_strdup("indie"), g_strdup("Indie"));
	g_hash_table_insert(self->genre_ids_to_names, g_strdup("sci-fi"), g_strdup("Science Fiction"));

	self->main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); // Create the main box as a vertical box
	gtk_widget_add_css_class(self->main, "genres-banner");
	gtk_widget_set_halign(self->main, GTK_ALIGN_START);
	gtk_widget_set_vexpand(self->main, FALSE);

	self->banner_revealer = gtk_revealer_new(); // Create our revealer for the banner
	self->strip_revealer = gtk_revealer_new(); // Create a revealer for the strip

	gtk_revealer_set_transition_type(GTK_REVEALER(self->strip_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
	gtk_revealer_set_transition_type(GTK_REVEALER(self->banner_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);

	self->banner_viewport = gtk_viewport_new(NULL, NULL); // Create our viewport for enabling the banner content to be scrollable
	self->strip_viewport = gtk_viewport_new(NULL, NULL); // Create our viewport for enabling the strip content to be scrollable

	self->banner_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); // Create our horizontal box for the content
	gtk_widget_add_css_class(self->banner_content, "large-banner");

	self->strip_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); // Create the horizontal box for the strip content
	gtk_widget_add_css_class(self->strip_content, "strip");

	gtk_revealer_set_child(GTK_REVEALER(self->banner_revealer), self->banner_viewport); // Set the viewport to be the content which gets revealed
	gtk_revealer_set_child(GTK_REVEALER(self->strip_revealer), self->strip_viewport); // Set the viewport to be the cont ent which gets revealed
	gtk_revealer_set_reveal_child(GTK_REVEALER(self->banner_revealer), TRUE); // Show the banner by default

	gtk_viewport_set_child(GTK_VIEWPORT(self->banner_viewport), self->banner_content); // Set the banner content for this viewport
	gtk_viewport_set_child(GTK_VIEWPORT(self->strip_viewport), self->strip_content); // Set the strip content for this viewport

	gtk_box_append(GTK_BOX(self->main), self->strip_revealer);
	gtk_box_append(GTK_BOX(self->main), self->banner_revealer);
}

void koto_audiobooks_genres_banner_add_genre(
	KotoAudiobooksGenresBanner * self,
	gchar * genre_id
) {
	if (!KOTO_IS_AUDIOBOOKS_GENRES_BANNER(self)) { // Not a banner
		return;
	}

	if (!koto_utils_string_is_valid(genre_id)) { // Not a valid genre ID
		return;
	}

	if (g_hash_table_contains(self->genre_ids_to_ptrs, genre_id)) { // Already have added this
		return;
	}

	gchar * name = g_hash_table_lookup(self->genre_ids_to_names, genre_id); // Get the named equivelant for this ID

	if (name == NULL) { // Didn't find a name equivelant
		name = g_strdup(genre_id); // Just duplicate the genre ID for now
	}

	KotoButton * genre_strip_button = koto_button_new_plain(name); // Create a new button with the name
	gtk_box_append(GTK_BOX(self->strip_content), GTK_WIDGET(genre_strip_button)); // Add our KotoButton to the strip content
	KotoAudiobooksGenreButton * genre_banner_button = koto_audiobooks_genre_button_new(genre_id, name); // Create our big button
	gtk_box_append(GTK_BOX(self->banner_content), GTK_WIDGET(genre_banner_button));

	GPtrArray * buttons = g_ptr_array_new();
	g_ptr_array_add(buttons, (gpointer) genre_strip_button); // Add our KotoButton as a gpointer to our GPtrArray as the first item
	g_ptr_array_add(buttons, (gpointer) genre_banner_button); // Add our KotoAudiobooksGenresButton as a gpointer to our GPtrArray as the second item

	g_hash_table_replace(self->genre_ids_to_ptrs, genre_id, buttons); // Add our GPtrArray to our genre_ids_to_ptrs
}

GtkWidget * koto_audiobooks_genres_banner_get_main(KotoAudiobooksGenresBanner * self) {
	return KOTO_IS_AUDIOBOOKS_GENRES_BANNER(self) ? self->main : NULL;
}

KotoAudiobooksGenresBanner * koto_audiobooks_genres_banner_new() {
	return g_object_new(KOTO_TYPE_AUDIOBOOKS_GENRE_BANNER, NULL);
}