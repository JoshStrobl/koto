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
#include "koto-config.h"
#include "koto-button.h"
#include "koto-expander.h"
#include "koto-nav.h"

struct _KotoNav {
	GObject parent_instance;
	GtkWidget *win;
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
	GObjectClass parent_class;
};

G_DEFINE_TYPE(KotoNav, koto_nav, G_TYPE_OBJECT);

static void koto_nav_class_init(KotoNavClass *c) {
	(void) c;
}

static void koto_nav_init(KotoNav *self) {
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

	KotoButton *h_button = koto_button_new_with_icon("Home", "user-home-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_SMALL);

	if (h_button != NULL) {
		self->home_button = h_button;
		gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->home_button));
	}

	koto_nav_create_audiobooks_section(self);
	koto_nav_create_music_section(self);
	koto_nav_create_podcasts_section(self);

	KotoButton *playlist_add_button = koto_button_new_with_icon("", "list-add-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_SMALL);
	KotoExpander *pl_expander = koto_expander_new_with_button("playlist-symbolic", "Playlists", playlist_add_button);

	if (pl_expander != NULL) {
		self->playlists_expander = pl_expander;
		gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->playlists_expander));
	}
}

void koto_nav_create_audiobooks_section(KotoNav *self) {
	KotoExpander *a_expander = koto_expander_new("ephy-bookmarks-symbolic", "Audiobooks");

	self->audiobook_expander = a_expander;
	gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->audiobook_expander));

	GtkWidget *new_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	koto_expander_set_content(a_expander, new_content);

	self->audiobooks_local = koto_button_new_plain("Local Library");
	self->audiobooks_audible = koto_button_new_plain("Audible");
	self->audiobooks_librivox = koto_button_new_plain("LibriVox");

	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->audiobooks_local));
	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->audiobooks_audible));
	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->audiobooks_librivox));
}

void koto_nav_create_music_section(KotoNav *self) {
	KotoExpander *m_expander = koto_expander_new("emblem-music-symbolic", "Music");
	self->music_expander = m_expander;
	gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->music_expander));

	GtkWidget *new_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->music_local = koto_button_new_plain("Local Library");
	self->music_radio = koto_button_new_plain("Radio");

	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->music_local));
	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->music_radio));

	koto_expander_set_content(m_expander, new_content);
}

void koto_nav_create_podcasts_section(KotoNav *self) {
	KotoExpander *p_expander = koto_expander_new("microphone-sensitivity-high-symbolic", "Podcasts");
	self->podcast_expander = p_expander;
	gtk_box_append(GTK_BOX(self->content), GTK_WIDGET(self->podcast_expander));

	GtkWidget *new_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	self->podcasts_local = koto_button_new_plain("Library");
	self->podcasts_discover = koto_button_new_plain("Find New Podcasts");

	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->podcasts_local));
	gtk_box_append(GTK_BOX(new_content), GTK_WIDGET(self->podcasts_discover));

	koto_expander_set_content(p_expander, new_content);
}

GtkWidget* koto_nav_get_nav(KotoNav *self) {
	return self->win;
}

KotoNav* koto_nav_new(void) {
	return g_object_new(KOTO_TYPE_NAV ,NULL);
}
