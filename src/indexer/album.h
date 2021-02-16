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
#include "file.h"

G_BEGIN_DECLS

#define KOTO_TYPE_INDEXED_ALBUM koto_indexed_album_get_type()
G_DECLARE_FINAL_TYPE (KotoIndexedAlbum, koto_indexed_album, KOTO, INDEXED_ALBUM, GObject);

KotoIndexedAlbum* koto_indexed_album_new(const gchar *path);

void koto_indexed_album_add_file(KotoIndexedAlbum *self, KotoIndexedFile *file);
gchar* koto_indexed_album_get_album_art(KotoIndexedAlbum *self);
GList* koto_indexed_album_get_files(KotoIndexedAlbum *self);
void koto_indexed_album_remove_file(KotoIndexedAlbum *self, KotoIndexedFile *file);
void koto_indexed_album_remove_file_by_name(KotoIndexedAlbum *self, const gchar *file_name);
void koto_indexed_album_set_album_art(KotoIndexedAlbum *self, const gchar *album_art);
void koto_indexed_album_set_album_name(KotoIndexedAlbum *self, const gchar *album_name);
void koto_indexed_album_update_path(KotoIndexedAlbum *self, const gchar *path);

G_END_DECLS
