/* artist.h
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
#include "album.h"

G_BEGIN_DECLS

#define KOTO_TYPE_INDEXED_ARTIST koto_indexed_artist_get_type()
G_DECLARE_FINAL_TYPE (KotoIndexedArtist, koto_indexed_artist, KOTO, INDEXED_ARTIST, GObject);

KotoIndexedArtist* koto_indexed_artist_new(const gchar *path);

void koto_indexed_artist_add_album(KotoIndexedArtist *self, KotoIndexedAlbum *album);
guint koto_indexed_artist_find_album_with_name(gconstpointer *album_data, gconstpointer *album_name_data);
GList* koto_indexed_artist_get_albums(KotoIndexedArtist *self);
void koto_indexed_artist_remove_album(KotoIndexedArtist *self, KotoIndexedAlbum *album);
void koto_indexed_artist_remove_album_by_name(KotoIndexedArtist *self, gchar *album_name);
void koto_indexed_artist_set_artist_name(KotoIndexedArtist *self, const gchar *artist_name);
void koto_indexed_artist_update_path(KotoIndexedArtist *self, const gchar *new_path);
void output_artists(gpointer artist_key, gpointer artist_ptr, gpointer data);

G_END_DECLS
