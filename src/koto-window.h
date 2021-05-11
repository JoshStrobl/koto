/* koto-window.h
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

#pragma once

#include <gtk-4.0/gtk/gtk.h>
#include "db/cartographer.h"
#include "playlist/playlist.h"

G_BEGIN_DECLS

#define KOTO_TYPE_WINDOW (koto_window_get_type())

G_DECLARE_FINAL_TYPE(KotoWindow, koto_window, KOTO, WINDOW, GtkApplicationWindow)

void koto_window_add_page(
	KotoWindow * self,
	gchar * page_name,
	GtkWidget * page
);

void koto_window_go_to_page(
	KotoWindow * self,
	gchar * page_name
);

void koto_window_handle_playlist_added(
	KotoCartographer * carto,
	KotoPlaylist * playlist,
	gpointer user_data
);

void koto_window_hide_dialogs(KotoWindow * self);

void koto_window_remove_page(
	KotoWindow * self,
	gchar * page_name
);

void koto_window_show_dialog(
	KotoWindow * self,
	gchar * dialog_name
);

void create_new_headerbar(KotoWindow * self);

void handle_album_added();

void load_library(KotoWindow * self);

void set_optimal_default_window_size(KotoWindow * self);

G_END_DECLS
