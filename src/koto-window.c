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
#include "components/koto-action-bar.h"
#include "db/cartographer.h"
#include "indexer/structs.h"
#include "pages/music/music-local.h"
#include "pages/playlist/list.h"
#include "playback/engine.h"
#include "playlist/add-remove-track-popover.h"
#include "playlist/current.h"
#include "playlist/create-modify-dialog.h"
#include "koto-config.h"
#include "koto-dialog-container.h"
#include "koto-nav.h"
#include "koto-playerbar.h"
#include "koto-window.h"

extern KotoActionBar *action_bar;
extern KotoAddRemoveTrackPopover *koto_add_remove_track_popup;
extern KotoCartographer *koto_maps;
extern KotoCreateModifyPlaylistDialog *playlist_create_modify_dialog;
extern KotoCurrentPlaylist *current_playlist;
extern KotoPageMusicLocal *music_local_page;
extern KotoPlaybackEngine *playback_engine;

struct _KotoWindow {
	GtkApplicationWindow  parent_instance;
	KotoIndexedLibrary *library;
	KotoCurrentPlaylist *current_playlist;

	KotoDialogContainer *dialogs;

	GtkWidget *overlay;
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
	current_playlist = koto_current_playlist_new();
	playback_engine = koto_playback_engine_new();

	GtkCssProvider* provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource(provider, "/com/github/joshstrobl/koto/style.css");
	gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	create_new_headerbar(self); // Create our headerbar

	self->overlay = gtk_overlay_new(); // Create our overlay
	self->dialogs = koto_dialog_container_new(); // Create our dialog container

	self->primary_layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_add_css_class(self->primary_layout, "primary-layout");
	gtk_widget_set_hexpand(self->primary_layout, TRUE);
	gtk_widget_set_vexpand(self->primary_layout, TRUE);

	playlist_create_modify_dialog = koto_create_modify_playlist_dialog_new(); // Create our Create Playlist dialog
	koto_dialog_container_add_dialog(self->dialogs, "create-modify-playlist", GTK_WIDGET(playlist_create_modify_dialog));

	gtk_overlay_set_child(GTK_OVERLAY(self->overlay), self->primary_layout); // Add our primary layout to the overlay
	gtk_overlay_add_overlay(GTK_OVERLAY(self->overlay), GTK_WIDGET(self->dialogs)); // Add the stack as our overlay

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
		gtk_widget_set_hexpand(self->pages, TRUE);
		gtk_widget_set_vexpand(self->pages, TRUE);
	}

	gtk_box_prepend(GTK_BOX(self->primary_layout), self->content_layout);

	koto_add_remove_track_popup = koto_add_remove_track_popover_new(); // Create our popover for adding and removing tracks
	action_bar = koto_action_bar_new(); // Create our Koto Action Bar

	if (KOTO_IS_ACTION_BAR(action_bar)) { // Is an action bar
		GtkActionBar *bar = koto_action_bar_get_main(action_bar);

		if (GTK_IS_ACTION_BAR(bar)) {
			gtk_box_append(GTK_BOX(self->primary_layout), GTK_WIDGET(bar)); // Add the action
		}
	}

	self->player_bar = koto_playerbar_new();

	if (KOTO_IS_PLAYERBAR(self->player_bar)) { // Is a playerbar
		GtkWidget *playerbar_main = koto_playerbar_get_main(self->player_bar);
		gtk_box_append(GTK_BOX(self->primary_layout), playerbar_main);
	}

	gtk_window_set_child(GTK_WINDOW(self), self->overlay);
#ifdef GDK_WINDOWING_X11
	set_optimal_default_window_size(self);
#endif
	gtk_window_set_title(GTK_WINDOW(self), "Koto");
	gtk_window_set_icon_name(GTK_WINDOW(self), "audio-headphones");
	gtk_window_set_startup_id(GTK_WINDOW(self), "com.github.joshstrobl.koto");

	gtk_widget_queue_draw(self->content_layout);
	g_thread_new("load-library", (void*) load_library, self);
}

void koto_window_add_page(KotoWindow *self, gchar *page_name, GtkWidget *page) {
	gtk_stack_add_named(GTK_STACK(self->pages), page, page_name);
}

void koto_window_go_to_page(KotoWindow *self, gchar *page_name) {
	gtk_stack_set_visible_child_name(GTK_STACK(self->pages), page_name);
}

void koto_window_handle_playlist_added(KotoCartographer *carto, KotoPlaylist *playlist, gpointer user_data) {
	(void) carto;

	if (!KOTO_IS_PLAYLIST(playlist)) {
		return;
	}

	KotoWindow *self = user_data;

	gchar *playlist_uuid = koto_playlist_get_uuid(playlist);
	KotoPlaylistPage *playlist_page = koto_playlist_page_new(playlist_uuid); // Create our new Playlist Page
	koto_window_add_page(self, playlist_uuid, koto_playlist_page_get_main(playlist_page)); // Get the GtkScrolledWindow "main" content of the playlist page and add that as a page to our stack by the playlist UUID
}

void koto_window_hide_dialogs(KotoWindow *self) {
	koto_dialog_container_hide(self->dialogs); // Hide the dialog container
}

void koto_window_remove_page(KotoWindow *self, gchar *page_name) {
	GtkWidget *page = gtk_stack_get_child_by_name(GTK_STACK(self->pages), page_name);
	g_return_if_fail(page != NULL);
	gtk_stack_remove(GTK_STACK(self->pages), page);
}

void koto_window_show_dialog(KotoWindow *self, gchar *dialog_name) {
	koto_dialog_container_show_dialog(self->dialogs, dialog_name);
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
		music_local_page = koto_page_music_local_new();

		// TODO: Remove and do some fancy state loading
		koto_window_add_page(self, "music.local", GTK_WIDGET(music_local_page));
		koto_window_go_to_page(self, "music.local");
		gtk_widget_show(self->pages); // Do not remove this. Will cause sporadic hiding of the local page content otherwise.
		koto_page_music_local_set_library(music_local_page, self->library);
	}

	g_thread_exit(0);
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
		gtk_widget_set_size_request(GTK_WIDGET(self), 2560, 1440);
	}
}
