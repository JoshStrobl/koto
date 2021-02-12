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
#include <sys/stat.h>
#include "file.h"
#include "file-indexer.h"

struct KotoIndexedAlbum {
	GObject parent_instance;
	gboolean has_album_art;
	gchar *album_name;
	gchar *art_path;
	gchar *path;
	GHashTable *songs;
};

struct _KotoIndexedArtist {
	GObject parent_instance;
	gboolean has_artist_art;
	gchar *artist_name;
	GHashTable *albums;
	gchar *path;
};

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
}

static void koto_indexed_library_init(KotoIndexedLibrary *self) {
	self->music_artists = g_hash_table_new(g_str_hash, g_str_equal);
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
			g_message("Set to %s", self->path);
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

	index_folder(self, self->path);
	magic_close(self->magic_cookie);
}

void index_folder(KotoIndexedLibrary *self, gchar *path) {
	DIR *dir = opendir(path); // Attempt to open our directory

	if (dir == NULL) {
		return;
	}

	struct dirent *entry;

	while ((entry = readdir(dir))) {
		if (!g_str_has_prefix(entry->d_name, ".")) { // Not a reference to parent dir, self, or a hidden item
			gchar *full_path = g_strdup_printf("%s%s%s", path, G_DIR_SEPARATOR_S, entry->d_name);

			if (entry->d_type == DT_DIR) { // Directory
				index_folder(self, full_path); // Index this directory
			} else if (entry->d_type == DT_REG) { // Regular file
				index_file(self, full_path); // Index the file
			}

			g_free(full_path);
		}
	}

	closedir(dir); // Close the directory
}

void index_file(KotoIndexedLibrary *self, gchar *path) {
	const char *mime_type = magic_file(self->magic_cookie, path);

	if (mime_type == NULL) { // Failed to get the mimetype
		return;
	}

	gchar** mime_info = g_strsplit(mime_type, ";", 2); // Only care about our first item

	if (
		g_str_has_prefix(mime_info[0], "audio/") || // Is audio
		g_str_has_prefix(mime_info[0], "image/") // Is image
	) {
		//g_message("File Name: %s", path);
	}

	if (g_str_has_prefix(mime_info[0], "audio/")) { // Is an audio file
		KotoIndexedFile *file = koto_indexed_file_new(path);
		gchar *filepath;
		gchar *parsed_name;

		g_object_get(file,
			"path", &filepath,
			"parsed-name", &parsed_name,
		NULL);

		g_free(filepath);
		g_free(parsed_name);
		g_object_unref(file);
	}

	g_strfreev(mime_info); // Free our mimeinfo
}

KotoIndexedLibrary* koto_indexed_library_new(const gchar *path) {
	return g_object_new(KOTO_TYPE_INDEXED_LIBRARY,
		"path", path,
		NULL
	);
}
