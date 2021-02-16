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
#include "album.h"
#include "artist.h"
#include "file.h"
#include "file-indexer.h"

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

	if (self->music_artists == NULL) { // Not a HashTable
		self->music_artists = g_hash_table_new(g_str_hash, g_str_equal);
	}

	gchar *artist_name;
	g_object_get(artist, "name", &artist_name, NULL);

	if (g_hash_table_contains(self->music_artists, artist_name)) { // Already have the artist
		g_free(artist_name);
		return;
	}

	g_hash_table_insert(self->music_artists, artist_name, artist); // Add the artist
}

KotoIndexedArtist* koto_indexed_library_get_artist(KotoIndexedLibrary *self, gchar *artist_name) {
	if (artist_name == NULL) {
		return NULL;
	}

	if (self->music_artists == NULL) { // Not a HashTable
		return NULL;
	}

	return g_hash_table_lookup(self->music_artists, (KotoIndexedArtist*) artist_name);
}

void koto_indexed_library_remove_artist(KotoIndexedLibrary *self, KotoIndexedArtist *artist) {
	if (artist == NULL) {
		return;
	}

	if (self->music_artists == NULL) { // Not a HashTable
		return;
	}

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
			self->path = g_strdup(g_value_get_string(val));
			start_indexing(self);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
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
				KotoIndexedAlbum *album = koto_indexed_album_new(full_path);

				KotoIndexedArtist *artist = koto_indexed_library_get_artist(self, artist_name); // Get the artist

				if (artist == NULL) {
					continue;
				}

				koto_indexed_artist_add_album(artist, album); // Add the album
				g_free(artist_name);
			}
		}

		g_free(full_path);
	}

	closedir(dir); // Close the directory
}

void output_artists(gpointer artist_key, gpointer artist_ptr, gpointer data) {
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

KotoIndexedLibrary* koto_indexed_library_new(const gchar *path) {
	return g_object_new(KOTO_TYPE_INDEXED_LIBRARY,
		"path", path,
		NULL
	);
}