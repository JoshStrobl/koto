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

typedef enum {
	KOTO_LIBRARY_TYPE_AUDIOBOOK = 1,
	KOTO_LIBRARY_TYPE_MUSIC = 2,
	KOTO_LIBRARY_TYPE_PODCAST = 3
} KotoLibraryType;

G_BEGIN_DECLS

/**
 * Type Definition
 **/

#define KOTO_TYPE_LIBRARY koto_library_get_type()
G_DECLARE_FINAL_TYPE(KotoLibrary, koto_library, KOTO, LIBRARY, GObject);
#define KOTO_IS_LIBRARY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_LIBRARY))

#define KOTO_TYPE_ARTIST koto_artist_get_type()
G_DECLARE_FINAL_TYPE(KotoArtist, koto_artist, KOTO, ARTIST, GObject);
#define KOTO_IS_ARTIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_ARTIST))

#define KOTO_TYPE_ALBUM koto_album_get_type()
G_DECLARE_FINAL_TYPE(KotoAlbum, koto_album, KOTO, ALBUM, GObject);
#define KOTO_IS_ALBUM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_ALBUM))

#define KOTO_TYPE_TRACK koto_track_get_type()
G_DECLARE_FINAL_TYPE(KotoTrack, koto_track, KOTO, TRACK, GObject);
#define KOTO_IS_TRACK(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_TRACK))

/**
 * Library Functions
 **/

KotoLibrary * koto_library_new(const gchar * path);

void koto_library_set_path(
	KotoLibrary * self,
	gchar * path
);

int process_artists(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_albums(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_playlists(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_playlists_tracks(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

int process_tracks(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
);

void read_from_db(KotoLibrary * self);

void start_indexing(KotoLibrary * self);

void index_folder(
	KotoLibrary * self,
	gchar * path,
	guint depth
);

/**
 * Artist Functions
 **/

KotoArtist * koto_artist_new(gchar * path);

KotoArtist * koto_artist_new_with_uuid(const gchar * uuid);

void koto_artist_add_album(
	KotoArtist * self,
	gchar * album_uuid
);

void koto_artist_commit(KotoArtist * self);

guint koto_artist_find_album_with_name(
	gconstpointer * album_data,
	gconstpointer * album_name_data
);

GList * koto_artist_get_albums(KotoArtist * self);

gchar * koto_artist_get_name(KotoArtist * self);

gchar * koto_artist_get_uuid(KotoArtist * self);

void koto_artist_remove_album(
	KotoArtist * self,
	KotoAlbum * album
);

void koto_artist_remove_album_by_name(
	KotoArtist * self,
	gchar * album_name
);

void koto_artist_set_artist_name(
	KotoArtist * self,
	gchar * artist_name
);

void koto_artist_update_path(
	KotoArtist * self,
	gchar * new_path
);

/**
 * Album Functions
 **/

KotoAlbum * koto_album_new(
	KotoArtist * artist,
	const gchar * path
);

KotoAlbum * koto_album_new_with_uuid(
	KotoArtist * artist,
	const gchar * uuid
);

void koto_album_add_track(
	KotoAlbum * self,
	KotoTrack * track
);

void koto_album_commit(KotoAlbum * self);

void koto_album_find_album_art(KotoAlbum * self);

void koto_album_find_tracks(
	KotoAlbum * self,
	magic_t magic_cookie,
	const gchar * path
);

gchar * koto_album_get_album_art(KotoAlbum * self);

gchar * koto_album_get_album_name(KotoAlbum * self);

gchar * koto_album_get_album_uuid(KotoAlbum * self);

GList * koto_album_get_tracks(KotoAlbum * self);

void koto_album_remove_file(
	KotoAlbum * self,
	KotoTrack * track
);

void koto_album_set_album_art(
	KotoAlbum * self,
	const gchar * album_art
);

void koto_album_set_album_name(
	KotoAlbum * self,
	const gchar * album_name
);

void koto_album_set_artist_uuid(
	KotoAlbum * self,
	const gchar * artist_uuid
);

void koto_album_set_as_current_playlist(KotoAlbum * self);

void koto_album_update_path(
	KotoAlbum * self,
	gchar * path
);

gint koto_album_sort_tracks(
	gconstpointer track1_uuid,
	gconstpointer track2_uuid,
	gpointer user_data
);

/**
 * File / Track Functions
 **/

KotoTrack * koto_track_new(
	KotoAlbum * album,
	const gchar * path,
	guint * cd
);

KotoTrack * koto_track_new_with_uuid(const gchar * uuid);

void koto_track_commit(KotoTrack * self);

GVariant * koto_track_get_metadata_vardict(KotoTrack * self);

gchar * koto_track_get_uuid(KotoTrack * self);

void koto_track_parse_name(KotoTrack * self);

void koto_track_remove_from_playlist(
	KotoTrack * self,
	gchar * playlist_uuid
);

void koto_track_save_to_playlist(
	KotoTrack * self,
	gchar * playlist_uuid,
	gint current
);

void koto_track_set_file_name(
	KotoTrack * self,
	gchar * new_file_name
);

void koto_track_set_cd(
	KotoTrack * self,
	guint cd
);

void koto_track_set_parsed_name(
	KotoTrack * self,
	gchar * new_parsed_name
);

void koto_track_set_position(
	KotoTrack * self,
	guint pos
);

void koto_track_update_metadata(KotoTrack * self);

void koto_track_update_path(
	KotoTrack * self,
	const gchar * new_path
);

G_END_DECLS
