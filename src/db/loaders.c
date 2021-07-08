/* loaders.c
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

#include "cartographer.h"
#include "db.h"
#include "loaders.h"
#include "../indexer/structs.h"
#include "../koto-utils.h"

extern KotoCartographer * koto_maps;
extern sqlite3 * koto_db;

int process_artists(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
) {
	(void) data;
	(void) num_columns;
	(void) column_names; // Don't need any of the params

	gchar * artist_uuid = g_strdup(koto_utils_unquote_string(fields[0])); // First column is UUID
	gchar * artist_name = g_strdup(koto_utils_unquote_string(fields[1])); // Second column is artist name

	KotoArtist * artist = koto_artist_new_with_uuid(artist_uuid); // Create our artist with the UUID

	g_object_set(
		artist,
		"name",
		artist_name,          // Set name
		NULL);

	int artist_paths = sqlite3_exec(koto_db, g_strdup_printf("SELECT * FROM libraries_artists WHERE artist_id=\"%s\"", artist_uuid), process_artist_paths, artist, NULL); // Process all the paths for this given artist

	if (artist_paths != SQLITE_OK) { // Failed to get our artists_paths
		g_critical("Failed to read our paths for this artist: %s", sqlite3_errmsg(koto_db));
		return 1;
	}

	koto_cartographer_add_artist(koto_maps, artist); // Add the artist to our global cartographer

	int albums_rc = sqlite3_exec(koto_db, g_strdup_printf("SELECT * FROM albums WHERE artist_id=\"%s\"", artist_uuid), process_albums, artist, NULL); // Process our albums

	if (albums_rc != SQLITE_OK) { // Failed to get our albums
		g_critical("Failed to read our albums: %s", sqlite3_errmsg(koto_db));
		return 1;
	}

	koto_artist_set_as_finalized(artist); // Indicate it is finalized

	int tracks_rc = sqlite3_exec(koto_db, g_strdup_printf("SELECT * FROM tracks WHERE artist_id=\"%s\"", artist_uuid), process_tracks, NULL, NULL); // Process our tracks by artist uuid

	if (tracks_rc != SQLITE_OK) { // Failed to get our tracks
		g_critical("Failed to read our tracks: %s", sqlite3_errmsg(koto_db));
		return 1;
	}

	g_free(artist_uuid);
	g_free(artist_name);

	return 0;
}

int process_artist_paths(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
) {
	(void) num_columns;
	(void) column_names; // Don't need these

	KotoArtist * artist = (KotoArtist*) data;

	gchar * library_uuid = g_strdup(koto_utils_unquote_string(fields[0]));
	gchar * relative_path = g_strdup(koto_utils_unquote_string(fields[2]));

	KotoLibrary * lib = koto_cartographer_get_library_by_uuid(koto_maps, library_uuid); // Get the library for this artist

	if (!KOTO_IS_LIBRARY(lib)) { // Failed to get the library for this UUID
		return 0;
	}

	koto_artist_set_path(artist, lib, relative_path, FALSE); // Add the relative path from the db for this artist and lib to the Artist, do not commit

	return 0;
}

int process_albums(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
) {
	(void) num_columns;
	(void) column_names; // Don't need these

	KotoArtist * artist = (KotoArtist*) data;

	gchar * album_uuid = g_strdup(koto_utils_unquote_string(fields[0]));
	gchar * artist_uuid = g_strdup(koto_utils_unquote_string(fields[1]));
	gchar * album_name = g_strdup(koto_utils_unquote_string(fields[2]));
	gchar * album_art = (fields[3] != NULL) ? g_strdup(koto_utils_unquote_string(fields[3])) : NULL;
	gchar * album_genres = (fields[4] != NULL) ? g_strdup(koto_utils_unquote_string(fields[4])) : NULL;

	KotoAlbum * album = koto_album_new_with_uuid(artist, album_uuid); // Create our album

	g_object_set(
		album,
		"name",
		album_name,         // Set name
		"art-path",
		album_art,             // Set art path if any
		"preparsed-genres",
		album_genres,
		NULL
	);

	koto_cartographer_add_album(koto_maps, album); // Add the album to our global cartographer
	koto_artist_add_album(artist, album); // Add the album

	g_free(album_uuid);
	g_free(artist_uuid);
	g_free(album_name);
	g_free(album_genres);

	if (album_art != NULL) {
		g_free(album_art);
	}

	return 0;
}

int process_playlists(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
) {
	(void) data;
	(void) num_columns;
	(void) column_names; // Don't need any of the params

	gchar * playlist_uuid = g_strdup(koto_utils_unquote_string(fields[0])); // First column is UUID
	gchar * playlist_name = g_strdup(koto_utils_unquote_string(fields[1])); // Second column is playlist name
	gchar * playlist_art_path = g_strdup(koto_utils_unquote_string(fields[2])); // Third column is any art path

	KotoPlaylist * playlist = koto_playlist_new_with_uuid(playlist_uuid); // Create a playlist using the existing UUID

	koto_playlist_set_name(playlist, playlist_name); // Add the playlist name
	koto_playlist_set_artwork(playlist, playlist_art_path); // Add the playlist art path

	koto_cartographer_add_playlist(koto_maps, playlist); // Add to cartographer

	int playlist_tracks_rc = sqlite3_exec(koto_db, g_strdup_printf("SELECT * FROM playlist_tracks WHERE playlist_id=\"%s\" ORDER BY position ASC", playlist_uuid), process_playlists_tracks, playlist, NULL); // Process our playlist tracks

	if (playlist_tracks_rc != SQLITE_OK) { // Failed to get our playlist tracks
		g_critical("Failed to read our playlist tracks: %s", sqlite3_errmsg(koto_db));
		return 1;
	}

	koto_playlist_mark_as_finalized(playlist); // Mark as finalized since loading should be complete

	g_free(playlist_uuid);
	g_free(playlist_name);
	g_free(playlist_art_path);

	return 0;
}

int process_playlists_tracks(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
) {
	(void) data;
	(void) num_columns;
	(void) column_names; // Don't need these

	gchar * playlist_uuid = g_strdup(koto_utils_unquote_string(fields[1]));
	gchar * track_uuid = g_strdup(koto_utils_unquote_string(fields[2]));
	gboolean current = g_strcmp0(koto_utils_unquote_string(fields[3]), "0");

	KotoPlaylist * playlist = koto_cartographer_get_playlist_by_uuid(koto_maps, playlist_uuid); // Get the playlist
	KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, track_uuid); // Get the track

	if (!KOTO_IS_PLAYLIST(playlist)) {
		goto freeforret;
	}

	koto_playlist_add_track(playlist, track, current, FALSE); // Add the track to the playlist but don't re-commit to the table

freeforret:
	g_free(playlist_uuid);
	g_free(track_uuid);

	return 0;
}

int process_tracks(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
) {
	(void) data;
	(void) num_columns;
	(void) column_names; // Don't need these

	gchar * track_uuid = g_strdup(koto_utils_unquote_string(fields[0]));
	gchar * artist_uuid = g_strdup(koto_utils_unquote_string(fields[1]));
	gchar * album_uuid = g_strdup(koto_utils_unquote_string(fields[2]));
	gchar * name = g_strdup(koto_utils_unquote_string(fields[3]));
	guint * disc_num = (guint*) g_ascii_strtoull(fields[4], NULL, 10);
	guint64 * position = (guint64*) g_ascii_strtoull(fields[5], NULL, 10);
	guint64 * duration = (guint64*) g_ascii_strtoull(fields[6], NULL, 10);
	gchar * genres = g_strdup(koto_utils_unquote_string(fields[7]));

	KotoTrack * track = koto_track_new_with_uuid(track_uuid); // Create our file

	g_object_set(
		track,
		"artist-uuid",
		artist_uuid,
		"album-uuid",
		album_uuid,
		"parsed-name",
		name,
		"cd",
		disc_num,
		"position",
		position,
		"duration",
		duration,
		"preparsed-genres",
		genres,
		NULL
	);

	g_free(name);

	int track_paths = sqlite3_exec(koto_db, g_strdup_printf("SELECT id, path FROM libraries_tracks WHERE track_id=\"%s\"", track_uuid), process_track_paths, track, NULL); // Process all pathes associated with the track

	if (track_paths != SQLITE_OK) { // Failed to read the paths
		g_warning("Failed to read paths associated with track %s: %s", track_uuid, sqlite3_errmsg(koto_db));
		g_free(track_uuid);
		g_free(artist_uuid);
		g_free(album_uuid);
		return 1;
	}

	KotoArtist * artist = koto_cartographer_get_artist_by_uuid(koto_maps, artist_uuid); // Get the artist
	koto_artist_add_track(artist, track); // Add the track for the artist

	if (koto_utils_is_string_valid(album_uuid)) { // If we have an album UUID
		KotoAlbum * album = koto_cartographer_get_album_by_uuid(koto_maps, album_uuid); // Attempt to get album

		if (KOTO_IS_ALBUM(album)) { // This is an album
			koto_album_add_track(album, track); // Add the track
		}
	}

	g_free(track_uuid);
	g_free(artist_uuid);
	g_free(album_uuid);

	return 0;
}

int process_track_paths(
	void * data,
	int num_columns,
	char ** fields,
	char ** column_names
) {
	KotoTrack * track = (KotoTrack*) data;
	(void) num_columns;
	(void) column_names; // Don't need these

	KotoLibrary * library = koto_cartographer_get_library_by_uuid(koto_maps, koto_utils_unquote_string(fields[0]));

	if (!KOTO_IS_LIBRARY(library)) { // Not a library
		return 1;
	}

	koto_track_set_path(track, library, koto_utils_unquote_string(fields[1]));
	return 0;
}


void read_from_db() {
	int artists_rc = sqlite3_exec(koto_db, "SELECT * FROM artists", process_artists, NULL, NULL); // Process our artists

	if (artists_rc != SQLITE_OK) { // Failed to get our artists
		g_critical("Failed to read our artists: %s", sqlite3_errmsg(koto_db));
		return;
	}

	int playlist_rc = sqlite3_exec(koto_db, "SELECT * FROM playlist_meta", process_playlists, NULL, NULL); // Process our playlists

	if (playlist_rc != SQLITE_OK) { // Failed to get our playlists
		g_critical("Failed to read our playlists: %s", sqlite3_errmsg(koto_db));
		return;
	}
}