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
#include <glib-2.0/glib.h>
#include <glib-2.0/gio/gio.h>
#include <magic.h>
#include <toml.h>
#include "misc-types.h"

typedef enum {
	KOTO_LIBRARY_TYPE_AUDIOBOOK = 1,
	KOTO_LIBRARY_TYPE_MUSIC = 2,
	KOTO_LIBRARY_TYPE_PODCAST = 3,
	KOTO_LIBRARY_TYPE_UNKNOWN = 4
} KotoLibraryType;

G_BEGIN_DECLS

/**
 * Type Definition
 **/

#define KOTO_TYPE_LIBRARY koto_library_get_type()
#define KOTO_LIBRARY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), KOTO_TYPE_LIBRARY, KotoLibrary))
typedef struct _KotoLibrary KotoLibrary;
typedef struct _KotoLibraryClass KotoLibraryClass;

GLIB_AVAILABLE_IN_ALL
GType koto_library_get_type(void) G_GNUC_CONST;

#define KOTO_IS_LIBRARY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_LIBRARY))

#define KOTO_TYPE_ARTIST koto_artist_get_type()
#define KOTO_ARTIST(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), KOTO_TYPE_ARTIST, KotoArtist))
typedef struct _KotoArtist KotoArtist;
typedef struct _KotoArtistClass KotoArtistClass;

GLIB_AVAILABLE_IN_ALL
GType koto_artist_get_type(void) G_GNUC_CONST;

#define KOTO_IS_ARTIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_ARTIST))

#define KOTO_TYPE_ALBUM koto_album_get_type()
#define KOTO_ALBUM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), KOTO_TYPE_ALBUM, KotoAlbum))
typedef struct _KotoAlbum KotoAlbum;
typedef struct _KotoAlbumClass KotoAlbumClass;

GLIB_AVAILABLE_IN_ALL
GType koto_album_get_type(void) G_GNUC_CONST;

#define KOTO_IS_ALBUM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_ALBUM))

#define KOTO_TYPE_TRACK koto_track_get_type()
G_DECLARE_FINAL_TYPE(KotoTrack, koto_track, KOTO, TRACK, GObject);
#define KOTO_IS_TRACK(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_TRACK))

/**
 * Library Functions
 **/

KotoLibrary * koto_library_new(
	KotoLibraryType type,
	const gchar * storage_uuid,
	const gchar * path
);

KotoLibrary * koto_library_new_from_toml_table(toml_table_t * lib_datum);

gchar * koto_library_get_path(KotoLibrary * self);

gchar * koto_library_get_relative_path_to_file(
	KotoLibrary * self,
	gchar * full_path
);

gchar * koto_library_get_storage_uuid(KotoLibrary * self);

KotoLibraryType koto_library_get_lib_type(KotoLibrary * self);

gchar * koto_library_get_uuid(KotoLibrary * self);

void koto_library_index(KotoLibrary * self);

gboolean koto_library_is_available(KotoLibrary * self);

gchar * koto_library_get_storage_uuid(KotoLibrary * self);

void koto_library_set_name(
	KotoLibrary * self,
	gchar * library_name
);

void koto_library_set_path(
	KotoLibrary * self,
	gchar * path
);

void koto_library_set_storage_uuid(
	KotoLibrary * self,
	gchar * uuid
);

gchar * koto_library_to_config_string(KotoLibrary * self);

KotoLibraryType koto_library_type_from_string(gchar * t);

gchar * koto_library_type_to_string(KotoLibraryType t);

void index_folder(
	KotoLibrary * self,
	gchar * path,
	guint depth
);

void index_file(
	KotoLibrary * lib,
	const gchar * path
);

/**
 * Artist Functions
 **/

KotoArtist * koto_artist_new(gchar * artist_name);

KotoArtist * koto_artist_new_with_uuid(const gchar * uuid);

void koto_artist_add_album(
	KotoArtist * self,
	KotoAlbum * album
);

void koto_artist_add_track(
	KotoArtist * self,
	KotoTrack * track
);

void koto_artist_apply_model(
	KotoArtist * self,
	KotoPreferredAlbumSortType model
);

void koto_artist_commit(KotoArtist * self);

GQueue * koto_artist_get_albums(KotoArtist * self);

GListStore * koto_artist_get_albums_store(KotoArtist * self);

KotoAlbum * koto_artist_get_album_by_name(
	KotoArtist * self,
	gchar * album_name
);

gchar * koto_artist_get_name(KotoArtist * self);

gchar * koto_artist_get_path(KotoArtist * self);

GList * koto_artist_get_tracks(KotoArtist * self);

gchar * koto_artist_get_uuid(KotoArtist * self);

KotoLibraryType koto_artist_get_lib_type(KotoArtist * self);

