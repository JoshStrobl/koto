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

G_BEGIN_DECLS

#define KOTO_INDEXED_ARTIST_TYPE koto_indexed_artist_get_type()
G_DECLARE_FINAL_TYPE (KotoIndexedArtist, koto_indexed_artist, KOTO, INDEXED_ARTIST, GObject);

KotoIndexedArtist* koto_indexed_artist_new(const gchar *path);

void add_album(KotoIndexedArtist *self, KotoIndexedAlbum *album);
KotoIndexedAlbum** get_albums(KotoIndexedArtist *self);
void remove_album(KotoIndexedArtist *self, KotoIndexedAlbum *album);
void remove_album_by_name(KotoIndexedArtist *self, gchar *album_name);

G_END_DECLS
