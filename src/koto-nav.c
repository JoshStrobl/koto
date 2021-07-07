/* koto-nav.c
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
#include "db/cartographer.h"
#include "indexer/structs.h"
#include "playlist/playlist.h"
#include "config/config.h"
#include "koto-button.h"
#include "koto-expander.h"
#include "koto-nav.h"
#include "koto-utils.h"
#include "koto-window.h"

extern KotoCartographer * koto_maps;
extern KotoWindow * main_window;

struct _KotoNav {
	GObject parent_instance;
	GtkWidget * win;
	GtkWidget * content;

	KotoButton * home_button;
	KotoExpander * audiobook_expander;
	KotoExpander * music_expander;
	KotoExpander * podcast_expander;
	KotoExpander * playlists_expander;

	// Audiobooks

	KotoButton * audiobooks_local;
	KotoButton * audiobooks_audible;
	KotoButton * audiobooks_librivox;

	// Music

	KotoButton * music_local;
	KotoButton * music_radio;

	// Playlists

	GHashTable * playlist_buttons;

	// Podcasts

	KotoButton * podcasts_local;
	KotoButton * podcasts_discover;
};

struct _KotoNavClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE(KotoNav, koto_nav, G_TYPE_OBJECT);

static void koto_nav_class_init(KotoNavClass * c) {
	(void) c;
}

static void koto_nav_init(KotoNav * self) {
	self->playlist_buttons = g_hash_table_new(g_str_hash, g_str_equal);
	self->win = gtk_scrolled_window_new();
	gtk_widget_set_hexpand_set(self->win, TRUE); // using hexpand-set works, hexpand seems to break it by causing it to take up way too much space
	gtk_widget_set_size_request(self->win, 300, -1);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self->win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(self->win), 300);
	gtk_scrolled_window_set_max_content_width(GTK_SCROLLED_WINDOW(self->win), 300);
	gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(self->win), TRUE);

	self->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	gtk_widget_add_css_class(self->win, "primary-nav");
	gtk_widget_set_vexpand(self->win, TRUE);

	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->win), self->content);

	KotoButton * h_button = koto_button_new_with_icon("Home", "user-home-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_SMALL);

	if (h_button != NULL) {
		self->home_button = h_button;
		gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->home_button));
	}

	koto_nav_create_audiobooks_section(self);
	koto_nav_create_music_section(self);
	koto_nav_create_podcasts_section(self);
	koto_nav_create_playlist_section(self);
}

void koto_nav_create_audiobooks_section(KotoNav * self) {
	KotoExpander * a_expander = koto_expander_new("ephy-bookmarks-symbolic", "Audiobooks");

	self->audiobook_expander = a_expander;
	gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->audiobook_expander));

	GtkWidget * new_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	koto_expander_set_content(a_expander, new_content);

	self->audiobooks_local = koto_button_new_plain("Library");
	self->audiobooks_audible = koto_button_new_plain("Audible");
	self->audiobooks_librivox = koto_button_new_plain("LibriVox");

	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->audiobooks_local));
	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->audiobooks_audible));
	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->audiobooks_librivox));
}

void koto_nav_create_music_section(KotoNav * self) {
	KotoExpander * m_expander = koto_expander_new("emblem-music-symbolic", "Music");

	self->music_expander = m_expander;
	gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->music_expander));

	GtkWidget * new_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->music_local = koto_button_new_plain("Library");
	self->music_radio = koto_button_new_plain("Radio");

	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->music_local));
	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->music_radio));

	koto_expander_set_content(m_expander, new_content);
	koto_button_add_click_handler(self->music_local, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_nav_handle_local_music_click), NULL);
}

void koto_nav_create_playlist_section(KotoNav * self) {
	KotoButton * playlist_add_button = koto_button_new_with_icon("", "list-add-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_SMALL);
	KotoExpander * pl_expander = koto_expander_new_with_button("playlist-symbolic", "Playlists", playlist_add_button);

	self->playlists_expander = pl_expander;
	gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->playlists_expander));

	// TODO: Turn into ListBox to sort playlists
	GtkWidget * playlist_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	koto_expander_set_content(self->playlists_expander, playlist_list);
	koto_button_add_click_handler(playlist_add_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_nav_handle_playlist_add_click), NULL);

	g_signal_connect(koto_maps, "playlist-added", G_CALLBACK(koto_nav_handle_playlist_added), self);
	g_signal_connect(koto_maps, "playlist-removed", G_CALLBACK(koto_nav_handle_playlist_removed), self);
}

void koto_nav_create_podcasts_section(KotoNav * self) {
	KotoExpander * p_expander = koto_expander_new("microphone-sensitivity-high-symbolic", "Podcasts");

	self->podcast_expander = p_expander;
	gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->podcast_expander));

	GtkWidget * new_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->podcasts_local = koto_button_new_plain("Library");
	self->podcasts_discover = koto_button_new_plain("Find New Podcasts");

	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->podcasts_local));
	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->podcasts_discover));

	koto_expander_set_content(p_expander, new_content);
}

void koto_nav_handle_playlist_add_click(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
) {
	(void) gesture;
	(void) n_press;
	(void) x;
	(void) y;
	(void) user_data;
	koto_window_show_dialog(main_window, "create-modify-playlist");
}

void koto_nav_handle_local_music_click(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
) {
	(void) gesture;
	(void) n_press;
	(void) x;
	(void) y;
	(void) user_data;
	koto_window_go_to_page(main_window, "music.local"); // Go to the playlist page
}

void koto_nav_handle_playlist_button_click(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
) {
	(void) gesture;
	(void) n_press;
	(void) x;
	(void) y;
	gchar * playlist_uuid = user_data;

	koto_window_go_to_page(main_window, playlist_uuid); // Go to the playlist page
}

void koto_nav_handle_playlist_added(
	KotoCartographer * carto,
	KotoPlaylist * playlist,
	gpointer user_data
) {
	(void) carto;
	if (!KOTO_IS_PLAYLIST(playlist)) {
		return;
	}

	KotoNav * self = user_data;

	if (!KOTO_IS_NAV(self)) {
		return;
	}

	gchar * playlist_uuid = koto_playlist_get_uuid(playlist); // Get the UUID for a playlist

	if (g_hash_table_contains(self->playlist_buttons, playlist_uuid)) { // Already added button
		g_free(playlist_uuid);
		return;
	}

	gchar * playlist_name = koto_playlist_get_name(playlist);
	gchar * playlist_art_path = koto_playlist_get_artwork(playlist); // Get any file path for it
	KotoButton * playlist_button = NULL;

	if (koto_utils_is_string_valid(playlist_art_path)) { // Have a file associated
		playlist_button = koto_button_new_with_file(playlist_name, playlist_art_path, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	} else { // No file associated
		playlist_button = koto_button_new_with_icon(playlist_name, "audio-x-generic-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	}

	if (KOTO_IS_BUTTON(playlist_button)) {
		g_hash_table_insert(self->playlist_buttons, playlist_uuid, playlist_button); // Add the button

		// TODO: Make this a ListBox and sort the playlists alphabetically
		GtkBox * playlist_expander_content = GTK_BOX(koto_expander_get_content(self->playlists_expander));

		if (GTK_IS_BOX(playlist_expander_content)) {
			gtk_box_append(playlist_expander_content, GTK_WIDGET(playlist_button));

			koto_button_add_click_handler(playlist_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_nav_handle_playlist_button_click), playlist_uuid);
			koto_window_handle_playlist_added(koto_maps, playlist, main_window); // TODO: MOVE THIS
			g_signal_connect(playlist, "modified", G_CALLBACK(koto_nav_handle_playlist_modified), self);
		}
	}
}

void koto_nav_handle_playlist_modified(
	KotoPlaylist * playlist,
	gpointer user_data
) {
	if (!KOTO_IS_PLAYLIST(playlist)) {
		return;
	}

	KotoNav * self = user_data;

	if (!KOTO_IS_NAV(self)) {
		return;
	}

	gchar * playlist_uuid = koto_playlist_get_uuid(playlist); // Get the UUID for a playlist

	KotoButton * playlist_button = g_hash_table_lookup(self->playlist_buttons, playlist_uuid);

	if (!KOTO_IS_BUTTON(playlist_button)) {
		return;
	}

	gchar * artwork = koto_playlist_get_artwork(playlist); // Get the artwork

	if (koto_utils_is_string_valid(artwork)) { // Have valid artwork
		koto_button_set_file_path(playlist_button, artwork); // Update the artwork path
	}

	gchar * name = koto_playlist_get_name(playlist); // Get the name

	if (koto_utils_is_string_valid(name)) { // Have valid name
		koto_button_set_text(playlist_button, name); // Update the button text
	}
}

void koto_nav_handle_playlist_removed(
	KotoCartographer * carto,
	gchar * playlist_uuid,
	gpointer user_data
) {
	(void) carto;
	KotoNav * self = user_data;

	if (!g_hash_table_contains(self->playlist_buttons, playlist_uuid)) { // Does not contain this
		return;
	}

	KotoButton * playlist_btn = g_hash_table_lookup(self->playlist_buttons, playlist_uuid); // Get the playlist button

	if (!KOTO_IS_BUTTON(playlist_btn)) { // Not a playlist button
		return;
	}

	GtkBox * playlist_expander_content = GTK_BOX(koto_expander_get_content(self->playlists_expander));

	gtk_box_remove(playlist_expander_content, GTK_WIDGET(playlist_btn)); // Remove the button
	g_hash_table_remove(self->playlist_buttons, playlist_uuid); // Remove from the playlist buttons hash table
}

GtkWidget * koto_nav_get_nav(KotoNav * self) {
	return self->win;
}

KotoNav * koto_nav_new(void) {
	return g_object_new(KOTO_TYPE_NAV, NULL);
}
