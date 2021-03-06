/* file-indexer.c
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

#include <dirent.h>
#include <magic.h>
#include <stdio.h>
#include <sys/stat.h>
#include <taglib/tag_c.h>
#include "../db/db.h"
#include "structs.h"
#include "../koto-utils.h"

extern sqlite3 *koto_db;

struct _KotoIndexedLibrary {
	GObject parent_instance;
	gchar *path;
	magic_t magic_cookie;
	GHashTable *music_artists;
};

G_DEFINE_TYPE(KotoIndexedLibrary, koto_indexed_library, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_PATH,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

static void koto_indexed_library_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_indexed_library_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_indexed_library_class_init(KotoIndexedLibraryClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_indexed_library_set_property;
	gobject_class->get_property = koto_indexed_library_get_property;

	props[PROP_PATH] = g_param_spec_string(
		"path",
		"Path",
		"Path to Music",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
	taglib_id3v2_set_default_text_encoding(TagLib_ID3v2_UTF8); // Ensure our id3v2 text encoding is UTF-8
}

static void koto_indexed_library_init(KotoIndexedLibrary *self) {
	self->music_artists = g_hash_table_new(g_str_hash, g_str_equal);
}

void koto_indexed_library_add_artist(KotoIndexedLibrary *self, KotoIndexedArtist *artist) {
	if (artist == NULL) { // No artist
		return;
	}

	koto_indexed_library_get_artists(self); // Call to generate if needed

	gchar *artist_name;
	g_object_get(artist, "name", &artist_name, NULL);

	if (g_hash_table_contains(self->music_artists, artist_name)) { // Already have the artist
		g_free(artist_name);
		return;
	}

	g_hash_table_insert(self->music_artists, artist_name, artist); // Add the artist by its name (this needs to be done so we can get the artist when doing the depth of 2 indexing for the album)
}

KotoIndexedArtist* koto_indexed_library_get_artist(KotoIndexedLibrary *self, gchar *artist_name) {
	if (artist_name == NULL) {
		return NULL;
	}

	koto_indexed_library_get_artists(self); // Call to generate if needed

	return g_hash_table_lookup(self->music_artists, (KotoIndexedArtist*) artist_name);
}

GHashTable* koto_indexed_library_get_artists(KotoIndexedLibrary *self) {
	if (self->music_artists == NULL) { // Not a HashTable
		self->music_artists = g_hash_table_new(g_str_hash, g_str_equal);
	}

	return self->music_artists;
}

void koto_indexed_library_remove_artist(KotoIndexedLibrary *self, KotoIndexedArtist *artist) {
	if (artist == NULL) {
		return;
	}

	koto_indexed_library_get_artists(self); // Call to generate if needed

	gchar *artist_name;
	g_object_get(artist, "name", &artist_name, NULL);

	g_hash_table_remove(self->music_artists, artist_name); // Remove the artist
}

static void koto_indexed_library_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoIndexedLibrary *self = KOTO_INDEXED_LIBRARY(obj);

	switch (prop_id) {
		case PROP_PATH:
			g_value_set_string(val, self->path);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_indexed_library_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec){
	KotoIndexedLibrary *self = KOTO_INDEXED_LIBRARY(obj);

	switch (prop_id) {
		case PROP_PATH:
			koto_indexed_library_set_path(self, g_strdup(g_value_get_string(val)));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_indexed_library_set_path(KotoIndexedLibrary *self, gchar *path) {
	if (path == NULL) {
		return;
	}

	if (self->path != NULL) {
		g_free(self->path);
	}

	self->path = path;

	if (created_new_db) { // Created new database
		start_indexing(self); // Start index operation
	} else { // Have existing database
		read_from_db(self); // Read from the database
	}
}

int process_artists(void *data, int num_columns, char **fields, char **column_names) {
	(void) num_columns; (void) column_names; // Don't need these

	KotoIndexedLibrary *self = (KotoIndexedLibrary*) data;

	gchar *artist_uuid = g_strdup(koto_utils_unquote_string(fields[0])); // First column is UUID
	gchar *artist_path = g_strdup(koto_utils_unquote_string(fields[1])); // Second column is path
	gchar *artist_name = g_strdup(koto_utils_unquote_string(fields[3])); // Fourth column is artist name

	KotoIndexedArtist *artist = koto_indexed_artist_new_with_uuid(artist_uuid); // Create our artist with the UUID

	g_object_set(artist,
		"path", artist_path, // Set path
		"name", artist_name,  // Set name
	NULL);

	koto_indexed_library_add_artist(self, artist); // Add the artist

	int albums_rc = sqlite3_exec(koto_db, g_strdup_printf("SELECT * FROM albums WHERE artist_id=\"%s\"", artist_uuid), process_albums, artist, NULL); // Process our albums
	if (albums_rc != SQLITE_OK) { // Failed to get our albums
		g_critical("Failed to read our albums: %s", sqlite3_errmsg(koto_db));
		return 1;
	}

	g_free(artist_uuid);
	g_free(artist_path);
	g_free(artist_name);

	return 0;
}

int process_albums(void *data, int num_columns, char **fields, char **column_names) {
	(void) num_columns; (void) column_names; // Don't need these

	KotoIndexedArtist *artist = (KotoIndexedArtist*) data;

	gchar *album_uuid = g_strdup(koto_utils_unquote_string(fields[0]));
	gchar *path = g_strdup(koto_utils_unquote_string(fields[1]));
	gchar *artist_uuid = g_strdup(koto_utils_unquote_string(fields[2]));
	gchar *album_name = g_strdup(koto_utils_unquote_string(fields[3]));
	gchar *album_art = (fields[4] != NULL) ? g_strdup(koto_utils_unquote_string(fields[4])) : NULL;

	KotoIndexedAlbum *album = koto_indexed_album_new_with_uuid(artist, album_uuid); // Create our album

	g_object_set(album,
		"path", path, // Set the path
		"name", album_name, // Set name
		"art-path", album_art, // Set art path if any
	NULL);

	koto_indexed_artist_add_album(artist, album); // Add the album

	int tracks_rc = sqlite3_exec(koto_db, g_strdup_printf("SELECT * FROM tracks WHERE album_id=\"%s\"", album_uuid), process_tracks, album, NULL); // Process our tracks
	if (tracks_rc != SQLITE_OK) { // Failed to get our tracks
		g_critical("Failed to read our tracks: %s", sqlite3_errmsg(koto_db));
		return 1;
	}

	g_free(album_uuid);
	g_free(path);
	g_free(artist_uuid);
	g_free(album_name);

	if (album_art != NULL) {
		g_free(album_art);
	}

	return 0;
}

int process_tracks(void *data, int num_columns, char **fields, char **column_names) {
	(void) num_columns; (void) column_names; // Don't need these

	KotoIndexedAlbum *album = (KotoIndexedAlbum*) data;
	gchar *track_uuid = g_strdup(koto_utils_unquote_string(fields[0]));
	gchar *path = g_strdup(koto_utils_unquote_string(fields[1]));
	gchar *artist_uuid = g_strdup(koto_utils_unquote_string(fields[3]));
	gchar *album_uuid = g_strdup(koto_utils_unquote_string(fields[4]));
	gchar *file_name = g_strdup(koto_utils_unquote_string(fields[5]));
	gchar *name = g_strdup(koto_utils_unquote_string(fields[6]));
	guint *disc_num = (guint*) g_ascii_strtoull(fields[6], NULL, 10);
	guint *position = (guint*) g_ascii_strtoull(fields[7], NULL, 10);

	KotoIndexedFile *file = koto_indexed_file_new_with_uuid(track_uuid); // Create our file
	g_object_set(file,
		"artist-uuid", artist_uuid,
		"album-uuid", album_uuid,
		"path", path,
		"file-name", file_name,
		"parsed-name", name,
		"cd", disc_num,
		"position", position,
	NULL);

	koto_indexed_album_add_file(album, file); // Add the file

	g_free(track_uuid);
	g_free(path);
	g_free(artist_uuid);
	g_free(album_uuid);
	g_free(file_name);
	g_free(name);

	return 0;
}

void read_from_db(KotoIndexedLibrary *self) {
	int artists_rc = sqlite3_exec(koto_db, "SELECT * FROM artists", process_artists, self, NULL); // Process our artists
	if (artists_rc != SQLITE_OK) { // Failed to get our artists
		g_critical("Failed to read our artists: %s", sqlite3_errmsg(koto_db));
		return;
	}

	g_hash_table_foreach(self->music_artists, output_artists, NULL);
}

void start_indexing(KotoIndexedLibrary *self) {
	struct stat library_stat;
	int success = stat(self->path, &library_stat);

	if (success != 0) { // Failed to read the library path
		return;
	}

	if (!S_ISDIR(library_stat.st_mode)) { // Is not a directory
		g_warning("%s is not a directory", self->path);
		return;
	}

	self->magic_cookie = magic_open(MAGIC_MIME);

	if (self->magic_cookie == NULL) {
		return;
	}

	if (magic_load(self->magic_cookie, NULL) != 0) {
		magic_close(self->magic_cookie);
		return;
	}

	index_folder(self, self->path, 0);
	magic_close(self->magic_cookie);
	g_hash_table_foreach(self->music_artists, output_artists, NULL);
}

KotoIndexedLibrary* koto_indexed_library_new(const gchar *path) {
	return g_object_new(KOTO_TYPE_INDEXED_LIBRARY,
		"path", path,
		NULL
	);
}

void index_folder(KotoIndexedLibrary *self, gchar *path, guint depth) {
	depth++;

	DIR *dir = opendir(path); // Attempt to open our directory

	if (dir == NULL) {
		return;
	}

	struct dirent *entry;

	while ((entry = readdir(dir))) {
		if (g_str_has_prefix(entry->d_name, ".")) { // A reference to parent dir, self, or a hidden item
			continue;
		}

		gchar *full_path = g_strdup_printf("%s%s%s", path, G_DIR_SEPARATOR_S, entry->d_name);

		if (entry->d_type == DT_DIR) { // Directory
			if (depth == 1) { // If we are following FOLDER/ARTIST/ALBUM then this would be artist
				KotoIndexedArtist *artist = koto_indexed_artist_new(full_path); // Attempt to get the artist
				gchar *artist_name;

				g_object_get(artist,
					"name", &artist_name,
					NULL
				);

				koto_indexed_library_add_artist(self, artist); // Add the artist
				index_folder(self, full_path, depth); // Index this directory
				g_free(artist_name);
			} else if (depth == 2) { // If we are following FOLDER/ARTIST/ALBUM then this would be album
				gchar *artist_name = g_path_get_basename(path); // Get the last entry from our path which is probably the artist

				KotoIndexedArtist *artist = koto_indexed_library_get_artist(self, artist_name); // Get the artist

				if (artist == NULL) {
					continue;
				}

				KotoIndexedAlbum *album = koto_indexed_album_new(artist, full_path);

				koto_indexed_artist_add_album(artist, album); // Add the album
				g_free(artist_name);
			}
		}

		g_free(full_path);
	}

	closedir(dir); // Close the directory
}

void output_artists(gpointer artist_key, gpointer artist_ptr, gpointer data) {
	(void)artist_key;
	(void)data;
	KotoIndexedArtist *artist = (KotoIndexedArtist*) artist_ptr;
	gchar *artist_name;
	g_object_get(artist, "name", &artist_name, NULL);
	g_debug("Artist: %s", artist_name);

	GList *albums = koto_indexed_artist_get_albums(artist); // Get the albums for this artist

	if (albums != NULL) {
		g_debug("Length of Albums: %d", g_list_length(albums));
	}

	GList *a;
	for (a = albums; a != NULL; a = a->next) {
		KotoIndexedAlbum *album = (KotoIndexedAlbum*) a->data;
		gchar *artwork = koto_indexed_album_get_album_art(album);
		gchar *album_name;
		g_object_get(album, "name", &album_name, NULL);
		g_debug("Album Art: %s", artwork);
		g_debug("Album Name: %s", album_name);

		GList *files = koto_indexed_album_get_files(album); // Get the files for the album
		GList *f;

		for (f = files; f != NULL; f = f->next) {
			KotoIndexedFile *file = (KotoIndexedFile*) f->data;
			gchar *filepath;
			gchar *parsed_name;
			guint *pos;

			g_object_get(file,
				"path", &filepath,
				"parsed-name", &parsed_name,
				"position", &pos,
			NULL);
			g_debug("File Path: %s", filepath);
			g_debug("Parsed Name: %s", parsed_name);
			g_debug("Position: %d", GPOINTER_TO_INT(pos));
			g_free(filepath);
			g_free(parsed_name);
		}
	}
}
