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
#include "structs.h"
#include "track-helpers.h"

extern KotoCartographer * koto_maps;
extern KotoCurrentPlaylist * current_playlist;
extern magic_t magic_cookie;
extern sqlite3 * koto_db;

enum {
	PROP_0,
	PROP_UUID,
	PROP_DO_INITIAL_INDEX,
	PROP_ALBUM_NAME,
	PROP_ART_PATH,
	PROP_ARTIST_UUID,
	PROP_ALBUM_PREPARED_GENRES,
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
	gchar * art_path;
	gchar * artist_uuid;

	GList * genres;

	GList * tracks;
	GHashTable * paths;

	gboolean has_album_art;
	gboolean do_initial_index;
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

	props[PROP_ALBUM_NAME] = g_param_spec_string(
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
	self->has_album_art = FALSE;
	self->genres = NULL;
	self->tracks = NULL;
	self->paths = g_hash_table_new(g_str_hash, g_str_equal);
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

	gchar * track_uuid = koto_track_get_uuid(track);

	if (g_list_index(self->tracks, track_uuid) != -1) { // Have added it already
		return;
	}

	koto_cartographer_add_track(koto_maps, track); // Add the track to cartographer if necessary
	self->tracks = g_list_insert_sorted_with_data(self->tracks, track_uuid, koto_track_helpers_sort_tracks_by_uuid, NULL);

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
		"INSERT INTO albums(id, artist_id, name, art_path, genres)"
		"VALUES('%s', '%s', quote(\"%s\"), quote(\"%s\"), '%s')"
		"ON CONFLICT(id) DO UPDATE SET artist_id=excluded.artist_id, name=excluded.name, art_path=excluded.art_path, genres=excluded.genres;",
		self->uuid,
		self->artist_uuid,
		self->name,
		self->art_path,
		genres_string
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

GList * koto_album_get_genres(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return NULL;
	}

	return self->genres;
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
		case PROP_ALBUM_NAME:
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
			self->uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_UUID]);
			break;
		case PROP_DO_INITIAL_INDEX:
			self->do_initial_index = g_value_get_boolean(val);
			break;
		case PROP_ALBUM_NAME: // Name of album
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
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

gchar * koto_album_get_art(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return g_strdup("");
	}

	return g_strdup((self->has_album_art && koto_utils_is_string_valid(self->art_path)) ? self->art_path : "");
}

gchar * koto_album_get_name(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return NULL;
	}

	if (!koto_utils_is_string_valid(self->name)) { // Not set
		return NULL;
	}

	return g_strdup(self->name); // Return duplicate of the name
}

gchar * koto_album_get_album_uuid(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return NULL;
	}

	if (!koto_utils_is_string_valid(self->uuid)) { // Not set
		return NULL;
	}

	return g_strdup(self->uuid); // Return a duplicate of the UUID
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

		if (!koto_utils_is_string_valid(library_relative_path)) { // Not a valid path
			continue;
		}

		return g_strdup(g_build_path(G_DIR_SEPARATOR_S, koto_library_get_path(cur_library), library_relative_path, NULL)); // Build our full library path using library's path and our file relative path
	}

	return NULL;
}

GList * koto_album_get_tracks(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return NULL;
	}

	return self->tracks; // Return the tracks
}

gchar * koto_album_get_uuid(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return NULL;
	}

	return self->uuid; // Return the UUID
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

	self->tracks = g_list_remove(self->tracks, koto_track_get_uuid(track));
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
}

void koto_album_set_as_current_playlist(KotoAlbum * self) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (self->tracks == NULL) { // No files to add to the playlist
		return;
	}

	KotoPlaylist * new_album_playlist = koto_playlist_new(); // Create a new playlist

	g_object_set(new_album_playlist, "ephemeral", TRUE, NULL); // Set as ephemeral / temporary

	// The following section effectively reverses our tracks, so the first is now last.
	// It then adds them in reverse order, since our playlist add function will prepend to our queue
	// This enables the preservation and defaulting of "newest" first everywhere else, while in albums preserving the rightful order of the album
	// e.g. first track (0) being added last is actually first in the playlist's tracks
	GList * reversed_tracks = g_list_copy(self->tracks); // Copy our tracks so we can reverse the order

	reversed_tracks = g_list_reverse(reversed_tracks); // Actually reverse it

	GList * t;

	for (t = reversed_tracks; t != NULL; t = t->next) { // For each of the tracks
		gchar * track_uuid = t->data;
		koto_playlist_add_track_by_uuid(new_album_playlist, track_uuid, FALSE, FALSE); // Add the UUID, skip commit to table since it is temporary
	}

	g_list_free(t);
	g_list_free(reversed_tracks);

	koto_playlist_apply_model(new_album_playlist, KOTO_PREFERRED_MODEL_TYPE_DEFAULT); // Ensure we are using our default model
	koto_current_playlist_set_playlist(current_playlist, new_album_playlist); // Set our new current playlist
}

void koto_album_set_preparsed_genres(
	KotoAlbum * self,
	gchar * genrelist
) {
	if (!KOTO_IS_ALBUM(self)) { // Not an album
		return;
	}

	if (!koto_utils_is_string_valid(genrelist)) { // If it is an empty string
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

KotoAlbum * koto_album_new(gchar * artist_uuid) {
	if (!koto_utils_is_string_valid(artist_uuid)) { // Invalid artist UUID provided
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
	gchar * artist_uuid = NULL;

	g_object_get(artist, "uuid", &artist_uuid, NULL);

	return g_object_new(
		KOTO_TYPE_ALBUM,
		"artist-uuid",
		artist,
		"uuid",
		g_strdup(uuid),
		"do-initial-index",
		FALSE,
		NULL
	);
}
