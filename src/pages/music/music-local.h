/* music-local.h
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

#include <glib-2.0/glib-object.h>
#include <gtk-4.0/gtk/gtk.h>
#include "../../indexer/structs.h"
#include "artist-view.h"

G_BEGIN_DECLS

#define KOTO_TYPE_PAGE_MUSIC_LOCAL (koto_page_music_local_get_type())

G_DECLARE_FINAL_TYPE(KotoPageMusicLocal, koto_page_music_local, KOTO, PAGE_MUSIC_LOCAL, GtkBox)

KotoPageMusicLocal* koto_page_music_local_new();
void koto_page_music_local_add_artist(
	KotoPageMusicLocal * self,
	KotoArtist * artist
);

void koto_page_music_local_handle_artist_click(
	GtkListBox * box,
	GtkListBoxRow * row,
	gpointer data
);

void koto_page_music_local_go_to_artist_by_name(
	KotoPageMusicLocal * self,
	gchar * artist_name
);

void koto_page_music_local_go_to_artist_by_uuid(
	KotoPageMusicLocal * self,
	gchar * artist_uuid
);

void koto_page_music_local_build_ui(KotoPageMusicLocal * self);

int koto_page_music_local_sort_artists(
	GtkListBoxRow * artist1,
	GtkListBoxRow * artist2,
	gpointer user_data
);

G_END_DECLS
