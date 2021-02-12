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

#include <gtk-3.0/gtk/gtk.h>
#include "koto-config.h"
#include "koto-button.h"
#include "koto-expander.h"
#include "koto-nav.h"

struct _KotoNav {
	GtkScrolledWindow parent_instance;
	GtkWidget *content;

	KotoButton *home_button;
	KotoExpander *audiobook_expander;
	KotoExpander *music_expander;
	KotoExpander *podcast_expander;
	KotoExpander *playlists_expander;

	// Audiobooks

	KotoButton *audiobooks_local;
	KotoButton *audiobooks_audible;
	KotoButton *audiobooks_librivox;

	// Music

	KotoButton *music_local;
	KotoButton *music_radio;

	// Podcasts

	KotoButton *podcasts_local;
	KotoButton *podcasts_discover;
};

struct _KotoNavClass {
	GtkScrolledWindowClass parent_class;
};

G_DEFINE_TYPE(KotoNav, koto_nav, GTK_TYPE_SCROLLED_WINDOW);

static void koto_nav_class_init(KotoNavClass *c) {

}

static void koto_nav_init(KotoNav *self) {
	gtk_widget_set_size_request(GTK_WIDGET(self), 300, -1); // Take up 300px width
	self->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	GtkStyleContext *style = gtk_widget_get_style_context(GTK_WIDGET(self));
	gtk_style_context_add_class(style, "primary-nav");

	gtk_container_add(GTK_CONTAINER(self), self->content);

	KotoButton *h_button = koto_button_new_with_icon("Home", "user-home-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_SMALL);

	if (h_button != NULL) {
		self->home_button = h_button;
		gtk_box_pack_start(GTK_BOX(self->content), GTK_WIDGET(self->home_button), FALSE, FALSE, 0);
	}

	koto_nav_create_audiobooks_section(self);
	koto_nav_create_music_section(self);
	koto_nav_create_podcasts_section(self);


	KotoButton *playlist_add_button = koto_button_new_with_icon("", "list-add-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_SMALL);
	KotoExpander *pl_expander = koto_expander_new_with_button("playlist-symbolic", "Playlists", playlist_add_button);

	if (pl_expander != NULL) {
		self->playlists_expander = pl_expander;
		gtk_box_pack_start(GTK_BOX(self->content), GTK_WIDGET(self->playlists_expander), FALSE, FALSE, 0);
	}

	gtk_widget_show_all(GTK_WIDGET(self));
}

void koto_nav_create_audiobooks_section(KotoNav *self) {
	KotoExpander *a_expander = koto_expander_new("ephy-bookmarks-symbolic", "Audiobooks");

	self->audiobook_expander = a_expander;
	gtk_box_pack_start(GTK_BOX(self->content), GTK_WIDGET(self->audiobook_expander), FALSE, FALSE, 0);

	GtkWidget *new_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->audiobooks_local = koto_button_new_plain("Local Library");
	self->audiobooks_audible = koto_button_new_plain("Audible");
	self->audiobooks_librivox = koto_button_new_plain("LibriVox");

	gtk_box_pack_start(GTK_BOX(new_content), GTK_WIDGET(self->audiobooks_local), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(new_content), GTK_WIDGET(self->audiobooks_audible), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(new_content), GTK_WIDGET(self->audiobooks_librivox), FALSE, FALSE, 0);

	koto_expander_set_content(a_expander, new_content);
}

void koto_nav_create_music_section(KotoNav *self) {
	KotoExpander *m_expander = koto_expander_new("emblem-music-symbolic", "Music");
	self->music_expander = m_expander;
	gtk_box_pack_start(GTK_BOX(self->content), GTK_WIDGET(self->music_expander), FALSE, FALSE, 0);

	GtkWidget *new_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->music_local = koto_button_new_plain("Local Library");
	self->music_radio = koto_button_new_plain("Radio");

	gtk_box_pack_start(GTK_BOX(new_content), GTK_WIDGET(self->music_local), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(new_content), GTK_WIDGET(self->music_radio), FALSE, FALSE, 0);

	koto_expander_set_content(m_expander, new_content);
}

void koto_nav_create_podcasts_section(KotoNav *self) {
	KotoExpander *p_expander = koto_expander_new("microphone-sensitivity-high-symbolic", "Podcasts");
	self->podcast_expander = p_expander;
	gtk_box_pack_start(GTK_BOX(self->content), GTK_WIDGET(self->podcast_expander), FALSE, FALSE, 0);

	GtkWidget *new_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->podcasts_local = koto_button_new_plain("Library");
	self->podcasts_discover = koto_button_new_plain("Find New Podcasts");

	gtk_box_pack_start(GTK_BOX(new_content), GTK_WIDGET(self->podcasts_local), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(new_content), GTK_WIDGET(self->podcasts_discover), FALSE, FALSE, 0);

	koto_expander_set_content(p_expander, new_content);
}

KotoNav* koto_nav_new(void) {
	return g_object_new(KOTO_TYPE_NAV,
		"hscrollbar-policy", GTK_POLICY_NEVER,
		"min-content-width", 300,
		"max-content-width", 300,
		"propagate-natural-height", TRUE,
		"propagate-natural-width", TRUE,
		"shadow-type", GTK_SHADOW_NONE
	,NULL);
}
