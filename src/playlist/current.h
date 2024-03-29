/* current.h
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
#include "playlist.h"

G_BEGIN_DECLS

/**
 * Type Definition
 **/

#define KOTO_TYPE_CURRENT_PLAYLIST koto_current_playlist_get_type()
#define KOTO_CURRENT_PLAYLIST(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), KOTO_TYPE_CURRENT_PLAYLIST, KotoCurrentPlaylist))
#define KOTO_IS_CURRENT_PLAYLIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_CURRENT_PLAYLIST))

typedef struct _KotoCurrentPlaylist KotoCurrentPlaylist;
typedef struct _KotoCurrentPlaylistClass KotoCurrentPlaylistClass;

GLIB_AVAILABLE_IN_ALL
GType koto_current_playlist_get_type(void) G_GNUC_CONST;

/**
 * Current Playlist Functions
 **/

KotoCurrentPlaylist * koto_current_playlist_new();

KotoPlaylist * koto_current_playlist_get_playlist(KotoCurrentPlaylist * self);

void koto_current_playlist_save_playlist_state(KotoCurrentPlaylist * self);

void koto_current_playlist_set_playlist(
	KotoCurrentPlaylist * self,
	KotoPlaylist * playlist,
	gboolean play_immediately,
	gboolean play_current
);

G_END_DECLS
