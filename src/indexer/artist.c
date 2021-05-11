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
#include <sqlite3.h>
#include "structs.h"
#include "../db/db.h"
#include "../koto-utils.h"

extern sqlite3 * koto_db;

struct _KotoIndexedArtist {
	GObject parent_instance;
	gchar * uuid;
	gchar * path;

	gboolean has_artist_art;
	gchar * artist_name;
	GList * albums;
};

G_DEFINE_TYPE(KotoIndexedArtist, koto_indexed_artist, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_UUID,
	PROP_PATH,
	PROP_ARTIST_NAME,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};

static void koto_indexed_artist_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_indexed_artist_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_indexed_artist_class_init(KotoIndexedArtistClass * c) {
	GObjectClass * gobject_class;


	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_indexed_artist_set_property;
	gobject_class->get_property = koto_indexed_artist_get_property;

	props[PROP_UUID] = g_param_spec_string(
		"uuid",
		"UUID to Artist in database",
		"UUID to Artist in database",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_PATH] = g_param_spec_string(
		"path",
		"Path",
		"Path to Artist",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_ARTIST_NAME] = g_param_spec_string(
		"name",
		"Name",
		"Name of Artist",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

void koto_indexed_artist_commit(KotoIndexedArtist * self) {
	if ((self->uuid == NULL) || strcmp(self->uuid, "")) { // UUID not set
		self->uuid = g_strdup(g_uuid_string_random());
	}

	// TODO: Support multiple types instead of just local music artist
	gchar * commit_op = g_strdup_printf(
		"INSERT INTO artists(id,path,type,name,art_path)"
		"VALUES ('%s', quote(\"%s\"), 0, quote(\"%s\"), NULL)"
		"ON CONFLICT(id) DO UPDATE SET path=excluded.path, type=excluded.type, name=excluded.name, art_path=excluded.art_path;",
		self->uuid,
		self->path,
		self->artist_name
	);

	gchar * commit_opt_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, commit_op, 0, 0, &commit_opt_errmsg);


	if (rc != SQLITE_OK) {
		g_warning("Failed to write our artist to the database: %s", commit_opt_errmsg);
	}

	g_free(commit_op);
	g_free(commit_opt_errmsg);
}

static void koto_indexed_artist_init(KotoIndexedArtist * self) {
	self->has_artist_art = FALSE;
	self->albums = NULL; // Create a new GList
}

static void koto_indexed_artist_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoIndexedArtist * self = KOTO_INDEXED_ARTIST(obj);


	switch (prop_id) {
		case PROP_UUID:
			g_value_set_string(val, self->uuid);
			break;
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

static void koto_indexed_artist_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoIndexedArtist * self = KOTO_INDEXED_ARTIST(obj);


	switch (prop_id) {
		case PROP_UUID:
			self->uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_UUID]);
			break;
		case PROP_PATH:
			koto_indexed_artist_update_path(self, (gchar*) g_value_get_string(val));
			break;
		case PROP_ARTIST_NAME:
			koto_indexed_artist_set_artist_name(self, (gchar*) g_value_get_string(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_indexed_artist_add_album(
	KotoIndexedArtist * self,
	gchar * album_uuid
) {
	if (!KOTO_IS_INDEXED_ARTIST(self)) { // Not an artist
		return;
	}

	if (!koto_utils_is_string_valid(album_uuid)) { // No album UUID really defined
		return;
	}

	gchar * uuid = g_strdup(album_uuid); // Duplicate our UUID


	if (g_list_index(self->albums, uuid) == -1) {
		self->albums = g_list_append(self->albums, uuid); // Push to end of list
	}
}

GList * koto_indexed_artist_get_albums(KotoIndexedArtist * self) {
	if (!KOTO_IS_INDEXED_ARTIST(self)) { // Not an artist
		return NULL;
	}

	return g_list_copy(self->albums);
}

gchar * koto_indexed_artist_get_name(KotoIndexedArtist * self) {
	if (!KOTO_IS_INDEXED_ARTIST(self)) { // Not an artist
		return g_strdup("");
	}

	return g_strdup(koto_utils_is_string_valid(self->artist_name) ? self->artist_name : ""); // Return artist name if set
}

void koto_indexed_artist_remove_album(
	KotoIndexedArtist * self,
	KotoIndexedAlbum * album
) {
	if (!KOTO_IS_INDEXED_ARTIST(self)) { // Not an artist
		return;
	}

	if (!KOTO_INDEXED_ALBUM(album)) { // No album defined
		return;
	}

	gchar * album_uuid;


	g_object_get(album, "uuid", &album_uuid, NULL);
	self->albums = g_list_remove(self->albums, album_uuid);
}

void koto_indexed_artist_update_path(
	KotoIndexedArtist * self,
	gchar * new_path
) {
	if (!KOTO_IS_INDEXED_ARTIST(self)) { // Not an artist
		return;
	}

	if (!koto_utils_is_string_valid(new_path)) { // No path really
		return;
	}

	if (koto_utils_is_string_valid(self->path)) { // Already have a path set
		g_free(self->path); // Free
	}

	self->path = g_strdup(new_path);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_PATH]);
}

void koto_indexed_artist_set_artist_name(
	KotoIndexedArtist * self,
	gchar * artist_name
) {
	if (!KOTO_IS_INDEXED_ARTIST(self)) { // Not an artist
		return;
	}

	if (!koto_utils_is_string_valid(artist_name)) { // No artist name
		return;
	}

	if (koto_utils_is_string_valid(self->artist_name)) { // Has artist name
		g_free(self->artist_name);
	}

	self->artist_name = g_strdup(artist_name);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ARTIST_NAME]);
}

KotoIndexedArtist * koto_indexed_artist_new(gchar * path) {
	KotoIndexedArtist* artist = g_object_new(
		KOTO_TYPE_INDEXED_ARTIST,
		"uuid",
		g_uuid_string_random(),
		"path",
		path,
		"name",
		g_path_get_basename(path),
		NULL
	);


	koto_indexed_artist_commit(artist); // Commit the artist immediately to the database
	return artist;
}

KotoIndexedArtist * koto_indexed_artist_new_with_uuid(const gchar * uuid) {
	return g_object_new(
		KOTO_TYPE_INDEXED_ARTIST,
		"uuid",
		g_strdup(uuid),
		NULL
	);
}
