/* library.c
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
#include "../../db/cartographer.h"
#include "../../indexer/structs.h"
#include "../../koto-utils.h"
#include "../../koto-window.h"
#include "genres-banner.h"
#include "library.h"
#include "writer-page.h"

extern KotoCartographer * koto_maps;
extern KotoWindow * main_window;

struct _KotoAudiobooksLibraryPage {
	GObject parent_instance;

	GtkWidget * main; // Our main content, contains banner and scrolled window
	GtkWidget * content_scroll; // Our Scrolled Window
	GtkWidget * content; // Content inside scrolled window

	KotoAudiobooksGenresBanner * banner;

	GtkWidget * writers_flow;
	GHashTable * writers_to_buttons; // HashTable of UUIDs of "Artists" to their KotoButton
	GHashTable * writers_to_pages; // HashTable of UUIDs of "Artists" to their KotoAudiobooksWritersPage
};

struct _KotoAudiobooksLibraryPageClass {
	GObjectClass parent_instance;
};

G_DEFINE_TYPE(KotoAudiobooksLibraryPage, koto_audiobooks_library_page, G_TYPE_OBJECT);

KotoAudiobooksLibraryPage * audiobooks_library_page;

static void koto_audiobooks_library_page_init(KotoAudiobooksLibraryPage * self) {
	self->main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_css_class(self->main, "audiobook-library");
	self->content_scroll = gtk_scrolled_window_new(); // Create our GtkScrolledWindow
	self->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_vexpand(self->content, TRUE); // Ensure content expands vertically to allow flowboxchild to take up as much space as it requests
	gtk_widget_add_css_class(self->content, "content");

	self->banner = koto_audiobooks_genres_banner_new(); // Create our banner

	self->writers_flow = gtk_flow_box_new(); // Create our flow box
	//gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(self->writers_flow), TRUE);
	gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(self->writers_flow), 100); // Set to a random amount that is not realistic, however GTK sets a default to 7 which is too small.
	gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(self->writers_flow), GTK_SELECTION_NONE);
	gtk_widget_add_css_class(self->writers_flow, "writers-button-flow");

	self->writers_to_buttons = g_hash_table_new(g_str_hash, g_str_equal);
	self->writers_to_pages = g_hash_table_new(g_str_hash, g_str_equal);

	g_signal_connect(koto_maps, "album-added", G_CALLBACK(koto_audiobooks_library_page_handle_add_album), self); // Notify when we have a new Album
	g_signal_connect(koto_maps, "artist-added", G_CALLBACK(koto_audiobooks_library_page_handle_add_artist), self); // Notify when we have a new Artist

	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->content_scroll), self->content); // Add our content to the content scroll
	gtk_box_append(GTK_BOX(self->main), koto_audiobooks_genres_banner_get_main(self->banner)); // Add the banner to the content
	gtk_box_append(GTK_BOX(self->main), self->content_scroll); // Add our scroll window to the main content
	gtk_box_append(GTK_BOX(self->content), self->writers_flow); // Add our flowbox to the content
}

static void koto_audiobooks_library_page_class_init(KotoAudiobooksLibraryPageClass * c) {
	(void) c;
}

void koto_audiobooks_library_page_create_artist_ux(
	KotoAudiobooksLibraryPage * self,
	KotoArtist * artist
) {
	if (!KOTO_IS_AUDIOBOOKS_LIBRARY_PAGE(self)) { // Not a AudiobooksLibraryPage
		return;
	}

	if (!KOTO_IS_ARTIST(artist)) { // Not an artist
		return;
	}

	gchar * artist_uuid = koto_artist_get_uuid(artist); // Get the artist UUID

	if (!g_hash_table_contains(self->writers_to_pages, artist_uuid)) { // Don't have the page
		KotoWriterPage * writers_page = koto_writer_page_new(artist);
		koto_window_add_page(main_window, artist_uuid, koto_writer_page_get_main(writers_page)); // Add the page to the stack
	}

	if (!g_hash_table_contains(self->writers_to_buttons, artist_uuid)) { // Don't have the button for this
		KotoButton * artist_button = koto_button_new_plain(koto_artist_get_name(artist)); // Create a KotoButton for this artist
		koto_button_set_data(artist_button, artist_uuid); // Set the artist_uuid as the data for the button

		koto_button_set_text_justify(artist_button, GTK_JUSTIFY_CENTER); // Center the text
		koto_button_set_text_wrap(artist_button, TRUE);
		gtk_widget_add_css_class(GTK_WIDGET(artist_button), "writer-button");
		gtk_widget_set_size_request(GTK_WIDGET(artist_button), 260, 120);

		koto_button_add_click_handler(artist_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_button_global_page_nav_callback), artist_button);

		g_hash_table_replace(self->writers_to_buttons, artist_uuid, artist_button); // Add the button to the Writers to Buttons hashtable
		gtk_flow_box_insert(GTK_FLOW_BOX(self->writers_flow), GTK_WIDGET(artist_button), -1); // Append the button to our flowbox

		GtkWidget * button_parent = gtk_widget_get_parent(GTK_WIDGET(artist_button)); // This is the GtkFlowboxChild that the button is in
		gtk_widget_set_halign(button_parent, GTK_ALIGN_START);
	}
}

void koto_audiobooks_library_page_handle_add_album(
	KotoCartographer * carto,
	KotoAlbum * album,
	KotoAudiobooksLibraryPage * self
) {

	if (!KOTO_IS_CARTOGRAPHER(carto)) { // Not cartographer
		return;
	}

	if (!KOTO_IS_ALBUM(album)) { // Not an album
		return;
	}

	if (!KOTO_IS_AUDIOBOOKS_LIBRARY_PAGE(self)) { // Not a AudiobooksLibraryPage
		return;
	}

	gchar * artist_uuid = koto_album_get_artist_uuid(album); // Get the Album's artist UUID
	KotoArtist * artist = koto_cartographer_get_artist_by_uuid(koto_maps, artist_uuid);

	if (!KOTO_IS_ARTIST(artist)) { // Failed to get artist
		return;
	}

	if (koto_artist_get_lib_type(artist) != KOTO_LIBRARY_TYPE_AUDIOBOOK) { // Not in an Audiobook library
		return;
	}

	koto_audiobooks_library_page_add_genres(self, koto_album_get_genres(album)); // Add all the genres necessary for this album
}

void koto_audiobooks_library_page_handle_add_artist(
	KotoCartographer * carto,
	KotoArtist * artist,
	KotoAudiobooksLibraryPage * self
) {
	if (!KOTO_IS_CARTOGRAPHER(carto)) { // Not cartographer
		return;
	}

	if (!KOTO_IS_ARTIST(artist)) { // Not an artist
		return;
	}

	if (!KOTO_IS_AUDIOBOOKS_LIBRARY_PAGE(self)) { // Not a AudiobooksLibraryPage
		return;
	}

	if (koto_artist_get_lib_type(artist) != KOTO_LIBRARY_TYPE_AUDIOBOOK) { // Not in an Audiobook library
		return;
	}

	koto_audiobooks_library_page_create_artist_ux(self, artist); // Create the UX for the artist if necessary
}

void koto_audiobooks_library_page_add_genres(
	KotoAudiobooksLibraryPage * self,
	GList * genres
) {
	if (!KOTO_IS_AUDIOBOOKS_LIBRARY_PAGE(self)) { // Not a AudiobooksLibraryPage
		return;
	}

	if (g_list_length(genres) == 0) { // No contents
		return;
	}

	GList * cur_list;
	for (cur_list = genres; cur_list != NULL; cur_list = cur_list->next) { // Iterate over each genre
		gchar * genre = (gchar*) cur_list->data;
		if (!koto_utils_string_is_valid(genre)) {
			continue;
		}

		if (g_strcmp0(genre, "audiobook") == 0) { // Is generic
			continue;
		}

		koto_audiobooks_genres_banner_add_genre(self->banner, genre); // Add this genre
	}
}

GtkWidget * koto_audiobooks_library_page_get_main(KotoAudiobooksLibraryPage * self) {
	return KOTO_IS_AUDIOBOOKS_LIBRARY_PAGE(self) ? self->main : NULL;
}

KotoAudiobooksLibraryPage * koto_audiobooks_library_page_new() {
	return g_object_new(KOTO_TYPE_AUDIOBOOKS_LIBRARY_PAGE, NULL);
}