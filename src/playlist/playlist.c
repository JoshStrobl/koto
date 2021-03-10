/* playlist.c
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
#include "playlist.h"

extern sqlite3 *koto_db;

struct _KotoPlaylist {
	GObject parent_instance;
	gchar *uuid;
	gchar *name;
	gchar *art_path;
	guint current_position;

	GQueue *tracks;
};

G_DEFINE_TYPE(KotoPlaylist, koto_playlist, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_UUID,
	PROP_NAME,
	PROP_ART_PATH,
	N_PROPERTIES,
};

static GParamSpec *props[N_PROPERTIES] = { NULL };
static void koto_playlist_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);;
static void koto_playlist_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_playlist_class_init(KotoPlaylistClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_playlist_set_property;
	gobject_class->get_property = koto_playlist_get_property;

	props[PROP_UUID] = g_param_spec_string(
		"uuid",
		"UUID of the Playlist",
		"UUID of the Playlist",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_NAME] = g_param_spec_string(
		"name",
		"Name of the Playlist",
		"Name of the Playlist",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ART_PATH] = g_param_spec_string(
		"art-path",
		"Path to any associated artwork of the Playlist",
		"Path to any associated artwork of the Playlist",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_playlist_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoPlaylist *self = KOTO_PLAYLIST(obj);

	switch (prop_id) {
		case PROP_UUID:
			g_value_set_string(val, self->uuid);
			break;
		case PROP_NAME:
			g_value_set_string(val, self->name);
			break;
		case PROP_ART_PATH:
			g_value_set_string(val, self->art_path);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_playlist_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoPlaylist *self = KOTO_PLAYLIST(obj);

	switch (prop_id) {
		case PROP_UUID:
			koto_playlist_set_uuid(self, g_value_get_string(val));
			break;
		case PROP_NAME:
			koto_playlist_set_name(self, g_value_get_string(val));
			break;
		case PROP_ART_PATH:
			koto_playlist_set_artwork(self, g_value_get_string(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_playlist_init(KotoPlaylist *self) {
	self->current_position = 0; // Default to 0
	self->tracks = g_queue_new(); // Set as an empty GQueue
}

void koto_playlist_add_track(KotoPlaylist *self, KotoIndexedFile *file) {
	gchar *uuid = NULL;
	g_object_get(file, "uuid", &uuid, NULL); // Get the UUID
	koto_playlist_add_track_by_uuid(self, uuid); // Add by the file's UUID
}

void koto_playlist_add_track_by_uuid(KotoPlaylist *self, const gchar *uuid) {
	gchar *dup_uuid = g_strdup(uuid);

	g_queue_push_tail(self->tracks, dup_uuid); // Append the UUID to the tracks
	// TODO: Add to table
}

void koto_playlist_debug(KotoPlaylist *self) {
	g_queue_foreach(self->tracks, koto_playlist_debug_foreach, NULL);
}

void koto_playlist_debug_foreach(gpointer data, gpointer user_data) {
	(void) user_data;
	gchar *uuid = data;
	g_message("UUID in Playlist: %s", uuid);
}

gchar* koto_playlist_get_artwork(KotoPlaylist *self) {
	return g_strdup(self->art_path); // Return a duplicate of our art path
}

guint koto_playlist_get_current_position(KotoPlaylist *self) {
	return self->current_position;
}

gchar* koto_playlist_get_current_uuid(KotoPlaylist *self) {
	return g_queue_peek_nth(self->tracks, self->current_position);
}

guint koto_playlist_get_length(KotoPlaylist *self) {
	return g_queue_get_length(self->tracks); // Get the length of the tracks
}

gchar* koto_playlist_get_name(KotoPlaylist *self) {
	return (self->name == NULL) ? NULL : g_strdup(self->name);
}

GQueue* koto_playlist_get_tracks(KotoPlaylist *self) {
	return self->tracks;
}

gchar* koto_playlist_get_uuid(KotoPlaylist *self) {
	return g_strdup(self->uuid);
}

gchar* koto_playlist_go_to_next(KotoPlaylist *self) {
	gchar *current_uuid = koto_playlist_get_current_uuid(self); // Get the current UUID

	if (current_uuid == self->tracks->tail->data) { // Current UUID matches the last item in the playlist
		return NULL;
	}

	self->current_position++; // Increment our position
	return koto_playlist_get_current_uuid(self); // Return the new UUID
}

gchar* koto_playlist_go_to_previous(KotoPlaylist *self) {
	gchar *current_uuid = koto_playlist_get_current_uuid(self); // Get the current UUID

	if (current_uuid == self->tracks->head->data) { // Current UUID matches the first item in the playlist
		return NULL;
	}

	self->current_position--; // Decrement our position
	return koto_playlist_get_current_uuid(self); // Return the new UUID
}

void koto_playlist_remove_track(KotoPlaylist *self, KotoIndexedFile *file) {
	gchar *file_uuid = NULL;
	g_object_get(file, "uuid", &file_uuid, NULL);

	if (file_uuid == NULL) {
		return;
	}

	koto_playlist_remove_track_by_uuid(self, file_uuid);
}

void koto_playlist_remove_track_by_uuid(KotoPlaylist *self, gchar *uuid) {
	if (uuid == NULL) {
		return;
	}

	gint file_index = g_queue_index(self->tracks, uuid); // Get the position of this uuid

	if (file_index == -1) { // Does not exist in our tracks list
		return;
	}

	g_queue_pop_nth(self->tracks, file_index); // Remove nth where it is the file index
	return;
}

void koto_playlist_set_artwork(KotoPlaylist *self, const gchar *path) {
	if (path == NULL) {
		return;
	}

	magic_t cookie = magic_open(MAGIC_MIME); // Create our magic cookie so we can validate if what we are setting is an image

	if (cookie == NULL) { // Failed to allocate
		return;
	}

	if (magic_load(cookie, NULL) != 0) { // Failed to load our cookie
		goto free_cookie;
	}

	const gchar *mime_type = magic_file(cookie, path); // Get the mimetype for this file

	if ((mime_type == NULL) || !g_str_has_prefix(mime_type, "image/")) { // Failed to get our mimetype or not an image
		goto free_cookie;
	}

	if (self->art_path != NULL) { // art path is set
		g_free(self->art_path); // Free the current path
	}

	self->art_path = g_strdup(path); // Update our art path to a duplicate of provided path

free_cookie:
		magic_close(cookie); // Close and free the cookie to the cookie monster
		return;
}

void koto_playlist_set_name(KotoPlaylist *self, const gchar *name) {
	if (name == NULL) { // No actual name
		return;
	}

	if (self->name != NULL) { // Have a name allocated already
		g_free(self->name); // Free the name
	}

	self->name = g_strdup(name);
	return;
}

void koto_playlist_set_uuid(KotoPlaylist *self, const gchar *uuid) {
	if (uuid == NULL) { // No actual UUID
		return;
	}

	if (self->uuid != NULL) { // Have a UUID allocated already
		return; // Do not allow changing the UUID
	}

	self->uuid = g_strdup(uuid); // Set the new UUID
	return;
}

KotoPlaylist* koto_playlist_new() {
	return g_object_new(KOTO_TYPE_PLAYLIST,
		"uuid", g_uuid_string_random(),
		NULL
	);
}

KotoPlaylist* koto_playlist_new_with_uuid(const gchar *uuid) {
	return g_object_new(KOTO_TYPE_PLAYLIST,
		"uuid", uuid,
		NULL
	);
}
