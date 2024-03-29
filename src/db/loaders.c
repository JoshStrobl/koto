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
#include "../indexer/album-playlist-funcs.h"
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

	gchar * artist_uuid = g_strdup(koto_utils_string_unquote(fields[0])); // First column is UUID
	gchar * artist_name = g_strdup(koto_utils_string_unquote(fields[1])); // Second column is artist name

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

	int tracks_rc = sqlite3_exec(koto_db, g_strdup_printf("SELECT * FROM tracks WHERE artist_id=\"%s\" AND album_id=''", artist_uuid), process_tracks, NULL, NULL); // Load all tracks for an artist that are NOT in an album (e.g. artists without albums)

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

	gchar * library_uuid = g_strdup(koto_utils_string_unquote(fields[0]));
	gchar * relative_path = g_strdup(koto_utils_string_unquote(fields[2]));

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

	gchar * album_uuid = g_strdup(koto_utils_string_unquote(fields[0]));
	gchar * artist_uuid = g_strdup(koto_utils_string_unquote(fields[1]));
	gchar * album_name = g_strdup(koto_utils_string_unquote(fields[2]));
	gchar * album_description = (fields[3] != NULL) ? g_strdup(koto_utils_string_unquote(fields[3])) : NULL;
	gchar * album_narrator = (fields[4] != NULL) ? g_strdup(koto_utils_string_unquote(fields[4])) : NULL;
	gchar * album_art = (fields[5] != NULL) ? g_strdup(koto_utils_string_unquote(fields[5])) : NULL;
	gchar * album_genres = (fields[6] != NULL) ? g_strdup(koto_utils_string_unquote(fields[6])) : NULL;
	guint64 * album_year = (guint64*) g_ascii_strtoull(fields[7], NULL, 10);

	KotoAlbum * album = koto_album_new_with_uuid(artist, album_uuid); // Create our album

	g_object_set(
		album,
		"name",
		album_name,         // Set name
		"description",
		album_description,
		"narrator",
		album_narrator,
		"art-path",
		album_art,             // Set art path if any
		"preparsed-genres",
		album_genres,
		"year",
		album_year,
		NULL
	);

	koto_cartographer_add_album(koto_maps, album); // Add the album to our global cartographer
	koto_artist_add_album(artist, album); // Add the album

	int tracks_rc = sqlite3_exec(koto_db, g_strdup_printf("SELECT * FROM tracks WHERE album_id=\"%s\"", album_uuid), process_tracks, NULL, NULL); // Process all the tracks for this specific album

	if (tracks_rc != SQLITE_OK) { // Failed to get our tracks
		g_critical("Failed to read our tracks: %s", sqlite3_errmsg(koto_db));
		return 1;
	}

	koto_album_mark_as_finalized(album); // Mark the album as finalized now that all tracks have been loaded, allowing our internal album playlist to re-sort itself

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

	gchar * playlist_uuid = g_strdup(koto_utils_string_unquote(fields[0])); // First column is UUID
	gchar * playlist_name = g_strdup(koto_utils_string_unquote(fields[1])); // Second column is playlist name
	gchar * playlist_art_path = g_strdup(koto_utils_string_unquote(fields[2])); // Third column is any art path
	guint64 playlist_preferred_sort = g_ascii_strtoull(koto_utils_string_unquote(fields[3]), NULL, 10); // Fourth column is preferred model which is an int
	gchar * playlist_album_id = g_strdup(koto_utils_string_unquote(fields[4])); // Fifth column is any album ID
	gchar * playlist_current_track_id = g_strdup(koto_utils_string_unquote(fields[5])); // Sixth column is any track ID
	guint64 playlist_track_current_playback_pos = g_ascii_strtoull(koto_utils_string_unquote(fields[6]), NULL, 10); // Seventh column is any playback position for the track

	KotoPreferredPlaylistSortType sort_type = (KotoPreferredPlaylistSortType) playlist_preferred_sort;

	KotoPlaylist * playlist = NULL;

	gboolean for_album = FALSE;

	if (koto_utils_string_is_valid(playlist_album_id)) { // Album UUID is set
		KotoAlbum * album = koto_cartographer_get_album_by_uuid(koto_maps, playlist_album_id); // Get the album based on the album ID set for the playlist

		if (KOTO_IS_ALBUM(album)) { // Is an album
			playlist = koto_album_get_playlist(album); // Get the playlist
			for_album = TRUE;
		}
	} else {
		playlist = koto_playlist_new_with_uuid(playlist_uuid); // Create a playlist using the existing UUID
		koto_playlist_apply_model(playlist, sort_type); // Set the model based on what is stored
		koto_cartographer_add_playlist(koto_maps, playlist); // Add to cartographer
	}

	if (!KOTO_IS_PLAYLIST(playlist)) { // Not a playlist yet, in this scenario it is likely the album no longer exists
		goto free;
	}

	g_object_set(
		playlist,
		"name",
		playlist_name,
		"art-path",
		playlist_art_path,
		"ephemeral",
		for_album,
		NULL
	);

	if (!for_album) { // Isn't for an album
		int playlist_tracks_rc = sqlite3_exec(koto_db, g_strdup_printf("SELECT * FROM playlist_tracks WHERE playlist_id=\"%s\" ORDER BY position ASC", playlist_uuid), process_playlists_tracks, playlist, NULL); // Process our playlist tracks

		if (playlist_tracks_rc != SQLITE_OK) { // Failed to get our playlist tracks
			g_critical("Failed to read our playlist tracks: %s", sqlite3_errmsg(koto_db));
			goto free;
		}
	}

	if (koto_utils_string_is_valid(playlist_current_track_id)) { // If we have a track UUID (probably)
		KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, playlist_current_track_id); // Get the track UUID

		if (KOTO_IS_TRACK(track)) { // If this is a track
			koto_track_set_playback_position(track, playlist_track_current_playback_pos); // Set the playback position of the track
			koto_playlist_set_track_as_current(playlist, playlist_current_track_id); // Ensure we have this track set as the current one in the playlist
		}
	}

	koto_playlist_mark_as_finalized(playlist); // Mark as finalized since loading should be complete

