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
#include <glib-2.0/gio/gio.h>
#include <magic.h>
#include <sqlite3.h>
#include "../db/cartographer.h"
#include "../koto-utils.h"
#include "playlist.h"

extern KotoCartographer * koto_maps;
extern sqlite3 * koto_db;

enum {
	PROP_0,
	PROP_UUID,
	PROP_NAME,
	PROP_ART_PATH,
	PROP_EPHEMERAL,
	PROP_IS_SHUFFLE_ENABLED,
	N_PROPERTIES,
};

enum {
	SIGNAL_MODIFIED,
	SIGNAL_TRACK_ADDED,
	SIGNAL_TRACK_LOAD_FINALIZED,
	SIGNAL_TRACK_REMOVED,
	N_SIGNALS
};

struct _KotoPlaylist {
	GObject parent_instance;
	gchar * uuid;
	gchar * name;
	gchar * art_path;
	gint current_position;
	gchar * current_uuid;

	KotoPreferredModelType model;

	gboolean ephemeral;
	gboolean is_shuffle_enabled;
	gboolean finalized;

	GListStore * store;
	GQueue * sorted_tracks;

	GQueue * tracks; // This is effectively our vanilla value that should never change
	GQueue * played_tracks;
};

struct _KotoPlaylistClass {
	GObjectClass parent_class;

	void (* modified) (KotoPlaylist * playlist);
	void (* track_added) (
		KotoPlaylist * playlist,
		gchar * track_uuid
	);
	void (* track_load_finalized) (KotoPlaylist * playlist);
	void (* track_removed) (
		KotoPlaylist * playlist,
		gchar * track_uuid
	);
};

G_DEFINE_TYPE(KotoPlaylist, koto_playlist, G_TYPE_OBJECT);

static GParamSpec * props[N_PROPERTIES] = {
	NULL
};
static guint playlist_signals[N_SIGNALS] = {
	0
};

static void koto_playlist_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

;
static void koto_playlist_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_playlist_class_init(KotoPlaylistClass * c) {
	GObjectClass * gobject_class;


	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_playlist_set_property;
	gobject_class->get_property = koto_playlist_get_property;

	props[PROP_UUID] = g_param_spec_string(
		"uuid",
		"UUID of the Playlist",
		"UUID of the Playlist",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_NAME] = g_param_spec_string(
		"name",
		"Name of the Playlist",
		"Name of the Playlist",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_ART_PATH] = g_param_spec_string(
		"art-path",
		"Path to any associated artwork of the Playlist",
		"Path to any associated artwork of the Playlist",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_EPHEMERAL] = g_param_spec_boolean(
		"ephemeral",
		"Is the playlist ephemeral (temporary)",
		"Is the playlist ephemeral (temporary)",
		FALSE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_IS_SHUFFLE_ENABLED] = g_param_spec_boolean(
		"is-shuffle-enabled",
		"Is shuffling enabled",
		"Is shuffling enabled",
		FALSE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);

	playlist_signals[SIGNAL_MODIFIED] = g_signal_new(
		"modified",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaylistClass, modified),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	playlist_signals[SIGNAL_TRACK_ADDED] = g_signal_new(
		"track-added",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaylistClass, track_added),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		G_TYPE_CHAR
	);

	playlist_signals[SIGNAL_TRACK_LOAD_FINALIZED] = g_signal_new(
		"track-load-finalized",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaylistClass, track_load_finalized),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	playlist_signals[SIGNAL_TRACK_REMOVED] = g_signal_new(
		"track-removed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoPlaylistClass, track_removed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		G_TYPE_CHAR
	);
}

