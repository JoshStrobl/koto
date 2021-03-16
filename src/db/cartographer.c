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
#include "cartographer.h"

struct _KotoCartographer {
	GObject parent_instance;

	GHashTable *albums;
	GHashTable *artists;
	GHashTable *playlists;
	GHashTable *tracks;
};

G_DEFINE_TYPE(KotoCartographer, koto_cartographer, G_TYPE_OBJECT);

KotoCartographer *koto_maps = NULL;

static void koto_cartographer_class_init(KotoCartographerClass *c) {
	(void) c;
}

static void koto_cartographer_init(KotoCartographer *self) {
	self->albums = g_hash_table_new(g_str_hash, g_str_equal);
	self->artists = g_hash_table_new(g_str_hash, g_str_equal);
	self->playlists = g_hash_table_new(g_str_hash, g_str_equal);
	self->tracks = g_hash_table_new(g_str_hash, g_str_equal);
}

void koto_cartographer_add_album(KotoCartographer *self, KotoIndexedAlbum *album) {
	gchar *album_uuid = NULL;
	g_object_get(album, "uuid", &album_uuid, NULL);

	if ((album_uuid != NULL) && (!koto_cartographer_has_album_by_uuid(self, album_uuid))) { // Don't have this album
		g_hash_table_replace(self->albums, album_uuid, album);
	}
}

void koto_cartographer_add_artist(KotoCartographer *self, KotoIndexedArtist *artist) {
	gchar *artist_uuid = NULL;
	g_object_get(artist, "uuid", &artist_uuid, NULL);

	if ((artist_uuid != NULL) && (!koto_cartographer_has_artist_by_uuid(self, artist_uuid))) { // Don't have this album
		g_hash_table_replace(self->artists, artist_uuid, artist);
	}
}

void koto_cartographer_add_playlist(KotoCartographer *self, KotoPlaylist *playlist) {
	gchar *playlist_uuid = NULL;
	g_object_get(playlist, "uuid", &playlist_uuid, NULL);

	if ((playlist_uuid != NULL) && (!koto_cartographer_has_playlist_by_uuid(self, playlist_uuid))) { // Don't have this album
		g_hash_table_replace(self->artists, playlist_uuid, playlist);
	}
}

void koto_cartographer_add_track(KotoCartographer *self, KotoIndexedTrack *track) {
	gchar *track_uuid = NULL;
	g_object_get(track, "uuid", &track_uuid, NULL);

	if ((track_uuid != NULL) && (!koto_cartographer_has_playlist_by_uuid(self, track_uuid))) { // Don't have this album
		g_hash_table_replace(self->artists, track_uuid, track);
	}
}

KotoIndexedAlbum* koto_cartographer_get_album_by_uuid(KotoCartographer *self, gchar* album_uuid) {
	return g_hash_table_lookup(self->albums, album_uuid);
}

KotoIndexedArtist* koto_cartographer_get_artist_by_uuid(KotoCartographer *self, gchar* artist_uuid) {
	return g_hash_table_lookup(self->artists, artist_uuid);
}

KotoPlaylist* koto_cartographer_get_playlist_by_uuid(KotoCartographer *self, gchar* playlist_uuid) {
	return g_hash_table_lookup(self->playlists, playlist_uuid);
}

KotoIndexedTrack* koto_cartographer_get_track_by_uuid(KotoCartographer *self, gchar* track_uuid) {
	return g_hash_table_lookup(self->tracks, track_uuid);
}

gboolean koto_cartographer_has_album(KotoCartographer *self, KotoIndexedAlbum *album)  {
	gchar *album_uuid = NULL;
	g_object_get(album, "uuid", &album_uuid, NULL);
	return koto_cartographer_has_album_by_uuid(self, album_uuid);
}

gboolean koto_cartographer_has_album_by_uuid(KotoCartographer *self, gchar* album_uuid) {
	if (album_uuid == NULL) {
		return FALSE;
	}

	return g_hash_table_contains(self->albums, album_uuid);
}

