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
#include <stdio.h>
#include "album.h"
#include "file.h"
#include "koto-utils.h"

struct _KotoIndexedAlbum {
	GObject parent_instance;
	gchar *path;

	gchar *name;
	gchar *art_path;
	GHashTable *files;

	gboolean has_album_art;
};

G_DEFINE_TYPE(KotoIndexedAlbum, koto_indexed_album, G_TYPE_OBJECT);

enum {
	PROP_0,
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



static void koto_indexed_album_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoIndexedAlbum *self = KOTO_INDEXED_ALBUM(obj);

	switch (prop_id) {
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

static void koto_indexed_album_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec){
	KotoIndexedAlbum *self = KOTO_INDEXED_ALBUM(obj);

	switch (prop_id) {
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

	DIR *dir = opendir(new_path); // Attempt to open our directory

	if (dir == NULL) {
		return;
	}

	if (self->path != NULL) {
		g_free(self->path);
	}

	self->path = g_strdup(new_path);

	koto_indexed_album_set_album_name(self, g_path_get_basename(self->path)); // Update our album name based on the base name

	magic_t magic_cookie = magic_open(MAGIC_MIME);

	if (magic_cookie == NULL) {
		return;
	}

	if (magic_load(magic_cookie, NULL) != 0) {
		magic_close(magic_cookie);
		return;
	}

	struct dirent *entry;

	while ((entry = readdir(dir))) {
		if (g_str_has_prefix(entry->d_name, ".")) { // Reference to parent dir, self, or a hidden item
			continue; // Skip
		}

		if (entry->d_type != DT_REG) { // If this is not a regular file
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
			}

			g_free(album_art_no_ext);
			g_free(lower_art);
		} else if (g_str_has_prefix(mime_type, "audio/")) { // Is an audio file
			KotoIndexedFile *file = koto_indexed_file_new(full_path);

			if (file != NULL) { // Is a file
				koto_indexed_album_add_file(self, file); // Add our file
			}
		}

		g_free(full_path);
	}

	magic_close(magic_cookie);
}

KotoIndexedAlbum* koto_indexed_album_new(const gchar *path) {
	return g_object_new(KOTO_TYPE_INDEXED_ALBUM,
		"path", path,
		NULL
	);
}