gint koto_artist_model_sort_albums(
	gconstpointer first_item,
	gconstpointer second_item,
	gpointer user_data
);

void koto_artist_remove_album(
	KotoArtist * self,
	KotoAlbum * album
);

void koto_artist_remove_track(
	KotoArtist * self,
	KotoTrack * track
);

void koto_artist_set_artist_name(
	KotoArtist * self,
	gchar * artist_name
);

void koto_artist_set_as_finalized(KotoArtist * self);

void koto_artist_set_path(
	KotoArtist * self,
	KotoLibrary * lib,
	const gchar * fixed_path,
	gboolean should_commit
);

/**
 * Album Functions
 **/

KotoAlbum * koto_album_new(gchar * artist_uuid);

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

gchar * koto_album_get_art(KotoAlbum * self);

gchar * koto_album_get_artist_uuid(KotoAlbum * self);

gchar * koto_album_get_description(KotoAlbum * self);

GList * koto_album_get_genres(KotoAlbum * self);

KotoLibraryType koto_album_get_lib_type(KotoAlbum * self);

gchar * koto_album_get_name(KotoAlbum * self);

gchar * koto_album_get_narrator(KotoAlbum * self);

gchar * koto_album_get_path(KotoAlbum * self);

GListStore * koto_album_get_store(KotoAlbum * self);

gchar * koto_album_get_uuid(KotoAlbum * self);

guint64 koto_album_get_year(KotoAlbum * self);

void koto_album_mark_as_finalized(KotoAlbum * self);

void koto_album_remove_track(
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

void koto_album_set_description(
	KotoAlbum * self,
	const char * description
);

void koto_album_set_as_current_playlist(KotoAlbum * self);

void koto_album_set_narrator(
	KotoAlbum * self,
	const char * narrator
);

void koto_album_set_path(
	KotoAlbum * self,
	KotoLibrary * lib,
	const gchar * fixed_path
);

void koto_album_set_preparsed_genres(
	KotoAlbum * self,
	gchar * genrelist
);

void koto_album_set_uuid(
	KotoAlbum * self,
	const gchar * uuid
);

void koto_album_set_year(
	KotoAlbum * self,
	guint64 year
);

/**
 * File / Track Functions
 **/

KotoTrack * koto_track_new(
	const gchar * artist_uuid,
	const gchar * album_uuid,
	const gchar * parsed_name,
	guint cd
);

KotoTrack * koto_track_new_with_uuid(const gchar * uuid);

void koto_track_commit(KotoTrack * self);

gchar * koto_track_get_description(KotoTrack * self);

guint koto_track_get_disc_number(KotoTrack * self);

guint64 koto_track_get_duration(KotoTrack * self);

GList * koto_track_get_genres(KotoTrack * self);

GVariant * koto_track_get_metadata_vardict(KotoTrack * self);

gchar * koto_track_get_path(KotoTrack * self);

gchar * koto_track_get_name(KotoTrack * self);

gchar * koto_track_get_narrator(KotoTrack * self);

guint64 koto_track_get_playback_position(KotoTrack * self);

guint64 koto_track_get_position(KotoTrack * self);

gchar * koto_track_get_uniqueish_key(KotoTrack * self);

gchar * koto_track_get_uuid(KotoTrack * self);

guint64 koto_track_get_year(KotoTrack * self);

void koto_track_remove_from_playlist(
	KotoTrack * self,
	gchar * playlist_uuid
);

void koto_track_set_album_uuid(
	KotoTrack * self,
	const gchar * album_uuid
);

void koto_track_save_to_playlist(
	KotoTrack * self,
	gchar * playlist_uuid
);

void koto_track_set_cd(
	KotoTrack * self,
	guint cd
);

void koto_track_set_description(
	KotoTrack * self,
	const gchar * description
);

void koto_track_set_duration(
	KotoTrack * self,
	guint64 duration
);

void koto_track_set_file_name(
	KotoTrack * self,
	gchar * new_file_name
);

void koto_track_set_genres(
	KotoTrack * self,
	char * genrelist
);

void koto_track_set_narrator(
	KotoTrack * self,
	const gchar * narrator
);

void koto_track_set_parsed_name(
	KotoTrack * self,
	gchar * new_parsed_name
);

void koto_track_set_path(
	KotoTrack * self,
	KotoLibrary * lib,
	gchar * fixed_path
);

void koto_track_set_playback_position(
	KotoTrack * self,
	guint64 position
);

void koto_track_set_position(
	KotoTrack * self,
	guint64 pos
);

void koto_track_set_preparsed_genres(
	KotoTrack * self,
	gchar * genrelist
);

void koto_track_set_year(
	KotoTrack * self,
	guint64 year
);

void koto_track_update_metadata(KotoTrack * self);

G_END_DECLS
