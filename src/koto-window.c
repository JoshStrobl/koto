/* koto-window.c
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

#include <gtk-4.0/gdk/x11/gdkx.h>
#include "indexer/file-indexer.h"
#include "pages/music/music-local.h"
#include "koto-config.h"
#include "koto-nav.h"
#include "koto-playerbar.h"
#include "koto-window.h"

struct _KotoWindow {
	GtkApplicationWindow  parent_instance;
	KotoIndexedLibrary *library;

	GtkWidget        *header_bar;
	GtkWidget *menu_button;
	GtkWidget *search_entry;

	GtkWidget *primary_layout;
	GtkWidget *content_layout;

	KotoNav *nav;
	GtkWidget *pages;
	KotoPlayerBar *player_bar;
};

G_DEFINE_TYPE (KotoWindow, koto_window, GTK_TYPE_APPLICATION_WINDOW)

static void koto_window_class_init (KotoWindowClass *klass) {
	(void)klass;
}

static void koto_window_init (KotoWindow *self) {
	GtkCssProvider* provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource(provider, "/com/github/joshstrobl/koto/style.css");
	gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	create_new_headerbar(self); // Create our headerbar

	self->primary_layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_css_class(self->primary_layout, "primary-layout");
	gtk_widget_set_hexpand(self->primary_layout, TRUE);
	gtk_widget_set_vexpand(self->primary_layout, TRUE);

	self->content_layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_add_css_class(self->content_layout, "content-layout");
	gtk_widget_set_hexpand(self->content_layout, TRUE);
	gtk_widget_set_vexpand(self->content_layout, TRUE);

	self->nav = koto_nav_new();

	if (self->nav != NULL) {
		gtk_box_prepend(GTK_BOX(self->content_layout), koto_nav_get_nav(self->nav));
	}

	self->pages = gtk_stack_new(); // New stack to hold our pages

	if (GTK_IS_STACK(self->pages)) { // Created our stack successfully
		gtk_stack_set_transition_type(GTK_STACK(self->pages), GTK_STACK_TRANSITION_TYPE_OVER_LEFT_RIGHT);
		gtk_box_append(GTK_BOX(self->content_layout), self->pages);
	}

	gtk_box_prepend(GTK_BOX(self->primary_layout), self->content_layout);

	self->player_bar = koto_playerbar_new();

	if (self->player_bar != NULL) {
		GtkWidget *playerbar_main = koto_playerbar_get_main(self->player_bar);
		gtk_box_append(GTK_BOX(self->primary_layout), playerbar_main);
	}

	gtk_window_set_child(GTK_WINDOW(self), self->primary_layout);
#ifdef GDK_WINDOWING_X11
	set_optimal_default_window_size(self);
#else
	gtk_widget_set_size_request(GTK_WIDGET(self), 1200, 675);
#endif
	gtk_window_set_title(GTK_WINDOW(self), "Koto");
	gtk_window_set_icon_name(GTK_WINDOW(self), "audio-headphones");
	gtk_window_set_startup_id(GTK_WINDOW(self), "com.github.joshstrobl.koto");

	g_thread_new("load-library", (void*) load_library, self);
}

void create_new_headerbar(KotoWindow *self) {
	self->header_bar = gtk_header_bar_new();
	gtk_widget_add_css_class(self->header_bar, "hdr");
	g_return_if_fail(GTK_IS_HEADER_BAR(self->header_bar));

	self->menu_button = gtk_button_new_from_icon_name("audio-headphones");

	self->search_entry = gtk_search_entry_new();
	gtk_widget_add_css_class(self->menu_button, "flat");
	gtk_widget_set_can_focus(self->search_entry, TRUE);
	gtk_widget_set_size_request(self->search_entry, 400, -1); // Have 400px width
	g_object_set(self->search_entry, "placeholder-text", "Search...", NULL);

	gtk_header_bar_pack_start(GTK_HEADER_BAR(self->header_bar), self->menu_button);
	gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(self->header_bar), TRUE);
	gtk_header_bar_set_title_widget(GTK_HEADER_BAR(self->header_bar), self->search_entry);

	gtk_window_set_titlebar(GTK_WINDOW(self), self->header_bar);
}

void load_library(KotoWindow *self) {
	KotoIndexedLibrary *lib = koto_indexed_library_new(g_get_user_special_dir(G_USER_DIRECTORY_MUSIC));

	if (lib != NULL) {
		self->library = lib;
		KotoPageMusicLocal* l = koto_page_music_local_new();

		if (GTK_IS_WIDGET(l)) { // Created our local library page
			koto_page_music_local_set_library(l, self->library);
			gtk_stack_add_named(GTK_STACK(self->pages), GTK_WIDGET(l), "music.local");
			// TODO: Remove and do some fancy state loading
			gtk_stack_set_visible_child_name(GTK_STACK(self->pages), "music.local");
		}
	}
}

void set_optimal_default_window_size(KotoWindow *self) {
	GdkDisplay *default_display = gdk_display_get_default();
	g_return_if_fail(GDK_IS_X11_DISPLAY(default_display));

	GdkMonitor *default_monitor = gdk_x11_display_get_primary_monitor(GDK_X11_DISPLAY(default_display)); // Get primary monitor for the X11
	g_return_if_fail(default_monitor);


	GdkRectangle workarea = {0};
	gdk_monitor_get_geometry(default_monitor, &workarea);

	if (workarea.width <= 1280) { // Honestly how do you even get anything done?
		gtk_widget_set_size_request(GTK_WIDGET(self), 1200, 675);
	} else if ((workarea.width > 1280) && (workarea.width <= 1600)) { // Plebian monitor resolution
		gtk_widget_set_size_request(GTK_WIDGET(self), 1300, 709);
	} else if ((workarea.width > 1600) && (workarea.width <= 1920)) { // Something slightly normal
		gtk_widget_set_size_request(GTK_WIDGET(self), 1600, 900);
	} else if ((workarea.width > 1920) && (workarea.width <= 2560)) { // Well aren't you hot stuff?
		gtk_widget_set_size_request(GTK_WIDGET(self), 1920, 1080);
	} else { // Now you're just flexing
		gtk_widget_set_size_request(GTK_WIDGET(self), 2560, 1400);
	}
}
