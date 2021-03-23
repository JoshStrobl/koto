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
#include "../db/cartographer.h"
#include "playlist.h"

extern KotoCartographer *koto_maps;
extern sqlite3 *koto_db;

struct _KotoPlaylist {
	GObject parent_instance;
	gchar *uuid;
	gchar *name;
	gchar *art_path;
	guint current_position;
	gboolean ephemeral;
	gboolean is_shuffle_enabled;

	GQueue *tracks;
	GQueue *played_tracks;
};

G_DEFINE_TYPE(KotoPlaylist, koto_playlist, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_UUID,
	PROP_NAME,
	PROP_ART_PATH,
	PROP_EPHEMERAL,
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

	props[PROP_EPHEMERAL] = g_param_spec_boolean(
		"ephemeral",
		"Is the playlist ephemeral (temporary)",
		"Is the playlist ephemeral (temporary)",
		FALSE,
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
		case PROP_EPHEMERAL:
			g_value_set_boolean(val, self->ephemeral);
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
		case PROP_EPHEMERAL:
			self->ephemeral = g_value_get_boolean(val);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_playlist_init(KotoPlaylist *self) {
	self->current_position = 0; // Default to 0
	self->played_tracks = g_queue_new(); // Set as an empty GQueue
	self->tracks = g_queue_new(); // Set as an empty GQueue
}

void koto_playlist_add_track(KotoPlaylist *self, KotoIndexedTrack *track) {
	gchar *uuid = NULL;
	g_object_get(track, "uuid", &uuid, NULL); // Get the UUID
	koto_playlist_add_track_by_uuid(self, uuid); // Add by the file's UUID
}

void koto_playlist_add_track_by_uuid(KotoPlaylist *self, const gchar *uuid) {
	gchar *dup_uuid = g_strdup(uuid);

	g_queue_push_tail(self->tracks, dup_uuid); // Append the UUID to the tracks
	// TODO: Add to table
}

void koto_playlist_commit(KotoPlaylist *self) {
	if (self->ephemeral) { // Temporary playlist
		return;
	}

	gchar *commit_op = g_strdup_printf(
		"INSERT INTO playlist_meta(id, name, art_path)"
		"VALUES('%s', quote(\"%s\"), quote(\"%s\")"
		"ON CONFLICT(id) DO UPDATE SET name=excluded.name, art_path=excluded.art_path;",
		self->uuid,
		self->name,
		self->art_path
	);

	gchar *commit_op_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, commit_op, 0, 0, &commit_op_errmsg);

	if (rc != SQLITE_OK) {
		g_warning("Failed to save playlist: %s", commit_op_errmsg);
	} else { // Successfully saved our playlist
		g_queue_foreach(self->tracks, koto_playlist_commit_tracks, self); // Iterate over each track to save it
	}

	g_free(commit_op);
	g_free(commit_op_errmsg);
}

void koto_playlist_commit_tracks(gpointer data, gpointer user_data) {
	KotoIndexedTrack *track = koto_cartographer_get_track_by_uuid(koto_maps, data); // Get the track

	if (track == NULL) { // Not a track
		KotoPlaylist *self = user_data;
		gchar *playlist_uuid = self->uuid; // Get the playlist UUID

		gchar *current_track = g_queue_peek_nth(self->tracks, self->current_position); // Get the UUID of the current track
		koto_indexed_track_save_to_playlist(track, playlist_uuid, g_queue_index(self->tracks, data), (data == current_track) ? 1 : 0); // Call to save the playlist to the track
		g_free(playlist_uuid);
		g_free(current_track);
	}
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

gchar* koto_playlist_get_random_track(KotoPlaylist *self) {
	gchar *track_uuid = NULL;
	guint tracks_len = g_queue_get_length(self->tracks);

	if (tracks_len == g_queue_get_length(self->played_tracks)) { // Played all tracks
		track_uuid = g_list_nth_data(self->tracks->head, 0); // Get the first
		g_queue_clear(self->played_tracks); // Clear our played tracks
	} else { // Have not played all tracks
		GRand* rando_calrissian = g_rand_new(); // Create a new RNG
		guint attempt = 0;

		while (track_uuid != NULL)  { // Haven't selected a track yet
			attempt++;
			gint32 *selected_item = g_rand_int_range(rando_calrissian, 0, (gint32) tracks_len);
			gchar *selected_track = g_queue_peek_nth(self->tracks, (guint) selected_item); // Get the UUID of the selected item

			if (g_queue_index(self->played_tracks, selected_track) == -1) { // Haven't played the track
				track_uuid = selected_track;
				break;
			} else { // Failed to get the track
				if (attempt > tracks_len / 2) {
					break; // Give up
				}
			}
		}

		g_rand_free(rando_calrissian); // Free rando
	}

	return track_uuid;
}

GQueue* koto_playlist_get_tracks(KotoPlaylist *self) {
	return self->tracks;
}

gchar* koto_playlist_get_uuid(KotoPlaylist *self) {
	return g_strdup(self->uuid);
}

gchar* koto_playlist_go_to_next(KotoPlaylist *self) {
	if (self->is_shuffle_enabled) { // Shuffling enabled
		return koto_playlist_get_random_track(self); // Get a random track
	}

	gchar *current_uuid = koto_playlist_get_current_uuid(self); // Get the current UUID

	if (current_uuid == self->tracks->tail->data) { // Current UUID matches the last item in the playlist
		return NULL;
	}

	self->current_position++; // Increment our position
	return koto_playlist_get_current_uuid(self); // Return the new UUID
}

gchar* koto_playlist_go_to_previous(KotoPlaylist *self) {
	if (self->is_shuffle_enabled) { // Shuffling enabled
		return koto_playlist_get_random_track(self); // Get a random track
	}

	gchar *current_uuid = koto_playlist_get_current_uuid(self); // Get the current UUID

	if (current_uuid == self->tracks->head->data) { // Current UUID matches the first item in the playlist
		return NULL;
	}

	self->current_position--; // Decrement our position
	return koto_playlist_get_current_uuid(self); // Return the new UUID
}

void koto_playlist_remove_track(KotoPlaylist *self, KotoIndexedTrack *track) {
	gchar *track_uuid = NULL;
	g_object_get(track, "uuid", &track_uuid, NULL);

	if (track_uuid != NULL) {
		koto_playlist_remove_track_by_uuid(self, track_uuid);
		g_free(track_uuid);
	}

	return;
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

void koto_playlist_unmap(KotoPlaylist *self) {
	koto_cartographer_remove_playlist_by_uuid(koto_maps, self->uuid); // Remove from our cartographer
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
