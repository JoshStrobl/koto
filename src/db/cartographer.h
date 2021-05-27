/* cartographer.h
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
#include "../playlist/playlist.h"

G_BEGIN_DECLS

/**
 * Type Definition
 **/

#define KOTO_TYPE_CARTOGRAPHER koto_cartographer_get_type()

typedef struct _KotoCartographer KotoCartographer;
typedef struct _KotoCartographerClass KotoCartographerClass;

GLIB_AVAILABLE_IN_ALL
GType koto_cartographer_get_type(void) G_GNUC_CONST;

/**
 * Cartographer Functions
 **/

KotoCartographer * koto_cartographer_new();

void koto_cartographer_add_album(
	KotoCartographer * self,
	KotoAlbum * album
);

void koto_cartographer_add_artist(
	KotoCartographer * self,
	KotoArtist * artist
);

void koto_cartographer_add_playlist(
	KotoCartographer * self,
	KotoPlaylist * playlist
);

void koto_cartographer_add_track(
	KotoCartographer * self,
	KotoTrack * track
);

void koto_cartographer_emit_playlist_added(
	KotoPlaylist * playlist,
	KotoCartographer * self
);

KotoAlbum * koto_cartographer_get_album_by_uuid(
	KotoCartographer * self,
	gchar * album_uuid
);

GHashTable * koto_cartographer_get_artists(KotoCartographer * self);

KotoArtist * koto_cartographer_get_artist_by_name(
	KotoCartographer * self,
	gchar * artist_name
);

KotoArtist * koto_cartographer_get_artist_by_uuid(
	KotoCartographer * self,
	gchar * artist_uuid
);

KotoPlaylist * koto_cartographer_get_playlist_by_uuid(
	KotoCartographer * self,
	gchar * playlist_uuid
);

GHashTable * koto_cartographer_get_playlists(KotoCartographer * self);

KotoTrack * koto_cartographer_get_track_by_uuid(
	KotoCartographer * self,
	gchar * track_uuid
);

gboolean koto_cartographer_has_album(
	KotoCartographer * self,
	KotoAlbum * album
);

gboolean koto_cartographer_has_album_by_uuid(
	KotoCartographer * self,
	gchar * album_uuid
);

gboolean koto_cartographer_has_artist(
	KotoCartographer * self,
	KotoArtist * artist
);

gboolean koto_cartographer_has_artist_by_uuid(
	KotoCartographer * self,
	gchar * artist_uuid
);

gboolean koto_cartographer_has_playlist(
	KotoCartographer * self,
	KotoPlaylist * playlist
);

gboolean koto_cartographer_has_playlist_by_uuid(
	KotoCartographer * self,
	gchar * playlist_uuid
);

gboolean koto_cartographer_has_track(
	KotoCartographer * self,
	KotoTrack * track
);

gboolean koto_cartographer_has_track_by_uuid(
	KotoCartographer * self,
	gchar * track_uuid
);

void koto_cartographer_remove_album(
	KotoCartographer * self,
	KotoAlbum * album
);

void koto_cartographer_remove_album_by_uuid(
	KotoCartographer * self,
	gchar * album_uuid
);

void koto_cartographer_remove_artist(
	KotoCartographer * self,
	KotoArtist * artist
);

void koto_cartographer_remove_artist_by_uuid(
	KotoCartographer * self,
	gchar * artist_uuid
);

void koto_cartographer_remove_playlist(
	KotoCartographer * self,
	KotoPlaylist * playlist
);

void koto_cartographer_remove_playlist_by_uuid(
	KotoCartographer * self,
	gchar * playlist_uuid
);

void koto_cartographer_remove_track(
	KotoCartographer * self,
	KotoTrack * track
);

void koto_cartographer_remove_track_by_uuid(
	KotoCartographer * self,
	gchar * track_uuid
);

G_END_DECLS
