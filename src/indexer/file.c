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
#include <taglib/tag_c.h>
#include "file.h"
#include "koto-utils.h"

struct _KotoIndexedFile {
	GObject parent_instance;
	gchar *path;

	gchar *file_name;
	gchar *parsed_name;
	gchar *artist;
	gchar *album;
	guint *position;
	gboolean acquired_metadata_from_id3;
};

G_DEFINE_TYPE(KotoIndexedFile, koto_indexed_file, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_PATH,
	PROP_ARTIST,
	PROP_ALBUM,
	PROP_FILE_NAME,
	PROP_PARSED_NAME,
	PROP_POSITION,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL };

static void koto_indexed_file_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_indexed_file_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_indexed_file_class_init(KotoIndexedFileClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_indexed_file_set_property;
	gobject_class->get_property = koto_indexed_file_get_property;

	props[PROP_PATH] = g_param_spec_string(
		"path",
		"Path",
		"Path to File",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ARTIST] = g_param_spec_string(
		"artist",
		"Name of Artist",
		"Name of Artist",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ALBUM] = g_param_spec_string(
		"album",
		"Name of Album",
		"Name of Album",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_FILE_NAME] = g_param_spec_string(
		"file-name",
		"Name of File",
		"Name of File",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_PARSED_NAME] = g_param_spec_string(
		"parsed-name",
		"Parsed Name of File",
		"Parsed Name of File",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_POSITION] = g_param_spec_uint(
		"position",
		"Position in Audiobook, Album, etc.",
		"Position in Audiobook, Album, etc.",
		0,
		G_MAXUINT16,
		0,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_indexed_file_init(KotoIndexedFile *self) {
	self->acquired_metadata_from_id3 = FALSE;
}

static void koto_indexed_file_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoIndexedFile *self = KOTO_INDEXED_FILE(obj);

	switch (prop_id) {
		case PROP_PATH:
			g_value_set_string(val, self->path);
			break;
		case PROP_ARTIST:
			g_value_set_string(val, self->artist);
			break;
		case PROP_ALBUM:
			g_value_set_string(val, self->album);
			break;
		case PROP_FILE_NAME:
			g_value_set_string(val, self->file_name);
			break;
		case PROP_PARSED_NAME:
			g_value_set_string(val, self->parsed_name);
			break;
		case PROP_POSITION:
			g_value_set_uint(val, GPOINTER_TO_UINT(self->position));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_indexed_file_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoIndexedFile *self = KOTO_INDEXED_FILE(obj);

	switch (prop_id) {
		case PROP_PATH:
			koto_indexed_file_update_path(self, g_value_get_string(val)); // Update the path
			break;
		case PROP_ARTIST:
			self->artist = g_strdup(g_value_get_string(val));
			break;
		case PROP_ALBUM:
			self->album = g_strdup(g_value_get_string(val));
			break;
		case PROP_FILE_NAME:
			koto_indexed_file_set_file_name(self, g_strdup(g_value_get_string(val))); // Update the file name
			break;
		case PROP_PARSED_NAME:
			koto_indexed_file_set_parsed_name(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_POSITION:
			koto_indexed_file_set_position(self, g_value_get_uint(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_indexed_file_parse_name(KotoIndexedFile *self) {
	gchar *copied_file_name = g_strdelimit(g_strdup(self->file_name), "_", ' '); // Replace _ with whitespace for starters

	if (self->artist != NULL && strcmp(self->artist, "") != 0) { // If we have artist set
		gchar **split = g_strsplit(copied_file_name, self->artist, -1); // Split whenever we encounter the artist
		copied_file_name = g_strjoinv("", split); // Remove the artist
		g_strfreev(split);
	}

	if (self->artist != NULL && strcmp(self->album, "") != 0) { // If we have album set
		gchar **split = g_strsplit(copied_file_name, self->album, -1); // Split whenever we encounter the album
		copied_file_name = g_strjoinv("", split); // Remove the album
		g_strfreev(split);

		split = g_strsplit(copied_file_name, g_utf8_strdown(self->album, g_utf8_strlen(self->album, -1)), -1); // Split whenever we encounter the album lowercased
		copied_file_name = g_strjoin("", split, NULL); // Remove the lowercased album
		g_strfreev(split);
	}

	if (g_str_match_string(copied_file_name, "-", FALSE)) { // Contains - like a heathen
		gchar **split = g_strsplit(copied_file_name, " - ", -1); // Split whenever we encounter " - "
		copied_file_name = g_strjoin("", split, NULL); // Remove entirely
		g_strfreev(split);

		if (g_str_match_string(copied_file_name, "-", FALSE)) { // If we have any stragglers
			split = g_strsplit(copied_file_name, "-", -1); // Split whenever we encounter -
			copied_file_name = g_strjoin("", split, NULL); // Remove entirely
			g_strfreev(split);
		}
	}

	gchar *file_without_ext = koto_utils_get_filename_without_extension(copied_file_name);
	g_free(copied_file_name);

	gchar **split = g_regex_split_simple("^([\\d]+)\\s", file_without_ext, G_REGEX_JAVASCRIPT_COMPAT, 0);

	if (g_strv_length(split) > 1) { // Has positional info at the beginning of the file
		gchar *num = split[1];

		g_free(file_without_ext); // Free the prior name
		file_without_ext = g_strdup(split[2]); // Set to our second item which is the rest of the song name without the prefixed numbers

		if ((strcmp(num, "0") == 0) || (strcmp(num, "00") == 0)) { // Is exactly zero
			koto_indexed_file_set_position(self, 0); // Set position to 0
		} else { // Either starts with 0 (like 09) or doesn't start with it at all
			guint64 potential_pos = g_ascii_strtoull(num, NULL, 10); // Attempt to convert

			if (potential_pos != 0) { // Got a legitimate position
				koto_indexed_file_set_position(self, potential_pos);
			}
		}
	}

	g_strfreev(split);

	koto_indexed_file_set_parsed_name(self, file_without_ext);
	g_free(file_without_ext);
}

void koto_indexed_file_set_file_name(KotoIndexedFile *self, gchar *new_file_name) {
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

	if (!self->acquired_metadata_from_id3) { // Haven't acquired our information from ID3
		koto_indexed_file_parse_name(self); // Update our parsed name
	}
}

void koto_indexed_file_set_parsed_name(KotoIndexedFile *self, gchar *new_parsed_name) {
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

void koto_indexed_file_set_position(KotoIndexedFile *self, guint pos) {
	if (pos == 0) { // No position change really
		return;
	}

	self->position = GUINT_TO_POINTER(pos);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_POSITION]);
}

void koto_indexed_file_update_metadata(KotoIndexedFile *self) {
		TagLib_File *t_file = taglib_file_new(self->path); // Get a taglib file for this file

		if ((t_file != NULL) && taglib_file_is_valid(t_file)) { // If we got the taglib file and it is valid
			self->acquired_metadata_from_id3 = TRUE;
			TagLib_Tag *tag = taglib_file_tag(t_file); // Get our tag
			koto_indexed_file_set_parsed_name(self, taglib_tag_title(tag)); // Set the title of the file
			self->artist = g_strdup(taglib_tag_artist((tag))); // Set the artist
			self->album = g_strdup(taglib_tag_album(tag)); // Set the album
			koto_indexed_file_set_position(self, (uint) taglib_tag_track(tag)); // Get the track, convert to uint and cast as a pointer
		}

		taglib_tag_free_strings(); // Free strings
		taglib_file_free(t_file); // Free the file
}

void koto_indexed_file_update_path(KotoIndexedFile *self, const gchar *new_path) {
	if (new_path == NULL) {
		return;
	}

	if (self->path != NULL) { // Already have a path
		g_free(self->path); // Free it
	}

	self->path = g_strdup(new_path); // Duplicate the path and set it
	koto_indexed_file_update_metadata(self); // Attempt to get ID3 info
	koto_indexed_file_set_file_name(self, g_path_get_basename(self->path)); // Update our file name

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_PATH]);
}

KotoIndexedFile* koto_indexed_file_new(const gchar *path) {
	return g_object_new(KOTO_TYPE_INDEXED_FILE,
		"path", path,
		NULL
	);
}