gboolean koto_cartographer_has_artist(KotoCartographer *self, KotoIndexedArtist *artist) {
	gchar *artist_uuid = NULL;
	g_object_get(artist, "uuid", &artist_uuid, NULL);
	return koto_cartographer_has_artist_by_uuid(self, artist_uuid);
}

gboolean koto_cartographer_has_artist_by_uuid(KotoCartographer *self, gchar* artist_uuid) {
	if (artist_uuid == NULL) {
		return FALSE;
	}

	return g_hash_table_contains(self->artists, artist_uuid);
}

gboolean koto_cartographer_has_playlist(KotoCartographer *self, KotoPlaylist *playlist) {
	gchar *playlist_uuid = NULL;
	g_object_get(playlist, "uuid", &playlist_uuid, NULL);
	return koto_cartographer_has_playlist_by_uuid(self, playlist_uuid);
}

gboolean koto_cartographer_has_playlist_by_uuid(KotoCartographer *self, gchar* playlist_uuid) {
	if (playlist_uuid == NULL) {
		return FALSE;
	}

	return g_hash_table_contains(self->playlists, playlist_uuid);
}

gboolean koto_cartographer_has_track(KotoCartographer *self, KotoIndexedTrack *track) {
	gchar *track_uuid = NULL;
	g_object_get(track, "uuid", &track_uuid, NULL);
	return koto_cartographer_has_album_by_uuid(self, track_uuid);
}

gboolean koto_cartographer_has_track_by_uuid(KotoCartographer *self, gchar* track_uuid) {
	if (track_uuid == NULL) {
		return FALSE;
	}

	return g_hash_table_contains(self->tracks, track_uuid);
}

void koto_cartographer_remove_album(KotoCartographer *self, KotoIndexedAlbum *album) {
	gchar *album_uuid = NULL;
	g_object_get(album, "uuid", &album_uuid, NULL);
	return koto_cartographer_remove_album_by_uuid(self, album_uuid);
}

void koto_cartographer_remove_album_by_uuid(KotoCartographer *self, gchar* album_uuid) {
	if (album_uuid != NULL) {
		g_hash_table_remove(self->albums, album_uuid);
	}

	return;
}

void koto_cartographer_remove_artist(KotoCartographer *self, KotoIndexedArtist *artist) {
	gchar *artist_uuid = NULL;
	g_object_get(artist, "uuid", &artist_uuid, NULL);
	return koto_cartographer_remove_artist_by_uuid(self, artist_uuid);
}

void koto_cartographer_remove_artist_by_uuid(KotoCartographer *self, gchar* artist_uuid) {
	if (artist_uuid == NULL) {
		g_hash_table_remove(self->artists, artist_uuid);
	}

	return;
}

void koto_cartographer_remove_playlist(KotoCartographer *self, KotoPlaylist *playlist) {
	gchar *playlist_uuid = NULL;
	g_object_get(playlist, "uuid", &playlist_uuid, NULL);
	return koto_cartographer_remove_playlist_by_uuid(self, playlist_uuid);
}

void koto_cartographer_remove_playlist_by_uuid(KotoCartographer *self, gchar* playlist_uuid) {
	if (playlist_uuid != NULL) {
		g_hash_table_remove(self->playlists, playlist_uuid);
	}

	return;
}

void koto_cartographer_remove_track(KotoCartographer *self, KotoIndexedTrack *track) {
	gchar *track_uuid = NULL;
	g_object_get(track, "uuid", &track_uuid, NULL);
	return koto_cartographer_remove_track_by_uuid(self, track_uuid);
}

void koto_cartographer_remove_track_by_uuid(KotoCartographer *self, gchar* track_uuid) {
	if (track_uuid != NULL) {
		g_hash_table_remove(self->tracks, track_uuid);
	}

	return;
}

KotoCartographer* koto_cartographer_new() {
	return g_object_new(KOTO_TYPE_CARTOGRAPHER, NULL);
}
