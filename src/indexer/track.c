/* track.c
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
#include <taglib/tag_c.h>
#include "../db/db.h"
#include "../db/cartographer.h"
#include "structs.h"
#include "track-helpers.h"
#include "koto-utils.h"

extern KotoCartographer * koto_maps;
extern sqlite3 * koto_db;

struct _KotoTrack {
	GObject parent_instance;
	gchar * artist_uuid;
	gchar * album_uuid;
	gchar * uuid;

	GHashTable * paths;

	gchar * parsed_name;
	guint cd;
	guint position;
	guint * playback_position;

	gboolean do_initial_index;
};

G_DEFINE_TYPE(KotoTrack, koto_track, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_ARTIST_UUID,
	PROP_ALBUM_UUID,
	PROP_UUID,
	PROP_DO_INITIAL_INDEX,
	PROP_PARSED_NAME,
	PROP_CD,
	PROP_POSITION,
	PROP_PLAYBACK_POSITION,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL
};

static void koto_track_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_track_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_track_class_init(KotoTrackClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_track_set_property;
	gobject_class->get_property = koto_track_get_property;

	props[PROP_ARTIST_UUID] = g_param_spec_string(
		"artist-uuid",
		"UUID to Artist associated with the File",
		"UUID to Artist associated with the File",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_ALBUM_UUID] = g_param_spec_string(
		"album-uuid",
		"UUID to Album associated with the File",
		"UUID to Album associated with the File",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_UUID] = g_param_spec_string(
		"uuid",
		"UUID to File in database",
		"UUID to File in database",
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

	props[PROP_PARSED_NAME] = g_param_spec_string(
		"parsed-name",
		"Parsed Name of File",
		"Parsed Name of File",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_CD] = g_param_spec_uint(
		"cd",
		"CD the Track belongs to",
		"CD the Track belongs to",
		0,
		G_MAXUINT16,
		1,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_POSITION] = g_param_spec_uint(
		"position",
		"Position in Audiobook, Album, etc.",
		"Position in Audiobook, Album, etc.",
		0,
		G_MAXUINT16,
		0,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_PLAYBACK_POSITION] = g_param_spec_uint(
		"playback-position",
		"Current playback position",
		"Current playback position",
		0,
		G_MAXUINT16,
		0,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_track_init(KotoTrack * self) {
	self->paths = g_hash_table_new(g_str_hash, g_str_equal); // Create our hash table of paths
}

static void koto_track_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoTrack * self = KOTO_TRACK(obj);

	switch (prop_id) {
		case PROP_ARTIST_UUID:
			g_value_set_string(val, self->artist_uuid);
			break;
		case PROP_ALBUM_UUID:
			g_value_set_string(val, self->album_uuid);
			break;
		case PROP_UUID:
			g_value_set_string(val, self->uuid);
			break;
		case PROP_PARSED_NAME:
			g_value_set_string(val, self->parsed_name);
			break;
		case PROP_CD:
			g_value_set_uint(val, self->cd);
			break;
		case PROP_POSITION:
			g_value_set_uint(val, self->position);
			break;
		case PROP_PLAYBACK_POSITION:
			g_value_set_uint(val, GPOINTER_TO_UINT(self->playback_position));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_track_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoTrack * self = KOTO_TRACK(obj);

	switch (prop_id) {
		case PROP_ARTIST_UUID:
			self->artist_uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ARTIST_UUID]);
			break;
		case PROP_ALBUM_UUID:
			koto_track_set_album_uuid(self, g_value_get_string(val));
			break;
		case PROP_UUID:
			self->uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_UUID]);
			break;
		case PROP_DO_INITIAL_INDEX:
			self->do_initial_index = g_value_get_boolean(val);
			break;
		case PROP_PARSED_NAME:
			koto_track_set_parsed_name(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_CD:
			koto_track_set_cd(self, g_value_get_uint(val));
			break;
		case PROP_POSITION:
			koto_track_set_position(self, g_value_get_uint(val));
			break;
		case PROP_PLAYBACK_POSITION:
			self->playback_position = GUINT_TO_POINTER(g_value_get_uint(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_track_commit(KotoTrack * self) {
	if (!KOTO_IS_TRACK(self)) {
		return;
	}

	if (!koto_utils_is_string_valid(self->artist_uuid)) { // No valid required artist UUID
		return;
	}

	if (!koto_utils_is_string_valid(self->album_uuid)) { // If we do not have a valid album UUID
		self->album_uuid = g_strdup("");
	}

	gchar * commit_msg = "INSERT INTO tracks(id, artist_id, album_id, name, disc, position)" \
						 "VALUES('%s', '%s', '%s', quote(\"%s\"), %d, %d)" \
						 "ON CONFLICT(id) DO UPDATE SET album_id=excluded.album_id, artist_id=excluded.artist_id, name=excluded.name, disc=excluded.disc, position=excluded.position;";

	gchar * commit_op = g_strdup_printf(
		commit_msg,
		self->uuid,
		self->artist_uuid,
		self->album_uuid,
		g_strescape(self->parsed_name, NULL),
		(int) self->cd,
		(int) self->position
	);

	if (new_transaction(commit_op, "Failed to write our file to the database", FALSE) != SQLITE_OK) {
		g_message("Failed with song: %s", self->parsed_name);
	}

	GHashTableIter paths_iter;
	g_hash_table_iter_init(&paths_iter, self->paths); // Create an iterator for our paths
	gpointer lib_uuid_ptr, track_rel_path_ptr;
	while (g_hash_table_iter_next(&paths_iter, &lib_uuid_ptr, &track_rel_path_ptr)) {
		gchar * lib_uuid = lib_uuid_ptr;
		gchar * track_rel_path = track_rel_path_ptr;

		gchar * commit_op = g_strdup_printf(
			"INSERT INTO libraries_tracks(id, track_id, path)"
			"VALUES ('%s', '%s', quote(\"%s\"))"
			"ON CONFLICT(id, track_id) DO UPDATE SET path=excluded.path;",
			lib_uuid,
			self->uuid,
			track_rel_path
		);

		new_transaction(commit_op, "Failed to add this path for the track", FALSE);
	}
}

guint koto_track_get_disc_number(KotoTrack * self) {
	return KOTO_IS_TRACK(self) ? self->cd : 1;
}

GVariant * koto_track_get_metadata_vardict(KotoTrack * self) {
	if (!KOTO_IS_TRACK(self)) {
		return NULL;
	}

	GVariantBuilder * builder = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
	KotoArtist * artist = koto_cartographer_get_artist_by_uuid(koto_maps, self->artist_uuid);
	gchar * artist_name = koto_artist_get_name(artist);

	if (koto_utils_is_string_valid(self->album_uuid)) { // Have an album associated
		KotoAlbum * album = koto_cartographer_get_album_by_uuid(koto_maps, self->album_uuid);

		if (KOTO_IS_ALBUM(album)) {
			gchar * album_art_path = koto_album_get_art(album);
			gchar * album_name = koto_album_get_name(album);

			if (koto_utils_is_string_valid(album_art_path)) { // Valid album art path
				album_art_path = g_strconcat("file://", album_art_path, NULL); // Prepend with file://
				g_variant_builder_add(builder, "{sv}", "mpris:artUrl", g_variant_new_string(album_art_path));
			}

			g_variant_builder_add(builder, "{sv}", "xesam:album", g_variant_new_string(album_name));
		}
	} else {
	} // TODO: Implement artist artwork fetching here

	g_variant_builder_add(builder, "{sv}", "mpris:trackid", g_variant_new_string(self->uuid));

	if (koto_utils_is_string_valid(artist_name)) { // Valid artist name
		GVariant * artist_name_variant;
		GVariantBuilder * artist_list_builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
		g_variant_builder_add(artist_list_builder, "s", artist_name);
		artist_name_variant = g_variant_new("as", artist_list_builder);
		g_variant_builder_unref(artist_list_builder);

		g_variant_builder_add(builder, "{sv}", "xesam:artist", artist_name_variant);
		g_variant_builder_add(builder, "{sv}", "playbackengine:artist", g_variant_new_string(artist_name)); // Add a sort of "meta" string val for our playback engine so we don't need to mess about with the array
	}

	g_variant_builder_add(builder, "{sv}", "xesam:discNumber", g_variant_new_uint64(self->cd));
	g_variant_builder_add(builder, "{sv}", "xesam:title", g_variant_new_string(self->parsed_name));
	g_variant_builder_add(builder, "{sv}", "xesam:url", g_variant_new_string(koto_track_get_path(self)));
	g_variant_builder_add(builder, "{sv}", "xesam:trackNumber", g_variant_new_uint64(self->position));

	GVariant * metadata_ret = g_variant_builder_end(builder);

	return metadata_ret;
}

gchar * koto_track_get_name(KotoTrack * self) {
	if (!KOTO_IS_TRACK(self)) { // Not a track
		return NULL;
	}

	return g_strdup(self->parsed_name);
}

gchar * koto_track_get_path(KotoTrack * self) {
	if (!KOTO_IS_TRACK(self) || (KOTO_IS_TRACK(self) && (g_list_length(g_hash_table_get_keys(self->paths)) == 0))) { // If this is not a track or is but we have no paths associated with it
		return NULL;
	}

	GHashTableIter iter;

	g_hash_table_iter_init(&iter, self->paths); // Create an iterator for our paths
	gpointer uuidptr;
	gpointer relpathptr;

	gchar * path = NULL;

	while (g_hash_table_iter_next(&iter, &uuidptr, &relpathptr)) { // Iterate over all the paths for this file
		KotoLibrary * library = koto_cartographer_get_library_by_uuid(koto_maps, (gchar*) uuidptr);

		if (KOTO_IS_LIBRARY(library)) {
			path = g_strdup(g_build_path(G_DIR_SEPARATOR_S, koto_library_get_path(library), koto_library_get_relative_path_to_file(library, (gchar*) relpathptr), NULL)); // Build our full library path using library's path and our file relative path
			break;
		}
	}

	return path;
}

guint koto_track_get_position(KotoTrack * self) {
	return KOTO_IS_TRACK(self) ? self->position : 0;
}

gchar * koto_track_get_uniqueish_key(KotoTrack * self) {
	KotoArtist * artist = koto_cartographer_get_artist_by_uuid(koto_maps, self->artist_uuid); // Get the artist associated with this track

	if (!KOTO_IS_ARTIST(artist)) { // Don't have an artist
		return g_strdup(self->parsed_name); // Just return the name of the file, which is very likely not unique
	}

	gchar * artist_name = koto_artist_get_name(artist); // Get the artist name

	if (koto_utils_is_string_valid(self->album_uuid)) { // If we have an album associated with this track (not necessarily guaranteed)
		KotoAlbum * possible_album = koto_cartographer_get_album_by_uuid(koto_maps, self->album_uuid);

		if (KOTO_IS_ALBUM(possible_album)) { // Album exists
			gchar * album_name = koto_album_get_name(possible_album); // Get the name of the album

			if (koto_utils_is_string_valid(album_name)) {
				return g_strdup_printf("%s-%s-%s", artist_name, album_name, self->parsed_name); // Create a key of (ARTIST/WRITER)-(ALBUM/AUDIOBOOK)-(CHAPTER/TRACK)
			}
		}
	}

	return g_strdup_printf("%s-%s", artist_name, self->parsed_name); // Create a key of just (ARTIST/WRITER)-(CHAPTER/TRACK)
}

gchar * koto_track_get_uuid(KotoTrack * self) {
	if (!KOTO_IS_TRACK(self)) {
		return NULL;
	}

	return self->uuid; // Do not return a duplicate since otherwise comparison refs fail due to pointer positions being different
}

void koto_track_remove_from_playlist(
	KotoTrack * self,
	gchar * playlist_uuid
) {
	if (!KOTO_IS_TRACK(self)) {
		return;
	}

	gchar * commit_op = g_strdup_printf(
		"DELETE FROM playlist_tracks WHERE track_id='%s' AND playlist_id='%s'",
		self->uuid,
		playlist_uuid
	);

	new_transaction(commit_op, "Failed to remove track from playlist", FALSE);
}

void koto_track_set_album_uuid(
	KotoTrack * self,
	const gchar * album_uuid
) {
	if (!KOTO_IS_TRACK(self)) {
		return;
	}

	if (album_uuid == NULL) {
		return;
	}

	gchar * uuid = g_strdup(album_uuid);

	if (!koto_utils_is_string_valid(uuid)) { // If this is not a valid string
		return;
	}

	self->album_uuid = uuid;
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ALBUM_UUID]);
}

void koto_track_save_to_playlist(
	KotoTrack * self,
	gchar * playlist_uuid,
	gint current
) {
	if (!KOTO_IS_TRACK(self)) {
		return;
	}

	gchar * commit_op = g_strdup_printf(
		"INSERT INTO playlist_tracks(playlist_id, track_id, current)"
		"VALUES('%s', '%s', quote(\"%d\"))",
		playlist_uuid,
		self->uuid,
		current
	);

	new_transaction(commit_op, "Failed to save track to playlist", FALSE);
}

void koto_track_set_cd(
	KotoTrack * self,
	guint cd
) {
	if (cd == 0) { // No change really
		return;
	}

	self->cd = cd;
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_CD]);
}

void koto_track_set_parsed_name(
	KotoTrack * self,
	gchar * new_parsed_name
) {
	if (!koto_utils_is_string_valid(new_parsed_name)) {
		return;
	}

	gboolean have_existing_name = koto_utils_is_string_valid(self->parsed_name);

	if (have_existing_name && (strcmp(self->parsed_name, new_parsed_name) == 0)) { // Have existing name that matches one provided
		return; // Don't do anything
	}

	if (have_existing_name) {
		g_free(self->parsed_name);
	}

	self->parsed_name = g_strdup(new_parsed_name);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_PARSED_NAME]);
}

void koto_track_set_path(
	KotoTrack * self,
	KotoLibrary * lib,
	gchar * fixed_path
) {
	if (!KOTO_IS_TRACK(self)) {
		return;
	}

	if (!koto_utils_is_string_valid(fixed_path)) { // Not a valid path
		return;
	}

	gchar * path = g_strdup(fixed_path); // Duplicate our fixed_path
	gchar * relative_path = koto_library_get_relative_path_to_file(lib, path); // Get the relative path to the file for the given library

	gchar * library_uuid = koto_library_get_uuid(lib); // Get the library for this path
	g_hash_table_replace(self->paths, library_uuid, relative_path); // Replace any existing value or add this one

	if (self->do_initial_index) {
		koto_track_update_metadata(self); // Attempt to get ID3 info
	}
}

void koto_track_set_position(
	KotoTrack * self,
	guint pos
) {
	if (pos == 0) { // No position change really
		return;
	}

	self->position = pos;
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_POSITION]);
}

void koto_track_update_metadata(KotoTrack * self) {
	if (!KOTO_IS_TRACK(self)) { // Not a track
		return;
	}

	gchar * optimal_track_path = koto_track_get_path(self); // Check all the libraries associated with this track, based on priority, return a built path using lib path + relative file path
	TagLib_File * t_file = taglib_file_new(optimal_track_path); // Get a taglib file for this file
	g_free(optimal_track_path);

	if ((t_file != NULL) && taglib_file_is_valid(t_file)) { // If we got the taglib file and it is valid
		TagLib_Tag * tag = taglib_file_tag(t_file); // Get our tag
		koto_track_set_position(self, (uint) taglib_tag_track(tag)); // Get the track, convert to uint and cast as a pointer
	} else { // Failed to get tag info
		guint64 position = koto_track_helpers_get_position_based_on_file_name(g_path_get_basename(optimal_track_path)); // Get the likely position
		koto_track_set_position(self, position); // Set our position
	}

	taglib_tag_free_strings(); // Free strings
	taglib_file_free(t_file); // Free the file
}

KotoTrack * koto_track_new(
	const gchar * artist_uuid,
	const gchar * album_uuid,
	const gchar * parsed_name,
	guint cd
) {
	KotoTrack * track = g_object_new(
		KOTO_TYPE_TRACK,
		"artist-uuid",
		artist_uuid,
		"album-uuid",
		album_uuid,
		"do-initial-index",
		TRUE,
		"uuid",
		g_uuid_string_random(),
		"cd",
		cd,
		"parsed-name",
		parsed_name,
		NULL
	);

	return track;
}

KotoTrack * koto_track_new_with_uuid(const gchar * uuid) {
	return g_object_new(
		KOTO_TYPE_TRACK,
		"uuid",
		g_strdup(uuid),
		NULL
	);
}
