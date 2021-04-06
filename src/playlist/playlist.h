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
#include <glib-2.0/glib-object.h>
#include "../indexer/structs.h"

G_BEGIN_DECLS

/**
 * Type Definition
**/

#define KOTO_TYPE_PLAYLIST koto_playlist_get_type()
G_DECLARE_FINAL_TYPE(KotoPlaylist, koto_playlist, KOTO, PLAYLIST, GObject);
#define KOTO_IS_PLAYLIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_PLAYLIST))

/**
 * Playlist Functions
**/

KotoPlaylist* koto_playlist_new();
KotoPlaylist* koto_playlist_new_with_uuid(const gchar *uuid);
void koto_playlist_add_to_played_tracks(KotoPlaylist *self, gchar *uuid);
void koto_playlist_add_track(KotoPlaylist *self, KotoIndexedTrack *track);
void koto_playlist_add_track_by_uuid(KotoPlaylist *self, const gchar *uuid);
void koto_playlist_commit(KotoPlaylist *self);
void koto_playlist_commit_tracks(gpointer data, gpointer user_data);
gchar* koto_playlist_get_artwork(KotoPlaylist *self);
guint koto_playlist_get_current_position(KotoPlaylist *self);
gchar* koto_playlist_get_current_uuid(KotoPlaylist *self);
guint koto_playlist_get_length(KotoPlaylist *self);
gchar* koto_playlist_get_name(KotoPlaylist *self);
GQueue* koto_playlist_get_tracks(KotoPlaylist *self);
gchar* koto_playlist_get_uuid(KotoPlaylist *self);
gchar* koto_playlist_go_to_next(KotoPlaylist *self);
gchar* koto_playlist_go_to_previous(KotoPlaylist *self);
void koto_playlist_remove_from_played_tracks(KotoPlaylist *self, gchar *uuid);
void koto_playlist_remove_track(KotoPlaylist *self, KotoIndexedTrack *track);
void koto_playlist_remove_track_by_uuid(KotoPlaylist *self, gchar *uuid);
void koto_playlist_set_artwork(KotoPlaylist *self, const gchar *path);
void koto_playlist_save_state(KotoPlaylist *self);
void koto_playlist_set_name(KotoPlaylist *self, const gchar *name);
void koto_playlist_set_position(KotoPlaylist *self, guint pos);
void koto_playlist_set_uuid(KotoPlaylist *self, const gchar *uuid);
void koto_playlist_unmap(KotoPlaylist *self);

G_END_DECLS