static void koto_playlist_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoPlaylist * self = KOTO_PLAYLIST(obj);


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
		case PROP_IS_SHUFFLE_ENABLED:
			g_value_set_boolean(val, self->is_shuffle_enabled);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_playlist_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoPlaylist * self = KOTO_PLAYLIST(obj);


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
		case PROP_IS_SHUFFLE_ENABLED:
			self->is_shuffle_enabled = g_value_get_boolean(val);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_playlist_init(KotoPlaylist * self) {
	self->current_position = -1; // Default to -1 so first time incrementing puts it at 0
	self->current_uuid = NULL;
	self->model = KOTO_PREFERRED_MODEL_TYPE_DEFAULT; // Default to default model
	self->is_shuffle_enabled = FALSE;
	self->ephemeral = FALSE;
	self->finalized = FALSE;

	self->tracks = g_queue_new(); // Set as an empty GQueue
	self->played_tracks = g_queue_new(); // Set as an empty GQueue
	self->sorted_tracks = g_queue_new(); // Set as an empty GQueue
	self->store = g_list_store_new(KOTO_TYPE_TRACK);
}

void koto_playlist_add_to_played_tracks(
	KotoPlaylist * self,
	gchar * uuid
) {
	if (g_queue_index(self->played_tracks, uuid) != -1) { // Already added
		return;
	}

	g_queue_push_tail(self->played_tracks, uuid); // Add to end
}

void koto_playlist_add_track(
	KotoPlaylist * self,
	KotoTrack * track,
	gboolean current,
	gboolean commit_to_table
) {
	koto_playlist_add_track_by_uuid(self, koto_track_get_uuid(track), current, commit_to_table);
}

void koto_playlist_add_track_by_uuid(
	KotoPlaylist * self,
	gchar * uuid,
	gboolean current,
	gboolean commit_to_table
) {
	KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, uuid); // Get the track

	if (!KOTO_IS_TRACK(track)) {
		return;
	}

	GList * found_tracks_uuids = g_queue_find_custom(self->tracks, uuid, koto_playlist_compare_track_uuids);

	if (found_tracks_uuids != NULL) { // Is somewhere in the tracks already
		g_list_free(found_tracks_uuids);
		return;
	}

	g_list_free(found_tracks_uuids);

	g_queue_push_tail(self->tracks, uuid); // Prepend the UUID to the tracks
	g_queue_push_tail(self->sorted_tracks, uuid); // Also add to our sorted tracks
	g_list_store_append(self->store, track); // Add to the store

	if (self->finalized) { // Is already finalized
		koto_playlist_apply_model(self, self->model); // Re-apply our current model to ensure our "sorted tracks" is sorted, as is our GLIstStore used in the playlist page
	}

	if (commit_to_table) {
		koto_track_save_to_playlist(track, self->uuid, (g_strcmp0(self->current_uuid, uuid) == 0) ? 1 : 0); // Call to save the playlist to the track
	}

	if (current && (g_queue_get_length(self->tracks) > 1)) { // Is current and NOT the first item
		self->current_uuid = uuid; // Mark this as current UUID
	}

	g_signal_emit(
		self,
		playlist_signals[SIGNAL_TRACK_ADDED],
		0,
		uuid
	);
}

void koto_playlist_apply_model(
	KotoPlaylist * self,
	KotoPreferredModelType preferred_model
) {
	GList * sort_user_data = NULL;

	sort_user_data = g_list_prepend(sort_user_data, GUINT_TO_POINTER(preferred_model)); // Prepend our preferred model first
	sort_user_data = g_list_prepend(sort_user_data, self); // Prepend ourself

	g_queue_sort(self->sorted_tracks, koto_playlist_model_sort_by_uuid, sort_user_data); // Sort tracks, which is by UUID
	g_list_store_sort(self->store, koto_playlist_model_sort_by_track, sort_user_data); // Sort tracks by indexed tracks

	self->model = preferred_model; // Update our preferred model
}

