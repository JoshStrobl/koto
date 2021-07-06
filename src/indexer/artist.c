/* artist.c
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
#include <sqlite3.h>
#include "../db/db.h"
#include "../db/cartographer.h"
#include "../playlist/playlist.h"
#include "../koto-utils.h"
#include "artist-playlist-funcs.h"
#include "structs.h"
#include "track-helpers.h"

extern KotoCartographer * koto_maps;

enum {
	PROP_0,
	PROP_UUID,
	PROP_ARTIST_NAME,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};

enum  {
	SIGNAL_ALBUM_ADDED,
	SIGNAL_ALBUM_REMOVED,
	SIGNAL_HAS_NO_ALBUMS,
	SIGNAL_TRACK_ADDED,
	SIGNAL_TRACK_REMOVED,
	N_SIGNALS
};

static guint artist_signals[N_SIGNALS] = {
	0
};

struct _KotoArtist {
	GObject parent_instance;
	gchar * uuid;

	KotoPlaylist * content_playlist;

	gboolean finalized;
	gboolean has_artist_art;
	gchar * artist_name;
	GList * albums;
	GList * tracks;
	GHashTable * paths;
	KotoLibraryType type;
};

struct _KotoArtistClass {
	GObjectClass parent_class;

	void (* album_added) (
		KotoArtist * artist,
		KotoAlbum * album
	);
	void (* album_removed) (
		KotoArtist * artist,
		KotoAlbum * album
	);
	void (* has_no_albums) (KotoArtist * artist);
	void (* track_added) (
		KotoArtist * artist,
		KotoTrack * track
	);
	void (* track_removed) (
		KotoArtist * artist,
		KotoTrack * track
	);
};

G_DEFINE_TYPE(KotoArtist, koto_artist, G_TYPE_OBJECT);

static void koto_artist_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_artist_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_artist_class_init(KotoArtistClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_artist_set_property;
	gobject_class->get_property = koto_artist_get_property;

	artist_signals[SIGNAL_ALBUM_ADDED] = g_signal_new(
		"album-added",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoArtistClass, album_added),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_ALBUM
	);

	artist_signals[SIGNAL_ALBUM_REMOVED] = g_signal_new(
		"album-removed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoArtistClass, album_removed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_ALBUM
	);

	artist_signals[SIGNAL_HAS_NO_ALBUMS] = g_signal_new(
		"has-no-albums",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoArtistClass, has_no_albums),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	artist_signals[SIGNAL_TRACK_ADDED] = g_signal_new(
		"track-added",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoArtistClass, track_added),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_TRACK
	);

	artist_signals[SIGNAL_TRACK_REMOVED] = g_signal_new(
		"track-removed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoArtistClass, track_removed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_TRACK
	);

	props[PROP_UUID] = g_param_spec_string(
		"uuid",
		"UUID to Artist in database",
		"UUID to Artist in database",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_ARTIST_NAME] = g_param_spec_string(
		"name",
		"Name",
		"Name of Artist",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_artist_init(KotoArtist * self) {
	self->albums = NULL; // Create a new GList

	self->content_playlist = koto_playlist_new(); // Create our playlist
	g_object_set(
		self->content_playlist,
		"ephemeral", // Indicate that it is temporary
		TRUE,
		NULL
	);

	self->finalized = FALSE; // Indicate we not finalized
	self->has_artist_art = FALSE;
	self->paths = g_hash_table_new(g_str_hash, g_str_equal);
	self->tracks = NULL;
	self->type = KOTO_LIBRARY_TYPE_UNKNOWN;
}

void koto_artist_commit(KotoArtist * self) {
	if ((self->uuid == NULL) || strcmp(self->uuid, "")) { // UUID not set
		self->uuid = g_strdup(g_uuid_string_random());
	}

	// TODO: Support multiple types instead of just local music artist
	gchar * commit_op = g_strdup_printf(
		"INSERT INTO artists(id , name, art_path)"
		"VALUES ('%s', quote(\"%s\"), NULL)"
		"ON CONFLICT(id) DO UPDATE SET name=excluded.name, art_path=excluded.art_path;",
		self->uuid,
		self->artist_name
	);

	new_transaction(commit_op, "Failed to write our artist to the database", FALSE);

	GHashTableIter paths_iter;
	g_hash_table_iter_init(&paths_iter, self->paths); // Create an iterator for our paths
	gpointer lib_uuid_ptr, artist_rel_path_ptr;
	while (g_hash_table_iter_next(&paths_iter, &lib_uuid_ptr, &artist_rel_path_ptr)) {
		gchar * lib_uuid = lib_uuid_ptr;
		gchar * artist_rel_path = artist_rel_path_ptr;

		gchar * commit_op = g_strdup_printf(
			"INSERT INTO libraries_artists(id, artist_id, path)"
			"VALUES ('%s', '%s', quote(\"%s\"))"
			"ON CONFLICT(id, artist_id) DO UPDATE SET path=excluded.path;",
			lib_uuid,
			self->uuid,
			artist_rel_path
		);

		new_transaction(commit_op, "Failed to add this path for the artist", FALSE);
	}
}

KotoPlaylist * koto_artist_get_playlist(KotoArtist * self) {
	if (!KOTO_IS_ARTIST(self)) {
		return NULL;
	}

	return self->content_playlist;
}

static void koto_artist_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoArtist * self = KOTO_ARTIST(obj);

	switch (prop_id) {
		case PROP_UUID:
			g_value_set_string(val, self->uuid);
			break;
		case PROP_ARTIST_NAME:
			g_value_set_string(val, self->artist_name);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_artist_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoArtist * self = KOTO_ARTIST(obj);

	switch (prop_id) {
		case PROP_UUID:
			self->uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_UUID]);
			break;
		case PROP_ARTIST_NAME:
			koto_artist_set_artist_name(self, (gchar*) g_value_get_string(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_artist_add_album(
	KotoArtist * self,
	KotoAlbum * album
) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return;
	}

	if (!KOTO_IS_ALBUM(album)) { // Album provided is not an album
		return;
	}

	gchar * album_uuid = koto_album_get_uuid(album);

	if (g_list_index(self->albums, album_uuid) != -1) { // If we have already added the album
		return;
	}

	self->albums = g_list_append(self->albums, album_uuid); // Push to our album's UUID to the end of the list

	g_signal_emit(
		self,
		artist_signals[SIGNAL_ALBUM_ADDED],
		0,
		album
	);
}

void koto_artist_add_track(
	KotoArtist * self,
	KotoTrack * track
) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return;
	}

	if (!KOTO_IS_TRACK(track)) { // Not a track
		return;
	}

	gchar * track_uuid = koto_track_get_uuid(track);

	if (g_list_index(self->tracks, track_uuid) != -1) { // If we have already added the track
		return;
	}

	koto_cartographer_add_track(koto_maps, track); // Add the track to cartographer if necessary
	self->tracks = g_list_insert_sorted_with_data(self->tracks, track_uuid, koto_track_helpers_sort_tracks_by_uuid, NULL);

	koto_playlist_add_track(self->content_playlist, track, FALSE, FALSE); // Add this new track for the artist to its playlist

	g_signal_emit(
		self,
		artist_signals[SIGNAL_TRACK_ADDED],
		0,
		track
	);
}

GList * koto_artist_get_albums(KotoArtist * self) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return NULL;
	}

	return g_list_copy(self->albums);
}

KotoAlbum * koto_artist_get_album_by_name(
	KotoArtist * self,
	gchar * album_name
) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return NULL;
	}

	KotoAlbum * album = NULL;

	GList * cur_list_iter;
	for (cur_list_iter = self->albums; cur_list_iter != NULL; cur_list_iter = cur_list_iter->next) { // Iterate through our albums by their UUIDs
		KotoAlbum * album_for_uuid = koto_cartographer_get_album_by_uuid(koto_maps, cur_list_iter->data); // Get the album

		if (!KOTO_IS_ALBUM(album_for_uuid)) { // Not an album
			continue;
		}

		if (g_strcmp0(koto_album_get_name(album_for_uuid), album_name) == 0) { // These album names match
			album = album_for_uuid;
			break;
		}
	}

	return album;
}

gchar * koto_artist_get_name(KotoArtist * self) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return g_strdup("");
	}

	return g_strdup(koto_utils_is_string_valid(self->artist_name) ? self->artist_name : ""); // Return artist name if set
}

GList * koto_artist_get_tracks(KotoArtist * self) {
	return KOTO_IS_ARTIST(self) ? self->tracks : NULL;
}

KotoLibraryType koto_artist_get_lib_type(KotoArtist * self) {
	return KOTO_IS_ARTIST(self) ? self->type : KOTO_LIBRARY_TYPE_UNKNOWN;
}

gchar * koto_artist_get_uuid(KotoArtist * self) {
	return KOTO_IS_ARTIST(self) ? self->uuid : NULL;
}

void koto_artist_remove_album(
	KotoArtist * self,
	KotoAlbum * album
) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return;
	}

	if (!KOTO_ALBUM(album)) { // No album defined
		return;
	}

	self->albums = g_list_remove(self->albums, koto_album_get_uuid(album));

	g_signal_emit(
		self,
		artist_signals[SIGNAL_ALBUM_REMOVED],
		0,
		album
	);
}

void koto_artist_remove_track(
	KotoArtist * self,
	KotoTrack * track
) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return;
	}

	if (!KOTO_IS_TRACK(track)) { // Not a track
		return;
	}

	gchar * track_uuid = koto_track_get_uuid(track);
	self->tracks = g_list_remove(self->tracks, koto_track_get_uuid(track));

	koto_playlist_remove_track_by_uuid(self->content_playlist, track_uuid); // Remove the track from our playlist

	g_signal_emit(
		self,
		artist_signals[SIGNAL_TRACK_ADDED],
		0,
		track
	);
}

void koto_artist_set_artist_name(
	KotoArtist * self,
	gchar * artist_name
) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return;
	}

	if (!koto_utils_is_string_valid(artist_name)) { // No artist name
		return;
	}

	if (koto_utils_is_string_valid(self->artist_name)) { // Has artist name
		g_free(self->artist_name);
	}

	self->artist_name = g_strdup(artist_name);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ARTIST_NAME]);
}

void koto_artist_set_as_finalized(KotoArtist * self) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return;
	}

	self->finalized = TRUE;

	if (g_list_length(self->albums) == 0) { // Have no albums
		g_signal_emit_by_name(self, "has-no-albums");
	}
}

void koto_artist_set_path(
	KotoArtist * self,
	KotoLibrary * lib,
	const gchar * fixed_path,
	gboolean should_commit
) {
	if (!KOTO_IS_ARTIST(self)) { // Not an artist
		return;
	}

	gchar * path = g_strdup(fixed_path); // Duplicate our fixed_path
	gchar * relative_path = koto_library_get_relative_path_to_file(lib, path); // Get the relative path to the file for the given library

	gchar * library_uuid = koto_library_get_uuid(lib); // Get the library for this path
	g_hash_table_replace(self->paths, library_uuid, relative_path); // Replace any existing value or add this one

	if (should_commit) { // Should commit to the DB
		koto_artist_commit(self); // Save the artist
	}

	self->type = koto_library_get_lib_type(lib); // Define our artist type as the type from the library
}

KotoArtist * koto_artist_new(gchar * artist_name) {
	KotoArtist * artist = g_object_new(
		KOTO_TYPE_ARTIST,
		"uuid",
		g_uuid_string_random(),
		"name",
		artist_name,
		NULL
	);

	return artist;
}

KotoArtist * koto_artist_new_with_uuid(const gchar * uuid) {
	return g_object_new(
		KOTO_TYPE_ARTIST,
		"uuid",
		g_strdup(uuid),
		NULL
	);
}
