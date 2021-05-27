/* cartographer.c
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
#include "../koto-utils.h"
#include "cartographer.h"

enum {
	SIGNAL_ALBUM_ADDED,
	SIGNAL_ALBUM_REMOVED,
	SIGNAL_ARTIST_ADDED,
	SIGNAL_ARTIST_REMOVED,
	SIGNAL_PLAYLIST_ADDED,
	SIGNAL_PLAYLIST_REMOVED,
	SIGNAL_TRACK_ADDED,
	SIGNAL_TRACK_REMOVED,
	N_SIGNALS
};

static guint cartographer_signals[N_SIGNALS] = {
	0
};

struct _KotoCartographer {
	GObject parent_instance;

	GHashTable * albums;
	GHashTable * artists;
	GHashTable * artists_name_to_uuid;
	GHashTable * playlists;
	GHashTable * tracks;
};

struct _KotoCartographerClass {
	GObjectClass parent_class;

	void (* album_added) (
		KotoCartographer * cartographer,
		KotoAlbum * album
	);
	void (* album_removed) (
		KotoCartographer * cartographer,
		KotoAlbum * album
	);
	void (* artist_added) (
		KotoCartographer * cartographer,
		KotoArtist * artist
	);
	void (* artist_removed) (
		KotoCartographer * cartographer,
		KotoArtist * artist
	);
	void (* playlist_added) (
		KotoCartographer * cartographer,
		KotoPlaylist * playlist
	);
	void (* playlist_removed) (
		KotoCartographer * cartographer,
		KotoPlaylist * playlist
	);
	void (* track_added) (
		KotoCartographer * cartographer,
		KotoTrack * track
	);
	void (* track_removed) (
		KotoCartographer * cartographer,
		KotoTrack * track
	);
};

G_DEFINE_TYPE(KotoCartographer, koto_cartographer, G_TYPE_OBJECT);

KotoCartographer * koto_maps = NULL;

static void koto_cartographer_class_init(KotoCartographerClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);

	cartographer_signals[SIGNAL_ALBUM_ADDED] = g_signal_new(
		"album-added",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoCartographerClass, album_added),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_ALBUM
	);

	cartographer_signals[SIGNAL_ALBUM_REMOVED] = g_signal_new(
		"album-removed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoCartographerClass, album_removed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		G_TYPE_CHAR
	);

	cartographer_signals[SIGNAL_ARTIST_ADDED] = g_signal_new(
		"artist-added",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoCartographerClass, artist_added),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_ARTIST
	);

	cartographer_signals[SIGNAL_ARTIST_REMOVED] = g_signal_new(
		"artist-removed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoCartographerClass, artist_removed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		G_TYPE_CHAR
	);

	cartographer_signals[SIGNAL_PLAYLIST_ADDED] = g_signal_new(
		"playlist-added",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoCartographerClass, playlist_added),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_PLAYLIST
	);

	cartographer_signals[SIGNAL_PLAYLIST_REMOVED] = g_signal_new(
		"playlist-removed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoCartographerClass, playlist_removed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		G_TYPE_CHAR
	);

	cartographer_signals[SIGNAL_TRACK_ADDED] = g_signal_new(
		"track-added",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoCartographerClass, track_added),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		KOTO_TYPE_TRACK
	);

	cartographer_signals[SIGNAL_TRACK_REMOVED] = g_signal_new(
		"track-removed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoCartographerClass, track_removed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		1,
		G_TYPE_CHAR
	);
}

static void koto_cartographer_init(KotoCartographer * self) {
	self->albums = g_hash_table_new(g_str_hash, g_str_equal);
	self->artists = g_hash_table_new(g_str_hash, g_str_equal);
	self->artists_name_to_uuid = g_hash_table_new(g_str_hash, g_str_equal);
	self->playlists = g_hash_table_new(g_str_hash, g_str_equal);
	self->tracks = g_hash_table_new(g_str_hash, g_str_equal);
}

void koto_cartographer_add_album(
	KotoCartographer * self,
	KotoAlbum * album
) {
	gchar * album_uuid = NULL;

	g_object_get(album, "uuid", &album_uuid, NULL);

	if ((album_uuid == NULL) || koto_cartographer_has_album_by_uuid(self, album_uuid)) { // Have the album or invalid UUID
		return;
	}

	g_hash_table_replace(self->albums, album_uuid, album);

	g_signal_emit(
		self,
		cartographer_signals[SIGNAL_ALBUM_ADDED],
		0,
		album
	);
}

void koto_cartographer_add_artist(
	KotoCartographer * self,
	KotoArtist * artist
) {
	if (!KOTO_IS_ARTIST(artist)) { // Not an artist
		return;
	}

	gchar * artist_uuid = NULL;

	g_object_get(artist, "uuid", &artist_uuid, NULL);

	if ((artist_uuid == NULL) || koto_cartographer_has_artist_by_uuid(self, artist_uuid)) { // Have the artist or invalid UUID
		return;
	}

	g_hash_table_replace(self->artists_name_to_uuid, koto_artist_get_name(artist), koto_artist_get_uuid(artist)); // Add the UUID as a value with the key being the name of the artist

	g_hash_table_replace(self->artists, artist_uuid, artist);

	g_signal_emit(
		self,
		cartographer_signals[SIGNAL_ARTIST_ADDED],
		0,
		artist
	);
}

void koto_cartographer_add_playlist(
	KotoCartographer * self,
	KotoPlaylist * playlist
) {
	gchar * playlist_uuid = NULL;

	g_object_get(playlist, "uuid", &playlist_uuid, NULL);

	if ((playlist_uuid == NULL) || koto_cartographer_has_playlist_by_uuid(self, playlist_uuid)) { // Have the playlist or invalid UUID
		return;
	}

	g_hash_table_replace(self->playlists, playlist_uuid, playlist);

	if (koto_playlist_get_is_finalized(playlist)) { // Already finalized
		koto_cartographer_emit_playlist_added(playlist, self); // Emit playlist-added immediately
	} else { // Not finalized
		g_signal_connect(playlist, "track-load-finalized", G_CALLBACK(koto_cartographer_emit_playlist_added), self);
	}
}

void koto_cartographer_emit_playlist_added(
	KotoPlaylist * playlist,
	KotoCartographer * self
) {
	g_signal_emit(
		self,
		cartographer_signals[SIGNAL_PLAYLIST_ADDED],
		0,
		playlist
	);
}

void koto_cartographer_add_track(
	KotoCartographer * self,
	KotoTrack * track
) {
	gchar * track_uuid = NULL;

	g_object_get(track, "uuid", &track_uuid, NULL);

	if ((track_uuid == NULL) || koto_cartographer_has_track_by_uuid(self, track_uuid)) { // Have the track or invalid UUID
		return;
	}

	g_hash_table_replace(self->tracks, track_uuid, track);

	g_signal_emit(
		self,
		cartographer_signals[SIGNAL_TRACK_ADDED],
		0,
		track
	);
}

KotoAlbum * koto_cartographer_get_album_by_uuid(
	KotoCartographer * self,
	gchar * album_uuid
) {
	return g_hash_table_lookup(self->albums, album_uuid);
}

GHashTable * koto_cartographer_get_artists(KotoCartographer * self) {
	return self->artists;
}

KotoArtist * koto_cartographer_get_artist_by_name(
	KotoCartographer * self,
	gchar * artist_name
) {
	if (!koto_utils_is_string_valid(artist_name)) { // Not a valid name
		return NULL;
	}

	return koto_cartographer_get_artist_by_uuid(self, g_hash_table_lookup(self->artists_name_to_uuid, artist_name));
}

KotoArtist * koto_cartographer_get_artist_by_uuid(
	KotoCartographer * self,
	gchar * artist_uuid
) {
	if (!koto_utils_is_string_valid(artist_uuid)) {
		return NULL;
	}

	return g_hash_table_lookup(self->artists, artist_uuid);
}

GHashTable * koto_cartographer_get_playlists(KotoCartographer * self) {
	return self->playlists;
}

KotoPlaylist * koto_cartographer_get_playlist_by_uuid(
	KotoCartographer * self,
	gchar * playlist_uuid
) {
	return g_hash_table_lookup(self->playlists, playlist_uuid);
}

KotoTrack * koto_cartographer_get_track_by_uuid(
	KotoCartographer * self,
	gchar * track_uuid
) {
	return g_hash_table_lookup(self->tracks, track_uuid);
}

gboolean koto_cartographer_has_album(
	KotoCartographer * self,
	KotoAlbum * album
) {
	gchar * album_uuid = NULL;

	g_object_get(album, "uuid", &album_uuid, NULL);
	return koto_cartographer_has_album_by_uuid(self, album_uuid);
}

gboolean koto_cartographer_has_album_by_uuid(
	KotoCartographer * self,
	gchar * album_uuid
) {
	if (album_uuid == NULL) {
		return FALSE;
	}

	return g_hash_table_contains(self->albums, album_uuid);
}

gboolean koto_cartographer_has_artist(
	KotoCartographer * self,
	KotoArtist * artist
) {
	gchar * artist_uuid = NULL;

	g_object_get(artist, "uuid", &artist_uuid, NULL);
	return koto_cartographer_has_artist_by_uuid(self, artist_uuid);
}

gboolean koto_cartographer_has_artist_by_uuid(
	KotoCartographer * self,
	gchar * artist_uuid
) {
	if (artist_uuid == NULL) {
		return FALSE;
	}

	return g_hash_table_contains(self->artists, artist_uuid);
}

gboolean koto_cartographer_has_playlist(
	KotoCartographer * self,
	KotoPlaylist * playlist
) {
	gchar * playlist_uuid = NULL;

	g_object_get(playlist, "uuid", &playlist_uuid, NULL);
	return koto_cartographer_has_playlist_by_uuid(self, playlist_uuid);
}

gboolean koto_cartographer_has_playlist_by_uuid(
	KotoCartographer * self,
	gchar * playlist_uuid
) {
	if (playlist_uuid == NULL) {
		return FALSE;
	}

	return g_hash_table_contains(self->playlists, playlist_uuid);
}

gboolean koto_cartographer_has_track(
	KotoCartographer * self,
	KotoTrack * track
) {
	gchar * track_uuid = NULL;

	g_object_get(track, "uuid", &track_uuid, NULL);
	return koto_cartographer_has_album_by_uuid(self, track_uuid);
}

gboolean koto_cartographer_has_track_by_uuid(
	KotoCartographer * self,
	gchar * track_uuid
) {
	if (track_uuid == NULL) {
		return FALSE;
	}

	return g_hash_table_contains(self->tracks, track_uuid);
}

void koto_cartographer_remove_album(
	KotoCartographer * self,
	KotoAlbum * album
) {
	gchar * album_uuid = NULL;

	g_object_get(album, "uuid", &album_uuid, NULL);
	koto_cartographer_remove_album_by_uuid(self, album_uuid);
}

void koto_cartographer_remove_album_by_uuid(
	KotoCartographer * self,
	gchar * album_uuid
) {
	if (album_uuid != NULL) {
		g_hash_table_remove(self->albums, album_uuid);

		g_signal_emit(
			self,
			cartographer_signals[SIGNAL_ALBUM_REMOVED],
			0,
			album_uuid
		);
	}
}

void koto_cartographer_remove_artist(
	KotoCartographer * self,
	KotoArtist * artist
) {
	if (!KOTO_IS_ARTIST(artist)) { // Not an artist
		return;
	}

	gchar * artist_uuid = NULL;

	g_object_get(artist, "uuid", &artist_uuid, NULL);
	koto_cartographer_remove_artist_by_uuid(self, artist_uuid);
	g_hash_table_remove(self->artists_name_to_uuid, koto_artist_get_name(artist)); // Add the UUID as a value with the key being the name of the artist
}

void koto_cartographer_remove_artist_by_uuid(
	KotoCartographer * self,
	gchar * artist_uuid
) {
	if (!koto_utils_is_string_valid(artist_uuid)) { // Artist UUID not valid
		return;
	}

	g_hash_table_remove(self->artists, artist_uuid);

	g_signal_emit(
		self,
		cartographer_signals[SIGNAL_ARTIST_REMOVED],
		0,
		artist_uuid
	);
}

void koto_cartographer_remove_playlist(
	KotoCartographer * self,
	KotoPlaylist * playlist
) {
	gchar * playlist_uuid = NULL;

	g_object_get(playlist, "uuid", &playlist_uuid, NULL);
	koto_cartographer_remove_playlist_by_uuid(self, playlist_uuid);
}

void koto_cartographer_remove_playlist_by_uuid(
	KotoCartographer * self,
	gchar * playlist_uuid
) {
	if (!koto_utils_is_string_valid(playlist_uuid)) { // Not a valid playlist UUID string
		return;
	}

	KotoPlaylist * possible_playlist = koto_cartographer_get_playlist_by_uuid(self, playlist_uuid);

	if (!KOTO_IS_PLAYLIST(possible_playlist)) { // If not a playlist
		return;
	}

	g_signal_emit(
		self,
		cartographer_signals[SIGNAL_PLAYLIST_REMOVED],
		0,
		playlist_uuid
	);

	g_hash_table_remove(self->playlists, playlist_uuid);
}

void koto_cartographer_remove_track(
	KotoCartographer * self,
	KotoTrack * track
) {
	gchar * track_uuid = NULL;

	g_object_get(track, "uuid", &track_uuid, NULL);
	koto_cartographer_remove_track_by_uuid(self, track_uuid);
}

void koto_cartographer_remove_track_by_uuid(
	KotoCartographer * self,
	gchar * track_uuid
) {
	if (track_uuid != NULL) {
		g_hash_table_remove(self->tracks, track_uuid);

		g_signal_emit(
			self,
			cartographer_signals[SIGNAL_TRACK_REMOVED],
			0,
			track_uuid
		);
	}
}

KotoCartographer * koto_cartographer_new() {
	return g_object_new(KOTO_TYPE_CARTOGRAPHER, NULL);
}
