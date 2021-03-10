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
#include "../playlist/current.h"
#include "../playlist/playlist.h"
#include "structs.h"
#include "koto-utils.h"

extern KotoCurrentPlaylist *current_playlist;
extern sqlite3 *koto_db;

struct _KotoIndexedAlbum {
	GObject parent_instance;
	KotoIndexedArtist *artist;
	gchar *uuid;
	gchar *path;

	gchar *name;
	gchar *art_path;
	GHashTable *files;

	gboolean has_album_art;
	gboolean do_initial_index;
};

G_DEFINE_TYPE(KotoIndexedAlbum, koto_indexed_album, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_ARTIST,
	PROP_UUID,
	PROP_DO_INITIAL_INDEX,
	PROP_PATH,
	PROP_ALBUM_NAME,
	PROP_ART_PATH,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

static void koto_indexed_album_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_indexed_album_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_indexed_album_class_init(KotoIndexedAlbumClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_indexed_album_set_property;
	gobject_class->get_property = koto_indexed_album_get_property;

	props[PROP_ARTIST] = g_param_spec_object(
		"artist",
		"Artist associated with Album",
		"Artist associated with Album",
		KOTO_TYPE_INDEXED_ARTIST,
		G_PARAM_CONSTRUCT_ONLY|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_UUID] = g_param_spec_string(
		"uuid",
		"UUID to Album in database",
		"UUID to Album in database",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_DO_INITIAL_INDEX] = g_param_spec_boolean(
		"do-initial-index",
		"Do an initial indexing operating instead of pulling from the database",
		"Do an initial indexing operating instead of pulling from the database",
		FALSE,
		G_PARAM_CONSTRUCT_ONLY|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_PATH] = g_param_spec_string(
		"path",
		"Path",
		"Path to Album",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ALBUM_NAME] = g_param_spec_string(
		"name",
		"Name",
		"Name of Album",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ART_PATH] = g_param_spec_string(
		"art-path",
		"Path to Artwork",
		"Path to Artwork",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_indexed_album_init(KotoIndexedAlbum *self) {
	self->has_album_art = FALSE;
}

void koto_indexed_album_add_file(KotoIndexedAlbum *self, KotoIndexedFile *file) {
	if (file == NULL) { // Not a file
		return;
	}

	if (self->files == NULL) { // No HashTable yet
		self->files = g_hash_table_new(g_str_hash, g_str_equal); // Create a new HashTable
	}

	gchar *file_name;
	g_object_get(file, "parsed-name", &file_name, NULL);

	if (g_hash_table_contains(self->files, file_name)) { // If we have this file already
		g_free(file_name);
		return;
	}

	g_hash_table_insert(self->files, file_name, file); // Add the file
}

void koto_indexed_album_commit(KotoIndexedAlbum *self) {
	gchar *artist_uuid;
	g_object_get(self->artist, "uuid", &artist_uuid, NULL); // Get the artist UUID

	if (self->art_path == NULL) { // If art_path isn't defined when committing
		koto_indexed_album_set_album_art(self, ""); // Set to an empty string
	}

	gchar *commit_op = g_strdup_printf(
		"INSERT INTO albums(id, path, artist_id, name, art_path)"
		"VALUES('%s', quote(\"%s\"), '%s', quote(\"%s\"), quote(\"%s\"))"
		"ON CONFLICT(id) DO UPDATE SET path=excluded.path, name=excluded.name, art_path=excluded.art_path;",
		self->uuid,
		self->path,
		artist_uuid,
		self->name,
		self->art_path
	);

	gchar *commit_op_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, commit_op, 0, 0, &commit_op_errmsg);

	if (rc != SQLITE_OK) {
		g_warning("Failed to write our album to the database: %s", commit_op_errmsg);
	}

	g_free(commit_op);
	g_free(commit_op_errmsg);
}

void koto_indexed_album_find_album_art(KotoIndexedAlbum *self) {
	magic_t magic_cookie = magic_open(MAGIC_MIME);

	if (magic_cookie == NULL) {
		return;
	}

	if (magic_load(magic_cookie, NULL) != 0) {
		magic_close(magic_cookie);
		return;
	}

	DIR *dir = opendir(self->path); // Attempt to open our directory

	if (dir == NULL) {
		return;
	}

	struct dirent *entry;

	while ((entry = readdir(dir))) {
		if (entry->d_type != DT_REG) { // Not a regular file
			continue; // SKIP
		}

		if (g_str_has_prefix(entry->d_name, ".")) { // Reference to parent dir, self, or a hidden item
			continue; // Skip
		}

		gchar *full_path = g_strdup_printf("%s%s%s", self->path, G_DIR_SEPARATOR_S, entry->d_name);

		const char *mime_type = magic_file(magic_cookie, full_path);

		if (mime_type == NULL) { // Failed to get the mimetype
			g_free(full_path);
			continue; // Skip
		}

		if (g_str_has_prefix(mime_type, "image/") && !self->has_album_art) { // Is an image file and doesn't have album art yet
			gchar *album_art_no_ext = g_strdup(koto_utils_get_filename_without_extension(entry->d_name)); // Get the name of the file without the extension
			gchar *lower_art = g_strdup(g_utf8_strdown(album_art_no_ext, -1)); // Lowercase

			if (
				(g_strrstr(lower_art, "Small") == NULL) && // Not Small
				(g_strrstr(lower_art, "back") == NULL) // Not back
			) {
				koto_indexed_album_set_album_art(self, full_path);
				g_free(album_art_no_ext);
				g_free(lower_art);
				break;
			}

			g_free(album_art_no_ext);
			g_free(lower_art);
		}

		g_free(full_path);
	}

	closedir(dir);
	magic_close(magic_cookie);
}

void koto_indexed_album_find_tracks(KotoIndexedAlbum *self, magic_t magic_cookie, const gchar *path) {
	if (magic_cookie == NULL) { // No cookie provided
		magic_cookie = magic_open(MAGIC_MIME);
	}

	if (magic_cookie == NULL) {
		return;
	}

	if (path == NULL) {
		path = self->path;
	}

	if (magic_load(magic_cookie, NULL) != 0) {
		magic_close(magic_cookie);
		return;
	}

	DIR *dir = opendir(path); // Attempt to open our directory

	if (dir == NULL) {
		return;
	}

	struct dirent *entry;

	while ((entry = readdir(dir))) {
		if (g_str_has_prefix(entry->d_name, ".")) { // Reference to parent dir, self, or a hidden item
			continue; // Skip
		}

		gchar *full_path = g_strdup_printf("%s%s%s", path, G_DIR_SEPARATOR_S, entry->d_name);

		if (entry->d_type == DT_DIR) { // If this is a directory
			koto_indexed_album_find_tracks(self, magic_cookie, full_path); // Recursively find tracks
			g_free(full_path);
			continue;
		}

		if (entry->d_type != DT_REG) { // Not a regular file
			continue; // SKIP
		}

		const char *mime_type = magic_file(magic_cookie, full_path);

		if (mime_type == NULL) { // Failed to get the mimetype
			g_free(full_path);
			continue; // Skip
		}

		if (g_str_has_prefix(mime_type, "audio/") || g_str_has_prefix(mime_type, "video/ogg")) { // Is an audio file or ogg because it is special
			gchar *appended_slash_to_path = g_strdup_printf("%s%s", g_strdup(self->path), G_DIR_SEPARATOR_S);
			gchar **possible_cd_split = g_strsplit(full_path, appended_slash_to_path, -1); // Split based on the album path
			guint *cd = (guint*) 1;

			gchar *file_with_cd_sep = g_strdup(possible_cd_split[1]); // Duplicate
			gchar **split_on_cd = g_strsplit(file_with_cd_sep, G_DIR_SEPARATOR_S, -1); // Split based on separator (e.g. / )

			if (g_strv_length(split_on_cd) > 1) {
				gchar *cdd = g_strdup(split_on_cd[0]);
				gchar **cd_sep = g_strsplit(g_utf8_strdown(cdd, -1), "cd", -1);

				if (g_strv_length(cd_sep) > 1) {
					gchar *pos_str = g_strdup(cd_sep[1]);
					cd = (guint*) g_ascii_strtoull(pos_str, NULL, 10); // Attempt to convert
					g_free(pos_str);
				}

				g_strfreev(cd_sep);
				g_free(cdd);
			}

			g_strfreev(split_on_cd);
			g_free(file_with_cd_sep);

			g_strfreev(possible_cd_split);
			g_free(appended_slash_to_path);

			KotoIndexedFile *file = koto_indexed_file_new(self, full_path, cd);

			if (file != NULL) { // Is a file
				koto_indexed_album_add_file(self, file); // Add our file
			}
		}

		g_free(full_path);
	}
}

static void koto_indexed_album_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoIndexedAlbum *self = KOTO_INDEXED_ALBUM(obj);

	switch (prop_id) {
		case PROP_ARTIST:
			g_value_set_object(val, self->artist);
			break;
		case PROP_UUID:
			g_value_set_string(val, self->uuid);
			break;
		case PROP_DO_INITIAL_INDEX:
			g_value_set_boolean(val, self->do_initial_index);
			break;
		case PROP_PATH:
			g_value_set_string(val, self->path);
			break;
		case PROP_ALBUM_NAME:
			g_value_set_string(val, self->name);
			break;
		case PROP_ART_PATH:
			g_value_set_string(val, koto_indexed_album_get_album_art(self));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

gchar* koto_indexed_album_get_album_art(KotoIndexedAlbum *self) {
	return g_strdup((self->has_album_art) ? self->art_path : "");
}

GList* koto_indexed_album_get_files(KotoIndexedAlbum *self) {
	if (self->files == NULL) { // No HashTable yet
		self->files = g_hash_table_new(g_str_hash, g_str_equal); // Create a new HashTable
	}

	return g_hash_table_get_values(self->files);
}

void koto_indexed_album_set_album_art(KotoIndexedAlbum *self, const gchar *album_art) {
	if (album_art == NULL) { // Not valid album art
		return;
	}

	if (self->art_path != NULL) {
		g_free(self->art_path);
	}

	self->art_path = g_strdup(album_art);

	self->has_album_art = TRUE;
}

void koto_indexed_album_remove_file(KotoIndexedAlbum *self, KotoIndexedFile *file) {
	if (file == NULL) { // Not a file
		return;
	}

	if (self->files == NULL) { // No HashTable yet
		self->files = g_hash_table_new(g_str_hash, g_str_equal); // Create a new HashTable
	}

	gchar *file_name;
	g_object_get(file, "parsed-name", &file_name, NULL);
	g_hash_table_remove(self->files, file_name);
}

void koto_indexed_album_remove_file_by_name(KotoIndexedAlbum *self, const gchar *file_name) {
	if (file_name == NULL) {
		return;
	}

	KotoIndexedFile *file = g_hash_table_lookup(self->files, file_name); // Get the files
	koto_indexed_album_remove_file(self, file);
}

void koto_indexed_album_set_album_name(KotoIndexedAlbum *self, const gchar *album_name) {
	if (album_name == NULL) { // Not valid album name
		return;
	}

	if (self->name != NULL) {
		g_free(self->name);
	}

	self->name = g_strdup(album_name);
}

void koto_indexed_album_set_as_current_playlist(KotoIndexedAlbum *self) {
	if (self->files == NULL) { // No files to add to the playlist
		return;
	}

	KotoPlaylist *new_album_playlist = koto_playlist_new(); // Create a new playlist

	GList *tracks_list_uuids = g_hash_table_get_keys(self->files); // Get the UUIDs
	for (guint i = 0; i < g_list_length(tracks_list_uuids); i++) { // For each of the tracks
		koto_playlist_add_track_by_uuid(new_album_playlist, (gchar*) g_list_nth_data(tracks_list_uuids, i)); // Add the UUID
	}

	g_list_free(tracks_list_uuids); // Free the uuids

	koto_current_playlist_set_playlist(current_playlist, new_album_playlist); // Set our new current playlist
}

static void koto_indexed_album_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec){
	KotoIndexedAlbum *self = KOTO_INDEXED_ALBUM(obj);

	switch (prop_id) {
		case PROP_ARTIST:
			self->artist = (KotoIndexedArtist*) g_value_get_object(val);
			break;
		case PROP_UUID:
			self->uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_UUID]);
			break;
		case PROP_DO_INITIAL_INDEX:
			self->do_initial_index = g_value_get_boolean(val);
			break;
		case PROP_PATH: // Path to the album
			koto_indexed_album_update_path(self, g_value_get_string(val));
			break;
		case PROP_ALBUM_NAME: // Name of album
			koto_indexed_album_set_album_name(self, g_value_get_string(val));
			break;
		case PROP_ART_PATH: // Path to art
			koto_indexed_album_set_album_art(self, g_value_get_string(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_indexed_album_update_path(KotoIndexedAlbum *self, const gchar* new_path) {
	if (new_path == NULL) {
		return;
	}

	if (self->path != NULL) {
		g_free(self->path);
	}

	self->path = g_strdup(new_path);

	koto_indexed_album_set_album_name(self, g_path_get_basename(self->path)); // Update our album name based on the base name

	if (!self->do_initial_index) { // Not doing our initial index
		return;
	}

	koto_indexed_album_find_album_art(self); // Update our path for the album art
}

KotoIndexedAlbum* koto_indexed_album_new(KotoIndexedArtist *artist, const gchar *path) {
	KotoIndexedAlbum* album = g_object_new(KOTO_TYPE_INDEXED_ALBUM,
		"artist", artist,
		"uuid", g_strdup(g_uuid_string_random()),
		"do-initial-index", TRUE,
		"path", path,
		NULL
	);

	koto_indexed_album_commit(album);
	koto_indexed_album_find_tracks(album, NULL, NULL); // Scan for tracks now that we committed to the database (hopefully)

	return album;
}

KotoIndexedAlbum* koto_indexed_album_new_with_uuid(KotoIndexedArtist *artist, const gchar *uuid) {
	return g_object_new(KOTO_TYPE_INDEXED_ALBUM,
		"artist", artist,
		"uuid", g_strdup(uuid),
		"do-initial-index", FALSE,
		NULL
	);
}