void koto_playlist_commit(KotoPlaylist * self) {
	if (self->ephemeral) { // Temporary playlist
		return;
	}

	gchar * commit_op = g_strdup_printf(
		"INSERT INTO playlist_meta(id, name, art_path, preferred_model)"
		"VALUES('%s', quote(\"%s\"), quote(\"%s\"), 0)"
		"ON CONFLICT(id) DO UPDATE SET name=excluded.name, art_path=excluded.art_path;",
		self->uuid,
		self->name,
		self->art_path
	);

	gchar * commit_op_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, commit_op, 0, 0, &commit_op_errmsg);


	if (rc != SQLITE_OK) {
		g_warning("Failed to save playlist: %s", commit_op_errmsg);
	} else { // Successfully saved our playlist
		g_queue_foreach(self->tracks, koto_playlist_commit_tracks, self); // Iterate over each track to save it
	}

	g_free(commit_op);
	g_free(commit_op_errmsg);
}

void koto_playlist_commit_tracks(
	gpointer data,
	gpointer user_data
) {
	KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, data); // Get the track


	if (track == NULL) { // Not a track
		KotoPlaylist * self = user_data;
		gchar * playlist_uuid = self->uuid; // Get the playlist UUID

		gchar * current_track = g_queue_peek_nth(self->tracks, self->current_position); // Get the UUID of the current track
		//koto_track_save_to_playlist(track, playlist_uuid, (data == current_track) ? 1 : 0); // Call to save the playlist to the track
		g_free(playlist_uuid);
		g_free(current_track);
	}
}

gint koto_playlist_compare_track_uuids(
	gconstpointer a,
	gconstpointer b
) {
	return g_strcmp0(a, b);
}

gchar * koto_playlist_get_artwork(KotoPlaylist * self) {
	return (self->art_path == NULL) ? NULL : g_strdup(self->art_path); // Return a duplicate of our art path
}

KotoPreferredModelType koto_playlist_get_current_model(KotoPlaylist * self) {
	return self->model;
}

guint koto_playlist_get_current_position(KotoPlaylist * self) {
	return self->current_position;
}

gboolean koto_playlist_get_is_finalized(KotoPlaylist * self) {
	return self->finalized;
}

guint koto_playlist_get_length(KotoPlaylist * self) {
	return g_queue_get_length(self->tracks); // Get the length of the tracks
}

gchar * koto_playlist_get_name(KotoPlaylist * self) {
	return (self->name == NULL) ? NULL : g_strdup(self->name);
}

gint koto_playlist_get_position_of_track(
	KotoPlaylist * self,
	KotoTrack * track
) {
	if (!KOTO_IS_PLAYLIST(self)) {
		return -1;
	}

	if (!G_IS_LIST_STORE(self->store)) {
		return -1;
	}

	if (!KOTO_IS_TRACK(track)) {
		return -1;
	}

	gint position = -1;
	guint found_pos = 0;


	if (g_list_store_find(self->store, track, &found_pos)) { // Found the item
		position = (gint) found_pos; // Cast our found position from guint to gint
	}

	return position;
}

