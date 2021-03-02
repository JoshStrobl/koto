/* structs.h
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
#include <magic.h>

G_BEGIN_DECLS

/**
 * Type Definition
**/

#define KOTO_TYPE_INDEXED_LIBRARY koto_indexed_library_get_type()
G_DECLARE_FINAL_TYPE(KotoIndexedLibrary, koto_indexed_library, KOTO, INDEXED_LIBRARY, GObject);

#define KOTO_TYPE_INDEXED_ARTIST koto_indexed_artist_get_type()
G_DECLARE_FINAL_TYPE (KotoIndexedArtist, koto_indexed_artist, KOTO, INDEXED_ARTIST, GObject);

#define KOTO_TYPE_INDEXED_ALBUM koto_indexed_album_get_type()
G_DECLARE_FINAL_TYPE (KotoIndexedAlbum, koto_indexed_album, KOTO, INDEXED_ALBUM, GObject);

#define KOTO_TYPE_INDEXED_FILE koto_indexed_file_get_type()
G_DECLARE_FINAL_TYPE(KotoIndexedFile, koto_indexed_file, KOTO, INDEXED_FILE, GObject);

/**
 * Library Functions
**/

KotoIndexedLibrary* koto_indexed_library_new(const gchar *path);

void koto_indexed_library_add_artist(KotoIndexedLibrary *self, KotoIndexedArtist *artist);
KotoIndexedArtist* koto_indexed_library_get_artist(KotoIndexedLibrary *self, gchar* artist_name);
GHashTable* koto_indexed_library_get_artists(KotoIndexedLibrary *self);
void koto_indexed_library_remove_artist(KotoIndexedLibrary *self, KotoIndexedArtist *artist);
void start_indexing(KotoIndexedLibrary *self);
void index_folder(KotoIndexedLibrary *self, gchar *path, guint depth);

/**
 * Artist Functions
**/

KotoIndexedArtist* koto_indexed_artist_new(const gchar *path);
KotoIndexedArtist* koto_indexed_artist_new_with_uuid(const gchar *uuid);

void koto_indexed_artist_add_album(KotoIndexedArtist *self, KotoIndexedAlbum *album);
void koto_indexed_artist_commit(KotoIndexedArtist *self);
guint koto_indexed_artist_find_album_with_name(gconstpointer *album_data, gconstpointer *album_name_data);
GList* koto_indexed_artist_get_albums(KotoIndexedArtist *self);
void koto_indexed_artist_remove_album(KotoIndexedArtist *self, KotoIndexedAlbum *album);
void koto_indexed_artist_remove_album_by_name(KotoIndexedArtist *self, gchar *album_name);
void koto_indexed_artist_set_artist_name(KotoIndexedArtist *self, const gchar *artist_name);
void koto_indexed_artist_update_path(KotoIndexedArtist *self, const gchar *new_path);
void output_artists(gpointer artist_key, gpointer artist_ptr, gpointer data);

/**
 * Album Functions
**/

KotoIndexedAlbum* koto_indexed_album_new(KotoIndexedArtist *artist, const gchar *path);
KotoIndexedAlbum* koto_indexed_album_new_with_uuid(KotoIndexedArtist *artist, const gchar *uuid);

void koto_indexed_album_add_file(KotoIndexedAlbum *self, KotoIndexedFile *file);
void koto_indexed_album_commit(KotoIndexedAlbum *self);
void koto_indexed_album_find_album_art(KotoIndexedAlbum *self);
void koto_indexed_album_find_tracks(KotoIndexedAlbum *self, magic_t magic_cookie, const gchar *path);
gchar* koto_indexed_album_get_album_art(KotoIndexedAlbum *self);
GList* koto_indexed_album_get_files(KotoIndexedAlbum *self);
void koto_indexed_album_remove_file(KotoIndexedAlbum *self, KotoIndexedFile *file);
void koto_indexed_album_remove_file_by_name(KotoIndexedAlbum *self, const gchar *file_name);
void koto_indexed_album_set_album_art(KotoIndexedAlbum *self, const gchar *album_art);
void koto_indexed_album_set_album_name(KotoIndexedAlbum *self, const gchar *album_name);
void koto_indexed_album_update_path(KotoIndexedAlbum *self, const gchar *path);

/**
 * File / Track Functions
**/

KotoIndexedFile* koto_indexed_file_new(KotoIndexedAlbum *album, const gchar *path, guint *cd);
KotoIndexedFile* koto_indexed_file_new_with_uuid(const gchar *uuid);

void koto_indexed_file_commit(KotoIndexedFile *self);
void koto_indexed_file_parse_name(KotoIndexedFile *self);
void koto_indexed_file_set_file_name(KotoIndexedFile *self, gchar *new_file_name);
void koto_indexed_file_set_cd(KotoIndexedFile *self, guint cd);
void koto_indexed_file_set_parsed_name(KotoIndexedFile *self, gchar *new_parsed_name);
void koto_indexed_file_set_position(KotoIndexedFile *self, guint pos);
void koto_indexed_file_update_metadata(KotoIndexedFile *self);
void koto_indexed_file_update_path(KotoIndexedFile *self, const gchar *new_path);

G_END_DECLS