free:
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

	gchar * playlist_uuid = g_strdup(koto_utils_string_unquote(fields[1]));
	gchar * track_uuid = g_strdup(koto_utils_string_unquote(fields[2]));

	KotoPlaylist * playlist = koto_cartographer_get_playlist_by_uuid(koto_maps, playlist_uuid); // Get the playlist
	KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, track_uuid); // Get the track

	if (!KOTO_IS_PLAYLIST(playlist)) {
		goto freeforret;
	}

	koto_playlist_add_track(playlist, track, FALSE, FALSE); // Add the track to the playlist but don't re-commit to the table

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

	gchar * track_uuid = g_strdup(koto_utils_string_unquote(fields[0]));

	KotoTrack * existing_track = koto_cartographer_get_track_by_uuid(koto_maps, track_uuid);

	if (KOTO_IS_TRACK(existing_track)) { // Already have track
		g_free(track_uuid);
		return 0;
	}

	gchar * artist_uuid = g_strdup(koto_utils_string_unquote(fields[1]));
	gchar * album_uuid = g_strdup(koto_utils_string_unquote(fields[2]));
	gchar * name = g_strdup(koto_utils_string_unquote(fields[3]));
	guint * disc_num = (guint*) g_ascii_strtoull(fields[4], NULL, 10);
	guint64 * position = (guint64*) g_ascii_strtoull(fields[5], NULL, 10);
	guint64 * duration = (guint64*) g_ascii_strtoull(fields[6], NULL, 10);
	gchar * genres = g_strdup(koto_utils_string_unquote(fields[7]));

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

	koto_cartographer_add_track(koto_maps, track); // Add the track to cartographer if necessary

	KotoArtist * artist = koto_cartographer_get_artist_by_uuid(koto_maps, artist_uuid); // Get the artist
	koto_artist_add_track(artist, track); // Add the track for the artist

	if (koto_utils_string_is_valid(album_uuid)) { // If we have an album UUID
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

	KotoLibrary * library = koto_cartographer_get_library_by_uuid(koto_maps, koto_utils_string_unquote(fields[0]));

	if (!KOTO_IS_LIBRARY(library)) { // Not a library
		return 1;
	}

	koto_track_set_path(track, library, koto_utils_string_unquote(fields[1]));
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