gchar * koto_playlist_get_random_track(KotoPlaylist * self) {
	gchar * track_uuid = NULL;
	guint tracks_len = g_queue_get_length(self->sorted_tracks);

	if (tracks_len == g_queue_get_length(self->played_tracks)) { // Played all tracks
		track_uuid = g_list_nth_data(self->sorted_tracks->head, 0); // Get the first
		g_queue_clear(self->played_tracks); // Clear our played tracks
	} else { // Have not played all tracks
		GRand* rando_calrissian = g_rand_new(); // Create a new RNG
		guint attempt = 0;

		while (track_uuid == NULL) { // Haven't selected a track yet
			attempt++;
			gint32 selected_item = g_rand_int_range(rando_calrissian, 0, (gint32) tracks_len);
			gchar * selected_track = g_queue_peek_nth(self->sorted_tracks, (guint) selected_item); // Get the UUID of the selected item

			if (g_queue_index(self->played_tracks, selected_track) == -1) { // Haven't played the track
				self->current_position = (gint) selected_item;
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

GListStore * koto_playlist_get_store(KotoPlaylist * self) {
	return self->store;
}

GQueue * koto_playlist_get_tracks(KotoPlaylist * self) {
	return self->tracks;
}

gchar * koto_playlist_get_uuid(KotoPlaylist * self) {
	return g_strdup(self->uuid);
}

gchar * koto_playlist_go_to_next(KotoPlaylist * self) {
	if (!KOTO_IS_PLAYLIST(self)) {
		return NULL;
	}

	if (self->is_shuffle_enabled) { // Shuffling enabled
		gchar * random_track_uuid = koto_playlist_get_random_track(self); // Get a random track
		koto_playlist_add_to_played_tracks(self, random_track_uuid);
		return random_track_uuid;
	}

	if (!koto_utils_is_string_valid(self->current_uuid)) { // No valid UUID yet
		self->current_position++;
	} else { // Have a UUID currently
		KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, self->current_uuid);

		if (!KOTO_IS_TRACK(track)) {
			return NULL;
		}

		gint pos_of_song = koto_playlist_get_position_of_track(self, track); // Get the position of the current track based on the current model

		if ((guint) pos_of_song == (g_queue_get_length(self->sorted_tracks) - 1)) { // At end
			return NULL;
		}

		self->current_position = pos_of_song + 1; // Increment our position based on position of song
	}

	self->current_uuid = g_queue_peek_nth(self->sorted_tracks, self->current_position);
	koto_playlist_add_to_played_tracks(self, self->current_uuid);

	return self->current_uuid;
}

gchar * koto_playlist_go_to_previous(KotoPlaylist * self) {
	if (self->is_shuffle_enabled) { // Shuffling enabled
		return koto_playlist_get_random_track(self); // Get a random track
	}

	if (!koto_utils_is_string_valid(self->current_uuid)) { // No valid UUID
		return NULL;
	}

	KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, self->current_uuid);


	if (!KOTO_IS_TRACK(track)) {
		return NULL;
	}

	gint pos_of_song = koto_playlist_get_position_of_track(self, track); // Get the position of the current track based on the current model


	if (pos_of_song == 0) {
		return NULL;
	}

	self->current_position = pos_of_song - 1; // Decrement our position based on position of song
	self->current_uuid = g_queue_peek_nth(self->sorted_tracks, self->current_position);

	return self->current_uuid;
}

void koto_playlist_mark_as_finalized(KotoPlaylist * self) {
	if (self->finalized) { // Already finalized
		return;
	}

	self->finalized = TRUE;
	koto_playlist_apply_model(self, self->model); // Re-apply our model to enforce mass sort

	g_signal_emit(
		self,
		playlist_signals[SIGNAL_TRACK_LOAD_FINALIZED],
		0
	);
}

gint koto_playlist_model_sort_by_uuid(
	gconstpointer first_item,
	gconstpointer second_item,
	gpointer data_list
) {
	KotoTrack * first_track = koto_cartographer_get_track_by_uuid(koto_maps, (gchar*) first_item);
	KotoTrack * second_track = koto_cartographer_get_track_by_uuid(koto_maps, (gchar*) second_item);


	return koto_playlist_model_sort_by_track(first_track, second_track, data_list);
}

gint koto_playlist_model_sort_by_track(
	gconstpointer first_item,
	gconstpointer second_item,
	gpointer data_list
) {
	KotoTrack * first_track = (KotoTrack*) first_item;
	KotoTrack * second_track = (KotoTrack*) second_item;

	GList* ptr_list = data_list;
	KotoPlaylist * self = g_list_nth_data(ptr_list, 0); // First item in the GPtrArray is a pointer to our playlist
	KotoPreferredModelType model = GPOINTER_TO_UINT(g_list_nth_data(ptr_list, 1)); // Second item in the GPtrArray is a pointer to our KotoPreferredModelType


	if (
		(model == KOTO_PREFERRED_MODEL_TYPE_DEFAULT) || // Newest first model
		(model == KOTO_PREFERRED_MODEL_TYPE_OLDEST_FIRST) // Oldest first
	) {
		gint first_track_pos = g_queue_index(self->tracks, koto_track_get_uuid(first_track));
		gint second_track_pos = g_queue_index(self->tracks, koto_track_get_uuid(second_track));

		if (first_track_pos == -1) { // First track isn't in tracks
			return 1;
		}

		if (second_track_pos == -1) { // Second track isn't in tracks
			return -1;
		}

		if (model == KOTO_PREFERRED_MODEL_TYPE_DEFAULT) { // Newest first
			return (first_track_pos < second_track_pos) ? 1 : -1; // Display first at end, not beginning
		} else {
			return (first_track_pos < second_track_pos) ? -1 : 1; // Display at beginning, not end
		}
	}

	if (model == KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ALBUM) { // Sort by album name
		gchar * first_album_uuid = NULL;
		gchar * second_album_uuid = NULL;

		g_object_get(
			first_track,
			"album-uuid",
			&first_album_uuid,
			NULL
		);

		g_object_get(
			second_track,
			"album-uuid",
			&second_album_uuid,
			NULL
		);

		if (g_strcmp0(first_album_uuid, second_album_uuid) == 0) { // Identical albums
			g_free(first_album_uuid);
			g_free(second_album_uuid);
			return 0; // Don't get too granular, just consider them equal
		}

		KotoAlbum * first_album = koto_cartographer_get_album_by_uuid(koto_maps, first_album_uuid);
		KotoAlbum * second_album = koto_cartographer_get_album_by_uuid(koto_maps, second_album_uuid);

		g_free(first_album_uuid);
		g_free(second_album_uuid);

		if (!KOTO_IS_ALBUM(first_album) && !KOTO_IS_ALBUM(second_album)) { // Neither are valid albums
			return 0; // Just consider them as equal
		}

		return g_utf8_collate(koto_album_get_album_name(first_album), koto_album_get_album_name(second_album));
	}

	if (model == KOTO_PREFERRED_MODEL_TYPE_SORT_BY_ARTIST) { // Sort by artist name
		gchar * first_artist_uuid = NULL;
		gchar * second_artist_uuid = NULL;

		g_object_get(
			first_track,
			"artist-uuid",
			&first_artist_uuid,
			NULL
		);

		g_object_get(
			second_track,
			"artist-uuid",
			&second_artist_uuid,
			NULL
		);

		KotoArtist * first_artist = koto_cartographer_get_artist_by_uuid(koto_maps, first_artist_uuid);
		KotoArtist * second_artist = koto_cartographer_get_artist_by_uuid(koto_maps, second_artist_uuid);

		g_free(first_artist_uuid);
		g_free(second_artist_uuid);

		if (!KOTO_IS_ARTIST(first_artist) && !KOTO_IS_ARTIST(second_artist)) { // Neither are valid artists
			return 0; // Just consider them as equal
		}

		return g_utf8_collate(koto_artist_get_name(first_artist), koto_artist_get_name(second_artist));
	}

	if (model == KOTO_PREFERRED_MODEL_TYPE_SORT_BY_TRACK_NAME) { // Track name
		gchar * first_track_name = NULL;
		gchar * second_track_name = NULL;

		g_object_get(
			first_track,
			"parsed-name",
			&first_track_name,
			NULL
		);

		g_object_get(
			second_track,
			"parsed-name",
			&second_track_name,
			NULL
		);

		gint ret = g_utf8_collate(first_track_name, second_track_name);
		g_free(first_track_name);
		g_free(second_track_name);

		return ret;
	}

	return 0;
}

void koto_playlist_remove_from_played_tracks(
	KotoPlaylist * self,
	gchar * uuid
) {
	g_queue_remove(self->played_tracks, uuid);
}

void koto_playlist_remove_track_by_uuid(
	KotoPlaylist * self,
	gchar * uuid
) {
	if (!KOTO_IS_PLAYLIST(self)) {
		return;
	}

	gint file_index = g_queue_index(self->tracks, uuid); // Get the position of this uuid

	if (file_index != -1) { // Have in tracks
		g_queue_pop_nth(self->tracks, file_index); // Remove nth where it is the file index
	}

	gint file_index_in_sorted = g_queue_index(self->sorted_tracks, uuid); // Get position in sorted tracks

	if (file_index_in_sorted != -1) { // Have in sorted tracks
		g_queue_pop_nth(self->sorted_tracks, file_index_in_sorted); // Remove nth where it is the index in sorted tracks
	}

	KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, uuid); // Get the track


	if (!KOTO_IS_TRACK(track)) { // Is not a track
		return;
	}

	guint position = 0;


	if (g_list_store_find(self->store, track, &position)) { // Got the position
		g_list_store_remove(self->store, position); // Remove from the store
	}

	koto_track_remove_from_playlist(track, self->uuid);

	g_signal_emit(
		self,
		playlist_signals[SIGNAL_TRACK_REMOVED],
		0,
		uuid
	);
}

void koto_playlist_set_artwork(
	KotoPlaylist * self,
	const gchar * path
) {
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

	const gchar * mime_type = magic_file(cookie, path); // Get the mimetype for this file


	if ((mime_type == NULL) || !g_str_has_prefix(mime_type, "image/")) { // Failed to get our mimetype or not an image
		goto free_cookie;
	}

	if (self->art_path != NULL) { // art path is set
		g_free(self->art_path); // Free the current path
	}

	self->art_path = g_strdup(path); // Update our art path to a duplicate of provided path

	if (self->finalized) { // Has already been loaded
		g_signal_emit(
			self,
			playlist_signals[SIGNAL_MODIFIED],
			0
		);
	}

free_cookie:
	magic_close(cookie); // Close and free the cookie to the cookie monster
}

void koto_playlist_set_name(
	KotoPlaylist * self,
	const gchar * name
) {
	if (name == NULL) {
		return;
	}

	if (self->name != NULL) { // Have a name allocated already
		g_free(self->name); // Free the name
	}

	self->name = g_strdup(name);

	if (self->finalized) { // Has already been loaded
		g_signal_emit(
			self,
			playlist_signals[SIGNAL_MODIFIED],
			0
		);
	}
}

void koto_playlist_set_position(
	KotoPlaylist * self,
	gint position
) {
	self->current_position = position;
}

void koto_playlist_set_track_as_current(
	KotoPlaylist * self,
	gchar * track_uuid
) {
	gint position_of_track = g_queue_index(self->sorted_tracks, track_uuid); // Get the position of the UUID in our tracks

	if (position_of_track != -1) { // In tracks
		self->current_uuid = track_uuid;
		self->current_position = position_of_track;
	}
}

void koto_playlist_set_uuid(
	KotoPlaylist * self,
	const gchar * uuid
) {
	if (uuid == NULL) { // No actual UUID
		return;
	}

	if (self->uuid != NULL) { // Have a UUID allocated already
		return; // Do not allow changing the UUID
	}

	self->uuid = g_strdup(uuid); // Set the new UUID
}

void koto_playlist_tracks_queue_push_to_store(
	gpointer data,
	gpointer user_data
) {
	gchar * track_uuid = (gchar*) data;
	KotoTrack * track = koto_cartographer_get_track_by_uuid(koto_maps, track_uuid);


	if (!KOTO_IS_TRACK(track)) { // Not a track
		return;
	}

	g_list_store_append(G_LIST_STORE(user_data), track);
}

void koto_playlist_unmap(KotoPlaylist * self) {
	koto_cartographer_remove_playlist_by_uuid(koto_maps, self->uuid); // Remove from our cartographer
}

KotoPlaylist * koto_playlist_new() {
	return g_object_new(
		KOTO_TYPE_PLAYLIST,
		"uuid",
		g_uuid_string_random(),
		NULL
	);
}

KotoPlaylist * koto_playlist_new_with_uuid(const gchar * uuid) {
	return g_object_new(
		KOTO_TYPE_PLAYLIST,
		"uuid",
		uuid,
		NULL
	);
}
