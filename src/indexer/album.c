/* album.c
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

#include <glib-2.0/glib.h>
#include <magic.h>
#include <sqlite3.h>
#include <stdio.h>
#include "../db/cartographer.h"
#include "../db/db.h"
#include "../playlist/current.h"
#include "../playlist/playlist.h"
#include "../koto-utils.h"
#include "album-playlist-funcs.h"
#include "track-helpers.h"

extern KotoCartographer * koto_maps;
extern KotoCurrentPlaylist * current_playlist;
extern magic_t magic_cookie;
extern sqlite3 * koto_db;

enum {
	PROP_0,
	PROP_UUID,
	PROP_DO_INITIAL_INDEX,
	PROP_NAME,
	PROP_ART_PATH,
	PROP_ARTIST_UUID,
	PROP_ALBUM_PREPARED_GENRES,
	PROP_DESCRIPTION,
	PROP_NARRATOR,
	PROP_YEAR,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};

enum {
	SIGNAL_TRACK_ADDED,
	SIGNAL_TRACK_REMOVED,
	N_SIGNALS
};

static guint album_signals[N_SIGNALS] = {
	0
};

struct _KotoAlbum {
	GObject parent_instance;
	gchar * uuid;

	gchar * name;
	guint64 year;
	gchar * description;
	gchar * narrator;
	gchar * art_path;
	gchar * artist_uuid;

	GList * genres;

	KotoPlaylist * playlist;
	GHashTable * paths;

	gboolean has_album_art;
	gboolean do_initial_index;
	gboolean finalized;
};

struct _KotoAlbumClass {
	GObjectClass parent_class;

	void (* track_added) (
		KotoAlbum * album,
		KotoTrack * track
	);
	void (* track_removed) (
		KotoAlbum * album,
		KotoTrack * track
	);
};

G_DEFINE_TYPE(KotoAlbum, koto_album, G_TYPE_OBJECT);

static void koto_album_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_album_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_album_class_init(KotoAlbumClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_album_set_property;
	gobject_class->get_property = koto_album_get_property;

	props[PROP_UUID] = g_param_spec_string(
		"uuid",
		"UUID to Album in database",
		"UUID to Album in database",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_DO_INITIAL_INDEX] = g_param_spec_boolean(
		"do-initial-index",
		"Do an initial indexing operating instead of pulling from the database",
		"Do an initial indexing operating instead of pulling from the database",
		FALSE,
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_NAME] = g_param_spec_string(
		"name",
		"Name",
		"Name of Album",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_ART_PATH] = g_param_spec_string(
		"art-path",
		"Path to Artwork",
		"Path to Artwork",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_ARTIST_UUID] = g_param_spec_string(
		"artist-uuid",
		"UUID of Artist associated with Album",
		"UUID of Artist associated with Album",
		NULL,
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_ALBUM_PREPARED_GENRES] = g_param_spec_string(
		"preparsed-genres",
		"Preparsed Genres",
		"Preparsed Genres",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	props[PROP_DESCRIPTION] = g_param_spec_string(
		"description",
		"Description of Album, typically for an audiobook or podcast",
		"Description of Album, typically for an audiobook or podcast",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	props[PROP_NARRATOR] = g_param_spec_string(
		"narrator",
		"Narrator of audiobook",
		"Narrator of audiobook",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	props[PROP_YEAR] = g_param_spec_uint64(
		"year",
		"Year",
		"Year of release",
		0,
		G_MAXSHORT,
		0,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);

	album_signals[SIGNAL_TRACK_ADDED] = g_signal_new(
		"track-added",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoAlbumClass, track_added),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_TRACK
	);

	album_signals[SIGNAL_TRACK_REMOVED] = g_signal_new(
		"track-removed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoAlbumClass, track_removed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_TRACK
	);
}

static void koto_album_init(KotoAlbum * self) {
	self->description = NULL;
	self->genres = NULL;
	self->has_album_art = FALSE;
	self->narrator = NULL;
	self->paths = g_hash_table_new(g_str_hash, g_str_equal);

	self->playlist = koto_playlist_new_with_uuid(NULL); // Create a playlist, with UUID set to NULL temporarily (until we know the album UUID)
	koto_playlist_apply_model(self->playlist, KOTO_PREFERRED_PLAYLIST_SORT_TYPE_SORT_BY_TRACK_POS); // Sort by track position
	koto_playlist_set_album_uuid(self->playlist, self->uuid); // Set the playlist UUID to be the same as the album
	g_object_set(self->playlist, "ephemeral", TRUE, NULL); // Set as ephemeral / temporary
	koto_cartographer_add_playlist(koto_maps, self->playlist); // Add to cartographer

	self->year = 0;
}

void koto_album_add_track(
	KotoAlbum * self,
	KotoTrack * track
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (!KOTO_IS_TRACK(track)) { // Not a track
		return;
	}

	if (koto_playlist_get_position_of_track(self->playlist, track) != -1) { // Have added it already
		return;
	}

	koto_cartographer_add_track(koto_maps, track); // Add the track to cartographer if necessary

	GList * track_genres = koto_track_get_genres(track); // Get the genres for the track
	GList * current_genre_list;

	gchar * existing_genres_as_string = koto_utils_join_string_list(self->genres, ";");

	for (current_genre_list = track_genres; current_genre_list != NULL; current_genre_list = current_genre_list->next) { // Iterate over each item in the track genres
		gchar * track_genre = current_genre_list->data; // Get this genre

		if (g_strcmp0(track_genre, "") == 0) {
			continue;
		}

		if (koto_utils_string_contains_substring(existing_genres_as_string, track_genre)) { // Genres list contains this genre
			continue;
		}

		self->genres = g_list_append(self->genres, g_strdup(track_genre)); // Duplicate the genre and add it to our list
	}

	if (self->year == 0) { // Don't have a year set yet
		guint64 track_year = koto_track_get_year(track); // Get the year from this track

		if (track_year > 0) { // Have a probably valid year set
			self->year = track_year; // Set our track
		}
	}

	if (!koto_utils_string_is_valid(self->narrator)) { // No narrator set yet
		gchar * track_narrator = koto_track_get_narrator(track); // Get the narrator for the track

		if (koto_utils_string_is_valid(track_narrator)) { // If this track has a narrator
			self->narrator = g_strdup(track_narrator);
			g_free(track_narrator);
		}
	}

	koto_playlist_add_track(self->playlist, track, FALSE, FALSE); // Add the track to our internal playlist

	g_signal_emit(
		self,
		album_signals[SIGNAL_TRACK_ADDED],
		0,
		track
	);
}

void koto_album_commit(KotoAlbum * self) {
	if (self->art_path == NULL) { // If art_path isn't defined when committing
		koto_album_set_album_art(self, ""); // Set to an empty string
	}

	gchar * genres_string = koto_utils_join_string_list(self->genres, ";");

	gchar * commit_op = g_strdup_printf(
		"INSERT INTO albums(id, artist_id, name, description, narrator, art_path, genres, year)"
		"VALUES('%s', '%s', quote(\"%s\"), quote(\"%s\"), quote(\"%s\"), quote(\"%s\"), '%s', %ld)"
		"ON CONFLICT(id) DO UPDATE SET artist_id=excluded.artist_id, name=excluded.name, description=excluded.description, narrator=excluded.narrator, art_path=excluded.art_path, genres=excluded.genres, year=excluded.year;",
		self->uuid,
		self->artist_uuid,
		koto_utils_string_get_valid(self->name),
		koto_utils_string_get_valid(self->description),
		koto_utils_string_get_valid(self->narrator),
		koto_utils_string_get_valid(self->art_path),
		koto_utils_string_get_valid(genres_string),
		self->year
	);

	new_transaction(commit_op, "Failed to write our album to the database", FALSE);

	g_free(genres_string);

	GHashTableIter paths_iter;
	g_hash_table_iter_init(&paths_iter, self->paths); // Create an iterator for our paths
	gpointer lib_uuid_ptr, album_rel_path_ptr;
	while (g_hash_table_iter_next(&paths_iter, &lib_uuid_ptr, &album_rel_path_ptr)) {
		gchar * lib_uuid = lib_uuid_ptr;
		gchar * album_rel_path = album_rel_path_ptr;

		gchar * commit_op = g_strdup_printf(
			"INSERT INTO libraries_albums(id, album_id, path)"
			"VALUES ('%s', '%s', quote(\"%s\"))"
			"ON CONFLICT(id, album_id) DO UPDATE SET path=excluded.path;",
			lib_uuid,
			self->uuid,
			album_rel_path
		);

		new_transaction(commit_op, "Failed to add this path for the album", FALSE);
	}
}

void koto_album_find_album_art(KotoAlbum * self) {
	if (self->has_album_art) { // If we already have album art
		return;
	}

	gchar * optimal_album_path = koto_album_get_path(self);
	DIR * dir = opendir(optimal_album_path); // Attempt to open our directory

	if (dir == NULL) {
		return;
	}

	struct dirent * entry;

	while ((entry = readdir(dir))) {
		if (entry->d_type != DT_REG) { // Not a regular file
			continue; // SKIP
		}

		if (g_str_has_prefix(entry->d_name, ".")) { // Reference to parent dir, self, or a hidden item
			continue; // Skip
		}

		gchar * full_path = g_strdup_printf("%s%s%s", optimal_album_path, G_DIR_SEPARATOR_S, entry->d_name);

		const char * mime_type = magic_file(magic_cookie, full_path);

		if (
			(mime_type == NULL) || // Failed to get the mimetype
			((mime_type != NULL) && !g_str_has_prefix(mime_type, "image/")) //  Got the mimetype but it is not an image
		) {
			g_free(full_path);
			continue; // Skip
		}

		gchar * album_art_no_ext = g_strdup(koto_utils_get_filename_without_extension(entry->d_name)); // Get the name of the file without the extension

		gchar * lower_art = g_strdup(g_utf8_strdown(album_art_no_ext, -1)); // Lowercase
		g_free(album_art_no_ext);

		gboolean should_set = (g_strrstr(lower_art, "Small") == NULL) && (g_strrstr(lower_art, "back") == NULL); // Not back or small

		g_free(lower_art);

		if (should_set) {
			koto_album_set_album_art(self, full_path);
			g_free(full_path);
			break;
		}

		g_free(full_path);
	}

	closedir(dir);
}

static void koto_album_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoAlbum * self = KOTO_ALBUM(obj);

	switch (prop_id) {
		case PROP_UUID:
			g_value_set_string(val, self->uuid);
			break;
		case PROP_DO_INITIAL_INDEX:
			g_value_set_boolean(val, self->do_initial_index);
			break;
		case PROP_NAME:
			g_value_set_string(val, self->name);
			break;
		case PROP_ART_PATH:
			g_value_set_string(val, koto_album_get_art(self));
			break;
		case PROP_ARTIST_UUID:
			g_value_set_string(val, self->artist_uuid);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_album_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoAlbum * self = KOTO_ALBUM(obj);

	switch (prop_id) {
		case PROP_UUID:
			koto_album_set_uuid(self, g_value_get_string(val));
			break;
		case PROP_DO_INITIAL_INDEX:
			self->do_initial_index = g_value_get_boolean(val);
			break;
		case PROP_NAME: // Name of album
			koto_album_set_album_name(self, g_value_get_string(val));
			break;
		case PROP_ART_PATH: // Path to art
			koto_album_set_album_art(self, g_value_get_string(val));
			break;
		case PROP_ARTIST_UUID:
			koto_album_set_artist_uuid(self, g_value_get_string(val));
			break;
		case PROP_ALBUM_PREPARED_GENRES:
			koto_album_set_preparsed_genres(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_DESCRIPTION:
			koto_album_set_description(self, g_value_get_string(val));
			break;
		case PROP_NARRATOR:
			koto_album_set_narrator(self, g_value_get_string(val));
			break;
		case PROP_YEAR:
			koto_album_set_year(self, g_value_get_uint64(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

gchar * koto_album_get_art(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return g_strdup("");
	}

	return g_strdup((self->has_album_art && koto_utils_string_is_valid(self->art_path)) ? self->art_path : "");
}

gchar * koto_album_get_artist_uuid(KotoAlbum * self) {
	return KOTO_IS_ALBUM(self) ? self->artist_uuid : NULL;
}

gchar * koto_album_get_description(KotoAlbum * self) {
	return KOTO_IS_ALBUM(self) ? self->description : NULL;
}

GList * koto_album_get_genres(KotoAlbum * self) {
	return KOTO_IS_ALBUM(self) ? self->genres : NULL;
}

KotoLibraryType koto_album_get_lib_type(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return KOTO_LIBRARY_TYPE_UNKNOWN;
	}

	return koto_artist_get_lib_type(koto_cartographer_get_artist_by_uuid(koto_maps, self->artist_uuid)); // Get the lib type for the artist. If artist isn't valid, we just return UNKNOWN
}

gchar * koto_album_get_name(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return NULL;
	}

	if (!koto_utils_string_is_valid(self->name)) { // Not set
		return NULL;
	}

	return self->name; // Return name of the album
}

gchar * koto_album_get_narrator(KotoAlbum * self) {
	return KOTO_IS_ALBUM(self) ? self->narrator : NULL;
}

gchar * koto_album_get_path(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self) || (KOTO_IS_ALBUM(self) && (g_list_length(g_hash_table_get_keys(self->paths)) == 0))) { // If this is not an album or is but we have no paths associated with it
		return NULL;
	}

	GList * libs = koto_cartographer_get_libraries(koto_maps); // Get all of our libraries
	GList * cur_lib_list;

	for (cur_lib_list = libs; cur_lib_list != NULL; cur_lib_list = libs->next) { // Iterate over our libraries
		KotoLibrary * cur_library = libs->data; // Get this as a KotoLibrary
		gchar * library_relative_path = g_hash_table_lookup(self->paths, koto_library_get_uuid(cur_library)); // Get any relative path in our paths based on the current UUID

		if (!koto_utils_string_is_valid(library_relative_path)) { // Not a valid path
			continue;
		}

		return g_strdup(g_build_path(G_DIR_SEPARATOR_S, koto_library_get_path(cur_library), library_relative_path, NULL)); // Build our full library path using library's path and our file relative path
	}

	return NULL;
}

KotoPlaylist * koto_album_get_playlist(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return NULL;
	}

	if (!KOTO_IS_PLAYLIST(self->playlist)) { // Not a playlist
		return NULL;
	}

	return self->playlist;
}

GListStore * koto_album_get_store(KotoAlbum * self) {
	KotoPlaylist * playlist = koto_album_get_playlist(self);

	if (!KOTO_IS_PLAYLIST(playlist)) { // Not a playlist
		return NULL;
	}

	return koto_playlist_get_store(playlist); // Return the store from the playlist
}

gchar * koto_album_get_uuid(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return NULL;
	}

	return self->uuid; // Return the UUID
}

guint64 koto_album_get_year(KotoAlbum * self) {
	return KOTO_IS_ALBUM(self) ? self->year : 0;
}

void koto_album_mark_as_finalized(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (self->finalized) { // Already finalized
		return;
	}

	self->finalized = TRUE;
	koto_playlist_mark_as_finalized(self->playlist);
	//koto_playlist_apply_model(self->playlist, self->model); // Resort our playlist
}

void koto_album_remove_track(
	KotoAlbum * self,
	KotoTrack * track
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (!KOTO_IS_TRACK(track)) { // Not a track
		return;
	}

	koto_playlist_remove_track_by_uuid(
		self->playlist,
		koto_track_get_uuid(track)
	);

	g_signal_emit(
		self,
		album_signals[SIGNAL_TRACK_REMOVED],
		0,
		track
	);
}

void koto_album_set_album_name(
	KotoAlbum * self,
	const gchar * album_name
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (album_name == NULL) { // Not valid album name
		return;
	}

	if (self->name != NULL) {
		g_free(self->name);
	}

	self->name = g_strdup(album_name);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_NAME]);
}

void koto_album_set_artist_uuid(
	KotoAlbum * self,
	const gchar * artist_uuid
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (artist_uuid == NULL) {
		return;
	}

	if (self->artist_uuid != NULL) {
		g_free(self->artist_uuid);
	}

	self->artist_uuid = g_strdup(artist_uuid);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ARTIST_UUID]);
}

void koto_album_set_album_art(
	KotoAlbum * self,
	const gchar * album_art
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (album_art == NULL) { // Not valid album art
		return;
	}

	if (self->art_path != NULL) {
		g_free(self->art_path);
	}

	self->art_path = g_strdup(album_art);

	self->has_album_art = TRUE;

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ART_PATH]);
}

void koto_album_set_as_current_playlist(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) {
		return;
	}

	if (!KOTO_IS_CURRENT_PLAYLIST(current_playlist)) {
		return;
	}

	if (!KOTO_IS_PLAYLIST(self->playlist)) { // Don't have a playlist for the album for some reason
		return;
	}

	koto_current_playlist_set_playlist(current_playlist, self->playlist, TRUE, FALSE); // Set our new current playlist and start playing immediately
}

void koto_album_set_description(
	KotoAlbum * self,
	const gchar * description
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (!koto_utils_string_is_valid(description)) {
		return;
	}

	self->description = g_strdup(description);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_DESCRIPTION]);
}

void koto_album_set_narrator(
	KotoAlbum * self,
	const gchar * narrator
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (!koto_utils_string_is_valid(narrator)) {
		return;
	}

	self->narrator = g_strdup(narrator);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_NARRATOR]);
}

void koto_album_set_path(
	KotoAlbum * self,
	KotoLibrary * lib,
	const gchar * fixed_path
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	gchar * path = g_strdup(fixed_path); // Duplicate our fixed_path
	gchar * relative_path = koto_library_get_relative_path_to_file(lib, path); // Get the relative path to the file for the given library

	gchar * library_uuid = koto_library_get_uuid(lib); // Get the library for this path
	g_hash_table_replace(self->paths, library_uuid, relative_path); // Replace any existing value or add this one

	koto_album_set_album_name(self, g_path_get_basename(relative_path)); // Update our album name based on the base name

	if (!self->do_initial_index) { // Not doing our initial index
		return;
	}

	koto_album_find_album_art(self); // Update our path for the album art
	self->do_initial_index = FALSE;
}

void koto_album_set_preparsed_genres(
	KotoAlbum * self,
	gchar * genrelist
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (!koto_utils_string_is_valid(genrelist)) { // If it is an empty string
		return;
	}

	GList * preparsed_genres_list = koto_utils_string_to_string_list(genrelist, ";");

	if (g_list_length(preparsed_genres_list) == 0) { // No genres
		g_list_free(preparsed_genres_list);
		return;
	}

	// TODO: Do a pass on in first memory optimization phase to ensure string elements are freed.
	g_list_free_full(self->genres, NULL); // Free the existing genres list
	self->genres = preparsed_genres_list;
};

void koto_album_set_uuid(
	KotoAlbum * self,
	const gchar * uuid
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (!koto_utils_string_is_valid(uuid)) {
		return;
	}

	self->uuid = g_strdup(uuid);
	g_object_set(
		self->playlist,
		"album-uuid",
		self->uuid,
		"uuid",
		self->uuid, // Ensure the playlist has the same UUID as the album
		NULL
	);

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_UUID]);
}

void koto_album_set_year(
	KotoAlbum * self,
	guint64 year
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (year <= 0) {
		return;
	}

	self->year = year;
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_YEAR]);
}

KotoAlbum * koto_album_new(gchar * artist_uuid) {
	if (!koto_utils_string_is_valid(artist_uuid)) { // Invalid artist UUID provided
		return NULL;
	}

	KotoAlbum * album = g_object_new(
		KOTO_TYPE_ALBUM,
		"artist-uuid",
		artist_uuid,
		"uuid",
		g_strdup(g_uuid_string_random()),
		"do-initial-index",
		TRUE,
		NULL
	);

	return album;
}

KotoAlbum * koto_album_new_with_uuid(
	KotoArtist * artist,
	const gchar * uuid
) {
	gchar * artist_uuid = koto_artist_get_uuid(artist);

	return g_object_new(
		KOTO_TYPE_ALBUM,
		"artist-uuid",
		artist_uuid,
		"uuid",
		g_strdup(uuid),
		"do-initial-index",
		FALSE,
		NULL
	);
}
