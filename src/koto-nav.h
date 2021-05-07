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

#pragma once
#include <gtk-4.0/gtk/gtk.h>
#include "db/cartographer.h"
#include "indexer/structs.h"

G_BEGIN_DECLS

#define KOTO_TYPE_NAV (koto_nav_get_type())

G_DECLARE_FINAL_TYPE (KotoNav, koto_nav, KOTO, NAV, GObject)

KotoNav* koto_nav_new (void);
void koto_nav_create_audiobooks_section(KotoNav *self);
void koto_nav_create_music_section(KotoNav *self);
void koto_nav_create_playlist_section(KotoNav *self);
void koto_nav_create_podcasts_section(KotoNav *self);
void koto_nav_handle_playlist_add_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);
void koto_nav_handle_playlist_added(KotoCartographer *carto, KotoPlaylist *playlist, gpointer user_data);
void koto_nav_handle_playlist_removed(KotoCartographer *carto, gchar *playlist_uuid, gpointer user_data);
void koto_nav_handle_local_music_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

GtkWidget* koto_nav_get_nav(KotoNav *self);

G_END_DECLS
