/* file.c
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
#include "../db/cartographer.h"
#include "structs.h"
#include "koto-utils.h"

extern KotoCartographer * koto_maps;
extern sqlite3 * koto_db;

struct _KotoIndexedTrack {
	GObject parent_instance;
	gchar * artist_uuid;
	gchar * album_uuid;
	gchar * uuid;
	gchar * path;

	gchar * file_name;
	gchar * parsed_name;
	guint * cd;
	guint * position;
	guint * playback_position;

	gboolean acquired_metadata_from_id3;
	gboolean do_initial_index;
};

G_DEFINE_TYPE(KotoIndexedTrack, koto_indexed_track, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_ARTIST_UUID,
	PROP_ALBUM_UUID,
	PROP_UUID,
	PROP_DO_INITIAL_INDEX,
	PROP_PATH,
	PROP_FILE_NAME,
	PROP_PARSED_NAME,
	PROP_CD,
	PROP_POSITION,
	PROP_PLAYBACK_POSITION,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL
};

static void koto_indexed_track_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_indexed_track_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_indexed_track_class_init(KotoIndexedTrackClass * c) {
	GObjectClass * gobject_class;


	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_indexed_track_set_property;
	gobject_class->get_property = koto_indexed_track_get_property;

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

	props[PROP_PATH] = g_param_spec_string(
		"path",
		"Path",
		"Path to File",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_FILE_NAME] = g_param_spec_string(
		"file-name",
		"Name of File",
		"Name of File",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
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

static void koto_indexed_track_init(KotoIndexedTrack * self) {
	self->acquired_metadata_from_id3 = FALSE;
}

static void koto_indexed_track_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoIndexedTrack * self = KOTO_INDEXED_TRACK(obj);


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
		case PROP_PATH:
			g_value_set_string(val, self->path);
			break;
		case PROP_FILE_NAME:
			g_value_set_string(val, self->file_name);
			break;
		case PROP_PARSED_NAME:
			g_value_set_string(val, self->parsed_name);
			break;
		case PROP_CD:
			g_value_set_uint(val, GPOINTER_TO_UINT(self->cd));
			break;
		case PROP_POSITION:
			g_value_set_uint(val, GPOINTER_TO_UINT(self->position));
			break;
		case PROP_PLAYBACK_POSITION:
			g_value_set_uint(val, GPOINTER_TO_UINT(self->playback_position));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_indexed_track_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoIndexedTrack * self = KOTO_INDEXED_TRACK(obj);


	switch (prop_id) {
		case PROP_ARTIST_UUID:
			self->artist_uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ARTIST_UUID]);
			break;
		case PROP_ALBUM_UUID:
			self->album_uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ALBUM_UUID]);
			break;
		case PROP_UUID:
			self->uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_UUID]);
			break;
		case PROP_DO_INITIAL_INDEX:
			self->do_initial_index = g_value_get_boolean(val);
			break;
		case PROP_PATH:
			koto_indexed_track_update_path(self, g_value_get_string(val)); // Update the path
			break;
		case PROP_FILE_NAME:
			koto_indexed_track_set_file_name(self, g_strdup(g_value_get_string(val))); // Update the file name
			break;
		case PROP_PARSED_NAME:
			koto_indexed_track_set_parsed_name(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_CD:
			koto_indexed_track_set_cd(self, g_value_get_uint(val));
			break;
		case PROP_POSITION:
			koto_indexed_track_set_position(self, g_value_get_uint(val));
			break;
		case PROP_PLAYBACK_POSITION:
			self->playback_position = GUINT_TO_POINTER(g_value_get_uint(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}


void koto_indexed_track_commit(KotoIndexedTrack * self) {
	if ((self->artist_uuid == NULL) || (strcmp(self->artist_uuid, "") == 0)) { // No valid required artist UUID
		return;
	}

	if (self->album_uuid == NULL) {
		g_object_set(self, "album-uuid", "", NULL); // Set to an empty string
	}

	gchar * commit_op = g_strdup_printf(
		"INSERT INTO tracks(id, path, type, artist_id, album_id, file_name, name, disc, position)"
		"VALUES('%s', quote(\"%s\"), 0, '%s', '%s', quote(\"%s\"), quote(\"%s\"), %d, %d)"
		"ON CONFLICT(id) DO UPDATE SET path=excluded.path, type=excluded.type, album_id=excluded.album_id, file_name=excluded.file_name, name=excluded.file_name, disc=excluded.disc, position=excluded.position;",
		self->uuid,
		self->path,
		self->artist_uuid,
		self->album_uuid,
		self->file_name,
		self->parsed_name,
		GPOINTER_TO_INT((int*) self->cd),
		GPOINTER_TO_INT((int*) self->position)
	);

	gchar * commit_op_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, commit_op, 0, 0, &commit_op_errmsg);


	if (rc != SQLITE_OK) {
		g_warning("Failed to write our file to the database: %s", commit_op_errmsg);
	}

	g_free(commit_op);
	g_free(commit_op_errmsg);
}

GVariant * koto_indexed_track_get_metadata_vardict(KotoIndexedTrack * self) {
	if (!KOTO_IS_INDEXED_TRACK(self)) {
		return NULL;
	}

	GVariantBuilder * builder = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);

	gchar * album_art_path = NULL;
	gchar * album_name = NULL;
	gchar * artist_name = NULL;

	KotoIndexedArtist * artist = koto_cartographer_get_artist_by_uuid(koto_maps, self->artist_uuid);
	KotoIndexedAlbum * album = koto_cartographer_get_album_by_uuid(koto_maps, self->album_uuid);


	g_object_get(album, "art-path", &album_art_path, "name", &album_name, NULL);

	g_object_get(artist, "name", &artist_name, NULL);

	g_variant_builder_add(builder, "{sv}", "mpris:trackid", g_variant_new_string(self->uuid));

	if (koto_utils_is_string_valid(album_art_path)) { // Valid album art path
		album_art_path = g_strconcat("file://", album_art_path, NULL); // Prepend with file://
		g_variant_builder_add(builder, "{sv}", "mpris:artUrl", g_variant_new_string(album_art_path));
	}

	g_variant_builder_add(builder, "{sv}", "xesam:album", g_variant_new_string(album_name));

	if (koto_utils_is_string_valid(artist_name)) { // Valid artist name
		GVariant * artist_name_variant;
		GVariantBuilder * artist_list_builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
		g_variant_builder_add(artist_list_builder, "s", artist_name);
		artist_name_variant = g_variant_new("as", artist_list_builder);
		g_variant_builder_unref(artist_list_builder);

		g_variant_builder_add(builder, "{sv}", "xesam:artist", artist_name_variant);
		g_variant_builder_add(builder, "{sv}", "playbackengine:artist", g_variant_new_string(artist_name)); // Add a sort of "meta" string val for our playback engine so we don't need to mess about with the array
	}

	g_variant_builder_add(builder, "{sv}", "xesam:discNumber", g_variant_new_uint64(GPOINTER_TO_UINT(self->cd)));
	g_variant_builder_add(builder, "{sv}", "xesam:title", g_variant_new_string(self->parsed_name));
	g_variant_builder_add(builder, "{sv}", "xesam:url", g_variant_new_string(self->path));
	g_variant_builder_add(builder, "{sv}", "xesam:trackNumber", g_variant_new_uint64(GPOINTER_TO_UINT(self->position)));

	GVariant * metadata_ret = g_variant_builder_end(builder);


	return metadata_ret;
}

gchar * koto_indexed_track_get_uuid(KotoIndexedTrack * self) {
	if (!KOTO_IS_INDEXED_TRACK(self)) {
		return NULL;
	}

	return self->uuid; // Do not return a duplicate since otherwise comparison refs fail due to pointer positions being different
}

void koto_indexed_track_parse_name(KotoIndexedTrack * self) {
	gchar * copied_file_name = g_strdelimit(g_strdup(self->file_name), "_", ' '); // Replace _ with whitespace for starters

	KotoIndexedArtist * artist = NULL;


	artist = koto_cartographer_get_artist_by_uuid(koto_maps, self->artist_uuid);

	if (artist != NULL) { // If we have artist
		gchar * artist_name = NULL;
		g_object_get(artist, "name", &artist_name, NULL);

		if (artist_name != NULL && (strcmp(artist_name, "") != 0)) {
			gchar ** split = g_strsplit(copied_file_name, artist_name, -1); // Split whenever we encounter the artist
			copied_file_name = g_strjoinv("", split); // Remove the artist
			g_strfreev(split);

			split = g_strsplit(copied_file_name, g_utf8_strdown(artist_name, -1), -1); // Lowercase album name and split by that
			copied_file_name = g_strjoinv("", split); // Remove the artist
			g_strfreev(split);
		}
	}

	gchar * file_without_ext = koto_utils_get_filename_without_extension(copied_file_name);


	g_free(copied_file_name);

	gchar ** split = g_regex_split_simple("^([\\d]+)", file_without_ext, G_REGEX_JAVASCRIPT_COMPAT, 0);


	if (g_strv_length(split) > 1) { // Has positional info at the beginning of the file
		gchar * num = split[1];

		g_free(file_without_ext); // Free the prior name
		file_without_ext = g_strdup(split[2]); // Set to our second item which is the rest of the song name without the prefixed numbers

		if ((strcmp(num, "0") == 0) || (strcmp(num, "00") == 0)) { // Is exactly zero
			koto_indexed_track_set_position(self, 0); // Set position to 0
		} else { // Either starts with 0 (like 09) or doesn't start with it at all
			guint64 potential_pos = g_ascii_strtoull(num, NULL, 10); // Attempt to convert

			if (potential_pos != 0) { // Got a legitimate position
				koto_indexed_track_set_position(self, potential_pos);
			}
		}
	}

	g_strfreev(split);

	split = g_strsplit(file_without_ext, " - ", -1); // Split whenever we encounter " - "
	file_without_ext = g_strjoinv("", split); // Remove entirely
	g_strfreev(split);

	split = g_strsplit(file_without_ext, "-", -1); // Split whenever we encounter -
	file_without_ext = g_strjoinv("", split); // Remove entirely
	g_strfreev(split);

	koto_indexed_track_set_parsed_name(self, file_without_ext);
	g_free(file_without_ext);
}

void koto_indexed_track_remove_from_playlist(
	KotoIndexedTrack * self,
	gchar * playlist_uuid
) {
	if (!KOTO_IS_INDEXED_TRACK(self)) {
		return;
	}

	gchar * commit_op = g_strdup_printf(
		"DELETE FROM playlist_tracks WHERE track_id='%s' AND playlist_id='%s'",
		self->uuid,
		playlist_uuid
	);

	gchar * commit_op_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, commit_op, 0, 0, &commit_op_errmsg);


	if (rc != SQLITE_OK) {
		g_warning("Failed to remove track from playlist: %s", commit_op_errmsg);
	}

	g_free(commit_op);
	g_free(commit_op_errmsg);
}

void koto_indexed_track_save_to_playlist(
	KotoIndexedTrack * self,
	gchar * playlist_uuid,
	gint current
) {
	if (!KOTO_IS_INDEXED_TRACK(self)) {
		return;
	}

	gchar * commit_op = g_strdup_printf(
		"INSERT INTO playlist_tracks(playlist_id, track_id, current)"
		"VALUES('%s', '%s', quote(\"%d\"))",
		playlist_uuid,
		self->uuid,
		current
	);

	gchar * commit_op_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, commit_op, 0, 0, &commit_op_errmsg);


	if (rc != SQLITE_OK) {
		g_warning("Failed to save track to playlist: %s", commit_op_errmsg);
	}

	g_free(commit_op);
	g_free(commit_op_errmsg);
}

void koto_indexed_track_set_file_name(
	KotoIndexedTrack * self,
	gchar * new_file_name
) {
	if (new_file_name == NULL) {
		return;
	}

	if ((self->file_name != NULL) && (strcmp(self->file_name, new_file_name) == 0)) { // Not null and the same
		return; // Don't do anything
	}

	if (self->file_name != NULL) { // If it is defined
		g_free(self->file_name);
	}

	self->file_name = g_strdup(new_file_name);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_FILE_NAME]);

	if (!self->acquired_metadata_from_id3 && self->do_initial_index) { // Haven't acquired our information from ID3
		koto_indexed_track_parse_name(self); // Update our parsed name
	}
}

void koto_indexed_track_set_cd(
	KotoIndexedTrack * self,
	guint cd
) {
	if (cd == 0) { // No change really
		return;
	}

	self->cd = GUINT_TO_POINTER(cd);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_CD]);
}

void koto_indexed_track_set_parsed_name(
	KotoIndexedTrack * self,
	gchar * new_parsed_name
) {
	if (new_parsed_name == NULL) {
		return;
	}

	if ((self->parsed_name != NULL) && (strcmp(self->parsed_name, new_parsed_name) == 0)) { // Not null and the same
		return; // Don't do anything
	}

	if (self->parsed_name != NULL) {
		g_free(self->parsed_name);
	}

	self->parsed_name = g_strdup(new_parsed_name);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_PARSED_NAME]);
}

void koto_indexed_track_set_position(
	KotoIndexedTrack * self,
	guint pos
) {
	if (pos == 0) { // No position change really
		return;
	}

	self->position = GUINT_TO_POINTER(pos);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_POSITION]);
}

void koto_indexed_track_update_metadata(KotoIndexedTrack * self) {
	TagLib_File * t_file = taglib_file_new(self->path); // Get a taglib file for this file


	if ((t_file != NULL) && taglib_file_is_valid(t_file)) { // If we got the taglib file and it is valid
		self->acquired_metadata_from_id3 = TRUE;
		TagLib_Tag * tag = taglib_file_tag(t_file); // Get our tag
		koto_indexed_track_set_parsed_name(self, taglib_tag_title(tag)); // Set the title of the file
		koto_indexed_track_set_position(self, (uint) taglib_tag_track(tag)); // Get the track, convert to uint and cast as a pointer
		koto_indexed_track_set_file_name(self, g_path_get_basename(self->path)); // Update our file name
	} else {
		koto_indexed_track_set_file_name(self, g_path_get_basename(self->path)); // Update our file name
	}

	taglib_tag_free_strings(); // Free strings
	taglib_file_free(t_file); // Free the file
}

void koto_indexed_track_update_path(
	KotoIndexedTrack * self,
	const gchar * new_path
) {
	if (new_path == NULL) {
		return;
	}

	if (self->path != NULL) { // Already have a path
		g_free(self->path); // Free it
	}

	self->path = g_strdup(new_path); // Duplicate the path and set it

	if (self->do_initial_index) {
		koto_indexed_track_update_metadata(self); // Attempt to get ID3 info
	}

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_PATH]);
}

KotoIndexedTrack * koto_indexed_track_new(
	KotoIndexedAlbum * album,
	const gchar * path,
	guint * cd
) {
	gchar * artist_uuid;
	gchar * album_uuid;


	g_object_get(album, "artist-uuid", &artist_uuid, "uuid", &album_uuid, NULL); // Get the artist and album uuids from our Album

	KotoIndexedTrack * track = g_object_new(
		KOTO_TYPE_INDEXED_TRACK,
		"artist-uuid",
		artist_uuid,
		"album-uuid",
		album_uuid,
		"do-initial-index",
		TRUE,
		"uuid",
		g_uuid_string_random(),
		"path",
		path,
		"cd",
		cd,
		NULL
	);


	koto_indexed_track_commit(track); // Immediately commit to the database
	return track;
}

KotoIndexedTrack * koto_indexed_track_new_with_uuid(const gchar * uuid) {
	return g_object_new(
		KOTO_TYPE_INDEXED_TRACK,
		"uuid",
		g_strdup(uuid),
		NULL
	);
}
