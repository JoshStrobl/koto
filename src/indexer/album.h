/* album.h
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

#define KOTO_INDEXED_ALBUM_TYPE koto_indexed_album_get_type()
G_DECLARE_FINAL_TYPE (KotoIndexedAlbum, koto_indexed_album, KOTO, INDEXED_ALBUM, GObject);

KotoIndexedAlbum* koto_indexed_album_new(const gchar *path);

void add_song(KotoIndexedAlbum *self, KotoIndexedFile *file);
KotoIndexedFile** get_songs(KotoIndexedAlbum *self);
void remove_song(KotoIndexedAlbum *self, KotoIndexedFile *file);
void remove_song_by_name(KotoIndexedAlbum *self, gchar *file_name);

G_END_DECLS
