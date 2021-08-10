/* playlist.h
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

#pragma once
#include <glib-2.0/glib.h>
#include <glib-2.0/gio/gio.h>
#include "../indexer/structs.h"

G_BEGIN_DECLS

typedef enum {
	KOTO_PREFERRED_PLAYLIST_SORT_TYPE_DEFAULT, // Considered to be newest first
	KOTO_PREFERRED_PLAYLIST_SORT_TYPE_OLDEST_FIRST,
	KOTO_PREFERRED_PLAYLIST_SORT_TYPE_SORT_BY_ALBUM,
	KOTO_PREFERRED_PLAYLIST_SORT_TYPE_SORT_BY_ARTIST,
	KOTO_PREFERRED_PLAYLIST_SORT_TYPE_SORT_BY_TRACK_NAME,
	KOTO_PREFERRED_PLAYLIST_SORT_TYPE_SORT_BY_TRACK_POS
} KotoPreferredPlaylistSortType;

/**
 * Type Definition
 **/

#define KOTO_TYPE_PLAYLIST koto_playlist_get_type()
#define KOTO_PLAYLIST(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), KOTO_TYPE_PLAYLIST, KotoPlaylist))
#define KOTO_IS_PLAYLIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_PLAYLIST))

typedef struct _KotoPlaylist KotoPlaylist;
typedef struct _KotoPlaylistClass KotoPlaylistClass;

GLIB_AVAILABLE_IN_ALL
GType koto_playlist_get_type(void) G_GNUC_CONST;

/**
 * Playlist Functions
 **/

KotoPlaylist * koto_playlist_new();

KotoPlaylist * koto_playlist_new_with_uuid(const gchar * uuid);

void koto_playlist_add_to_played_tracks(
	KotoPlaylist * self,
	gchar * uuid
);

void koto_playlist_add_track(
	KotoPlaylist * self,
	KotoTrack * track,
	gboolean current,
	gboolean commit_to_table
);

void koto_playlist_apply_model(
	KotoPlaylist * self,
	KotoPreferredPlaylistSortType preferred_model
);

void koto_playlist_commit(KotoPlaylist * self);

void koto_playlist_emit_modified(KotoPlaylist * self);

gchar * koto_playlist_get_artwork(KotoPlaylist * self);

KotoPreferredPlaylistSortType koto_playlist_get_current_model(KotoPlaylist * self);

gint koto_playlist_get_current_position(KotoPlaylist * self);

KotoTrack * koto_playlist_get_current_track(KotoPlaylist * self);

guint koto_playlist_get_length(KotoPlaylist * self);

gboolean koto_playlist_get_is_finalized(KotoPlaylist * self);

gboolean koto_playlist_get_is_hidden(KotoPlaylist * self);

gchar * koto_playlist_get_name(KotoPlaylist * self);

gint koto_playlist_get_position_of_track(
	KotoPlaylist * self,
	KotoTrack * track
);

GListStore * koto_playlist_get_store(KotoPlaylist * self);

GQueue * koto_playlist_get_tracks(KotoPlaylist * self);

gchar * koto_playlist_get_uuid(KotoPlaylist * self);

gchar * koto_playlist_go_to_next(KotoPlaylist * self);

gchar * koto_playlist_go_to_previous(KotoPlaylist * self);

void koto_playlist_mark_as_finalized(KotoPlaylist * self);

gint koto_playlist_model_sort_by_uuid(
	gconstpointer first_item,
	gconstpointer second_item,
	gpointer data_list
);

gint koto_playlist_model_sort_by_track(
	gconstpointer first_item,
	gconstpointer second_item,
	gpointer model_ptr
);

void koto_playlist_remove_from_played_tracks(
	KotoPlaylist * self,
	gchar * uuid
);

void koto_playlist_remove_track_by_uuid(
	KotoPlaylist * self,
	gchar * uuid
);

void koto_playlist_set_album_uuid(
	KotoPlaylist * self,
	const gchar * album_uuid
);

void koto_playlist_set_artwork(
	KotoPlaylist * self,
	const gchar * path
);

void koto_playlist_save_current_playback_state(KotoPlaylist * self);

void koto_playlist_set_name(
	KotoPlaylist * self,
	const gchar * name
);

void koto_playlist_set_position(
	KotoPlaylist * self,
	gint position
);

void koto_playlist_set_track_as_current(
	KotoPlaylist * self,
	gchar * track_uuid
);

void koto_playlist_set_uuid(
	KotoPlaylist * self,
	const gchar * uuid
);

void koto_playlist_tracks_queue_push_to_store(
	gpointer data,
	gpointer user_data
);

void koto_playlist_unmap(KotoPlaylist * self);

G_END_DECLS
