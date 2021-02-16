/* artist.c
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
#include "album.h"
#include "artist.h"

struct _KotoIndexedArtist {
	GObject parent_instance;
	gchar *path;

	gboolean has_artist_art;
	gchar *artist_name;
	GHashTable *albums;
};

G_DEFINE_TYPE(KotoIndexedArtist, koto_indexed_artist, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_PATH,
	PROP_ARTIST_NAME,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

static void koto_indexed_artist_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_indexed_artist_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_indexed_artist_class_init(KotoIndexedArtistClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_indexed_artist_set_property;
	gobject_class->get_property = koto_indexed_artist_get_property;

	props[PROP_PATH] = g_param_spec_string(
		"path",
		"Path",
		"Path to Artist",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ARTIST_NAME] = g_param_spec_string(
		"name",
		"Name",
		"Name of Artist",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_indexed_artist_init(KotoIndexedArtist *self) {
	self->has_artist_art = FALSE;
	self->albums = NULL; // Set to null initially maybe
}

static void koto_indexed_artist_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoIndexedArtist *self = KOTO_INDEXED_ARTIST(obj);

	switch (prop_id) {
		case PROP_PATH:
			g_value_set_string(val, self->path);
			break;
		case PROP_ARTIST_NAME:
			g_value_set_string(val, self->artist_name);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_indexed_artist_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec){
	KotoIndexedArtist *self = KOTO_INDEXED_ARTIST(obj);

	switch (prop_id) {
		case PROP_PATH:
			koto_indexed_artist_update_path(self, g_value_get_string(val));
			break;
		case PROP_ARTIST_NAME:
			koto_indexed_artist_set_artist_name(self, g_value_get_string(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_indexed_artist_add_album(KotoIndexedArtist *self, KotoIndexedAlbum *album) {
	if (album == NULL) { // No album really defined
		return;
	}

	if (self->albums == NULL) { // No HashTable yet
		self->albums = g_hash_table_new(g_str_hash, g_str_equal); // Create a new HashTable
	}

	gchar *album_name;
	g_object_get(album, "name", &album_name, NULL);

	if (album_name == NULL) {
		g_free(album_name);
		return;
	}

	if (g_hash_table_contains(self->albums, album_name)) { // If we have this album already
		g_free(album_name);
		return;
	}

	g_hash_table_insert(self->albums, album_name, album); // Add the album
}

GList* koto_indexed_artist_get_albums(KotoIndexedArtist *self) {
	if (self->albums == NULL) { // No HashTable yet
		self->albums = g_hash_table_new(g_str_hash, g_str_equal); // Create a new HashTable
	}

	return g_hash_table_get_values(self->albums);
}

void koto_indexed_artist_remove_album(KotoIndexedArtist *self, KotoIndexedAlbum *album) {
	if (album == NULL) { // No album defined
		return;
	}

	if (self->albums == NULL) { // No HashTable yet
		self->albums = g_hash_table_new(g_str_hash, g_str_equal); // Create a new HashTable
	}

	gchar *album_name;
	g_object_get(album, "name", &album_name, NULL);

	g_hash_table_remove(self->albums, album_name);
}

void koto_indexed_artist_remove_album_by_name(KotoIndexedArtist *self, gchar *album_name) {
	if (album_name == NULL) {
		return;
	}

	KotoIndexedAlbum *album = g_hash_table_lookup(self->albums, album_name); // Get the album
	koto_indexed_artist_remove_album(self, album);
}

void koto_indexed_artist_update_path(KotoIndexedArtist *self, const gchar *new_path) {
	if (new_path == NULL) { // No path really
		return;
	}

	if (self->path != NULL) { // Already have a path set
		g_free(self->path); // Free
	}

	self->path = g_strdup(new_path);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_PATH]);
}

void koto_indexed_artist_set_artist_name(KotoIndexedArtist *self, const gchar *artist_name) {
	if (artist_name == NULL) { // No artist name
		return;
	}

	if (self->artist_name != NULL) { // Has artist name
		g_free(self->artist_name);
	}

	self->artist_name = g_strdup(artist_name);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ARTIST_NAME]);
}

KotoIndexedArtist* koto_indexed_artist_new(const gchar *path) {
	return g_object_new(KOTO_TYPE_INDEXED_ARTIST,
		"path", path,
		"name", g_path_get_basename(path),
		NULL
	);
}
