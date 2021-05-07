/* album.c
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
#include <stdio.h>
#include "../db/cartographer.h"
#include "../playlist/current.h"
#include "../playlist/playlist.h"
#include "structs.h"
#include "koto-utils.h"

extern KotoCartographer *koto_maps;
extern KotoCurrentPlaylist *current_playlist;
extern sqlite3 *koto_db;

struct _KotoIndexedAlbum {
	GObject parent_instance;
	gchar *uuid;
	gchar *path;

	gchar *name;
	gchar *art_path;
	gchar *artist_uuid;
	GList *tracks;

	gboolean has_album_art;
	gboolean do_initial_index;
};

G_DEFINE_TYPE(KotoIndexedAlbum, koto_indexed_album, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_UUID,
	PROP_DO_INITIAL_INDEX,
	PROP_PATH,
	PROP_ALBUM_NAME,
	PROP_ART_PATH,
	PROP_ARTIST_UUID,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

static void koto_indexed_album_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_indexed_album_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_indexed_album_class_init(KotoIndexedAlbumClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_indexed_album_set_property;
	gobject_class->get_property = koto_indexed_album_get_property;

	props[PROP_UUID] = g_param_spec_string(
		"uuid",
		"UUID to Album in database",
		"UUID to Album in database",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_DO_INITIAL_INDEX] = g_param_spec_boolean(
		"do-initial-index",
		"Do an initial indexing operating instead of pulling from the database",
		"Do an initial indexing operating instead of pulling from the database",
		FALSE,
		G_PARAM_CONSTRUCT_ONLY|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_PATH] = g_param_spec_string(
		"path",
		"Path",
		"Path to Album",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ALBUM_NAME] = g_param_spec_string(
		"name",
		"Name",
		"Name of Album",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ART_PATH] = g_param_spec_string(
		"art-path",
		"Path to Artwork",
		"Path to Artwork",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ARTIST_UUID] = g_param_spec_string(
		"artist-uuid",
		"UUID of Artist associated with Album",
		"UUID of Artist associated with Album",
		NULL,
		G_PARAM_CONSTRUCT_ONLY|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_indexed_album_init(KotoIndexedAlbum *self) {
	self->has_album_art = FALSE;
	self->tracks = NULL;
}

void koto_indexed_album_add_track(KotoIndexedAlbum *self, KotoIndexedTrack *track) {
	if (track == NULL) { // Not a file
		return;
	}

	gchar *track_uuid;
	g_object_get(track, "uuid", &track_uuid, NULL);

	if (g_list_index(self->tracks, track_uuid) == -1) {
		self->tracks = g_list_insert_sorted_with_data(self->tracks, track_uuid, koto_indexed_album_sort_tracks, NULL);
	}
}

void koto_indexed_album_commit(KotoIndexedAlbum *self) {
	if (self->art_path == NULL) { // If art_path isn't defined when committing
		koto_indexed_album_set_album_art(self, ""); // Set to an empty string
	}

	gchar *commit_op = g_strdup_printf(
		"INSERT INTO albums(id, path, artist_id, name, art_path)"
		"VALUES('%s', quote(\"%s\"), '%s', quote(\"%s\"), quote(\"%s\"))"
		"ON CONFLICT(id) DO UPDATE SET path=excluded.path, name=excluded.name, art_path=excluded.art_path;",
		self->uuid,
		self->path,
		self->artist_uuid,
		self->name,
		self->art_path
	);

	gchar *commit_op_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, commit_op, 0, 0, &commit_op_errmsg);

	if (rc != SQLITE_OK) {
		g_warning("Failed to write our album to the database: %s", commit_op_errmsg);
	}

	g_free(commit_op);
	g_free(commit_op_errmsg);
}

void koto_indexed_album_find_album_art(KotoIndexedAlbum *self) {
	magic_t magic_cookie = magic_open(MAGIC_MIME);

	if (magic_cookie == NULL) {
		return;
	}

	if (magic_load(magic_cookie, NULL) != 0) {
		magic_close(magic_cookie);
		return;
	}

	DIR *dir = opendir(self->path); // Attempt to open our directory

	if (dir == NULL) {
		return;
	}

	struct dirent *entry;

	while ((entry = readdir(dir))) {
		if (entry->d_type != DT_REG) { // Not a regular file
			continue; // SKIP
		}

		if (g_str_has_prefix(entry->d_name, ".")) { // Reference to parent dir, self, or a hidden item
			continue; // Skip
		}

		gchar *full_path = g_strdup_printf("%s%s%s", self->path, G_DIR_SEPARATOR_S, entry->d_name);

		const char *mime_type = magic_file(magic_cookie, full_path);

		if (mime_type == NULL) { // Failed to get the mimetype
			g_free(full_path);
			continue; // Skip
		}

		if (g_str_has_prefix(mime_type, "image/") && !self->has_album_art) { // Is an image file and doesn't have album art yet
			gchar *album_art_no_ext = g_strdup(koto_utils_get_filename_without_extension(entry->d_name)); // Get the name of the file without the extension
			gchar *lower_art = g_strdup(g_utf8_strdown(album_art_no_ext, -1)); // Lowercase

			if (
				(g_strrstr(lower_art, "Small") == NULL) && // Not Small
				(g_strrstr(lower_art, "back") == NULL) // Not back
			) {
				koto_indexed_album_set_album_art(self, full_path);
				g_free(album_art_no_ext);
				g_free(lower_art);
				break;
			}

			g_free(album_art_no_ext);
			g_free(lower_art);
		}

		g_free(full_path);
	}

	closedir(dir);
	magic_close(magic_cookie);
}

void koto_indexed_album_find_tracks(KotoIndexedAlbum *self, magic_t magic_cookie, const gchar *path) {
	if (magic_cookie == NULL) { // No cookie provided
		magic_cookie = magic_open(MAGIC_MIME);
	}

	if (magic_cookie == NULL) {
		return;
	}

	if (path == NULL) {
		path = self->path;
	}

	if (magic_load(magic_cookie, NULL) != 0) {
		magic_close(magic_cookie);
		return;
	}

	DIR *dir = opendir(path); // Attempt to open our directory

	if (dir == NULL) {
		return;
	}

	struct dirent *entry;

	while ((entry = readdir(dir))) {
		if (g_str_has_prefix(entry->d_name, ".")) { // Reference to parent dir, self, or a hidden item
			continue; // Skip
		}

		gchar *full_path = g_strdup_printf("%s%s%s", path, G_DIR_SEPARATOR_S, entry->d_name);

		if (entry->d_type == DT_DIR) { // If this is a directory
			koto_indexed_album_find_tracks(self, magic_cookie, full_path); // Recursively find tracks
			g_free(full_path);
			continue;
		}

		if (entry->d_type != DT_REG) { // Not a regular file
			continue; // SKIP
		}

		const char *mime_type = magic_file(magic_cookie, full_path);

		if (mime_type == NULL) { // Failed to get the mimetype
			g_free(full_path);
			continue; // Skip
		}

		if (g_str_has_prefix(mime_type, "audio/") || g_str_has_prefix(mime_type, "video/ogg")) { // Is an audio file or ogg because it is special
			gchar *appended_slash_to_path = g_strdup_printf("%s%s", g_strdup(self->path), G_DIR_SEPARATOR_S);
			gchar **possible_cd_split = g_strsplit(full_path, appended_slash_to_path, -1); // Split based on the album path
			guint *cd = (guint*) 1;

			gchar *track_with_cd_sep = g_strdup(possible_cd_split[1]); // Duplicate
			gchar **split_on_cd = g_strsplit(track_with_cd_sep, G_DIR_SEPARATOR_S, -1); // Split based on separator (e.g. / )

			if (g_strv_length(split_on_cd) > 1) {
				gchar *cdd = g_strdup(split_on_cd[0]);
				gchar **cd_sep = g_strsplit(g_utf8_strdown(cdd, -1), "cd", -1);

				if (g_strv_length(cd_sep) > 1) {
					gchar *pos_str = g_strdup(cd_sep[1]);
					cd = (guint*) g_ascii_strtoull(pos_str, NULL, 10); // Attempt to convert
					g_free(pos_str);
				}

				g_strfreev(cd_sep);
				g_free(cdd);
			}

			g_strfreev(split_on_cd);
			g_free(track_with_cd_sep);

			g_strfreev(possible_cd_split);
			g_free(appended_slash_to_path);

			KotoIndexedTrack *track = koto_indexed_track_new(self, full_path, cd);

			if (track != NULL) { // Is a file
				koto_indexed_album_add_track(self, track); // Add our file
			}
		}

		g_free(full_path);
	}
}

static void koto_indexed_album_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoIndexedAlbum *self = KOTO_INDEXED_ALBUM(obj);

	switch (prop_id) {
		case PROP_UUID:
			g_value_set_string(val, self->uuid);
			break;
		case PROP_DO_INITIAL_INDEX:
			g_value_set_boolean(val, self->do_initial_index);
			break;
		case PROP_PATH:
			g_value_set_string(val, self->path);
			break;
		case PROP_ALBUM_NAME:
			g_value_set_string(val, self->name);
			break;
		case PROP_ART_PATH:
			g_value_set_string(val, koto_indexed_album_get_album_art(self));
			break;
		case PROP_ARTIST_UUID:
			g_value_set_string(val, self->artist_uuid);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_indexed_album_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec){
	KotoIndexedAlbum *self = KOTO_INDEXED_ALBUM(obj);

	switch (prop_id) {
		case PROP_UUID:
			self->uuid = g_strdup(g_value_get_string(val));
			g_object_notify_by_pspec(G_OBJECT(self), props[PROP_UUID]);
			break;
		case PROP_DO_INITIAL_INDEX:
			self->do_initial_index = g_value_get_boolean(val);
			break;
		case PROP_PATH: // Path to the album
			koto_indexed_album_update_path(self, g_value_get_string(val));
			break;
		case PROP_ALBUM_NAME: // Name of album
			koto_indexed_album_set_album_name(self, g_value_get_string(val));
			break;
		case PROP_ART_PATH: // Path to art
			koto_indexed_album_set_album_art(self, g_value_get_string(val));
			break;
		case PROP_ARTIST_UUID:
			koto_indexed_album_set_artist_uuid(self, g_value_get_string(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

gchar* koto_indexed_album_get_album_art(KotoIndexedAlbum *self) {
	if (!KOTO_IS_INDEXED_ALBUM(self)) { // Not an album
		return g_strdup("");
	}

	return g_strdup((self->has_album_art && (self->art_path != NULL) && (g_strcmp0(self->art_path, "") != 0)) ? self->art_path : "");
}

gchar *koto_indexed_album_get_album_name(KotoIndexedAlbum *self) {
	if (!KOTO_IS_INDEXED_ALBUM(self)) { // Not an album
		return NULL;
	}

	if ((self->name == NULL) || g_strcmp0(self->name, "") == 0) { // Not set
		return NULL;
	}

	return g_strdup(self->name); // Return duplicate of the name
}

gchar* koto_indexed_album_get_album_uuid(KotoIndexedAlbum *self) {
	if ((self->uuid == NULL) || g_strcmp0(self->uuid, "") == 0) { // Not set
		return NULL;
	}

	return g_strdup(self->uuid); // Return a duplicate of the UUID
}

GList* koto_indexed_album_get_tracks(KotoIndexedAlbum *self) {
	if (!KOTO_IS_INDEXED_ALBUM(self)) { // Not an album
		return NULL;
	}

	return self->tracks; // Return
}

void koto_indexed_album_set_album_art(KotoIndexedAlbum *self, const gchar *album_art) {
	if (!KOTO_IS_INDEXED_ALBUM(self)) { // Not an album
		return;
	}

	if (album_art == NULL) { // Not valid album art
		return;
	}

	if (self->art_path != NULL) {
		g_free(self->art_path);
	}

	self->art_path = g_strdup(album_art);

	self->has_album_art = TRUE;
}

void koto_indexed_album_remove_file(KotoIndexedAlbum *self, KotoIndexedTrack *track) {
	if (!KOTO_IS_INDEXED_ALBUM(self)) { // Not an album
		return;
	}

	if (track == NULL) { // Not a file
		return;
	}

	gchar *track_uuid;
	g_object_get(track, "parsed-name", &track_uuid, NULL);
	self->tracks = g_list_remove(self->tracks, track_uuid);
}

void koto_indexed_album_set_album_name(KotoIndexedAlbum *self, const gchar *album_name) {
	if (!KOTO_IS_INDEXED_ALBUM(self)) { // Not an album
		return;
	}

	if (album_name == NULL) { // Not valid album name
		return;
	}

	if (self->name != NULL) {
		g_free(self->name);
	}

	self->name = g_strdup(album_name);
}

void koto_indexed_album_set_artist_uuid(KotoIndexedAlbum *self, const gchar *artist_uuid) {
	if (!KOTO_IS_INDEXED_ALBUM(self)) { // Not an album
		return;
	}

	if (artist_uuid == NULL) {
		return;
	}

	if (self->artist_uuid != NULL) {
		g_free(self->artist_uuid);
	}

	self->artist_uuid = g_strdup(artist_uuid);
}

void koto_indexed_album_set_as_current_playlist(KotoIndexedAlbum *self) {
	if (!KOTO_IS_INDEXED_ALBUM(self)) { // Not an album
		return;
	}

	if (self->tracks == NULL) { // No files to add to the playlist
		return;
	}

	KotoPlaylist *new_album_playlist = koto_playlist_new(); // Create a new playlist
	g_object_set(new_album_playlist, "ephemeral", TRUE, NULL); // Set as ephemeral / temporary

	// The following section effectively reverses our tracks, so the first is now last.
	// It then adds them in reverse order, since our playlist add function will prepend to our queue
	// This enables the preservation and defaulting of "newest" first everywhere else, while in albums preserving the rightful order of the album
	// e.g. first track (0) being added last is actually first in the playlist's tracks
	GList *reversed_tracks = g_list_copy(self->tracks); // Copy our tracks so we can reverse the order
	reversed_tracks = g_list_reverse(reversed_tracks); // Actually reverse it

	GList *t;
	for (t = reversed_tracks; t != NULL; t = t->next) { // For each of the tracks
		gchar* track_uuid = t->data;
		koto_playlist_add_track_by_uuid(new_album_playlist, track_uuid, FALSE, FALSE); // Add the UUID, skip commit to table since it is temporary
	}

	g_list_free(t);
	g_list_free(reversed_tracks);

	koto_playlist_apply_model(new_album_playlist, KOTO_PREFERRED_MODEL_TYPE_DEFAULT); // Ensure we are using our default model
	koto_current_playlist_set_playlist(current_playlist, new_album_playlist); // Set our new current playlist
}

gint koto_indexed_album_sort_tracks(gconstpointer track1_uuid, gconstpointer track2_uuid, gpointer user_data) {
	(void) user_data;
	KotoIndexedTrack *track1 = koto_cartographer_get_track_by_uuid(koto_maps, (gchar*) track1_uuid);
	KotoIndexedTrack *track2 = koto_cartographer_get_track_by_uuid(koto_maps, (gchar*) track2_uuid);

	if ((track1 == NULL) && (track2 == NULL)) { // Neither tracks actually exist
		return 0;
	} else if ((track1 != NULL) && (track2 == NULL)) { // Only track2 does not exist
		return -1;
	} else if ((track1 == NULL) && (track2 != NULL)) { // Only track1 does not exist
		return 1;
	}

	guint *track1_disc = (guint*) 1;
	guint *track2_disc = (guint*) 2;

	g_object_get(track1, "cd", &track1_disc, NULL);
	g_object_get(track2, "cd", &track2_disc, NULL);

	if (track1_disc < track2_disc) { // Track 2 is in a later CD / Disc
		return -1;
	} else if (track1_disc > track2_disc) { // Track1 is later
		return 1;
	}

	guint16 *track1_pos;
	guint16 *track2_pos;

	g_object_get(track1, "position", &track1_pos, NULL);
	g_object_get(track2, "position", &track2_pos, NULL);

	if (track1_pos == track2_pos) { // Identical positions (like reported as 0)
		gchar *track1_name;
		gchar *track2_name;

		g_object_get(track1, "parsed-name", &track1_name, NULL);
		g_object_get(track2, "parsed-name", &track2_name, NULL);

		return g_utf8_collate(track1_name, track2_name);
	} else if (track1_pos < track2_pos) {
		return -1;
	} else {
		return 1;
	}
}

void koto_indexed_album_update_path(KotoIndexedAlbum *self, const gchar* new_path) {
	if (!KOTO_IS_INDEXED_ALBUM(self)) { // Not an album
		return;
	}

	if ((new_path == NULL) || g_strcmp0(new_path, "") == 0) {
		return;
	}

	if ((self->path != NULL) && g_strcmp0(self->path, "") != 0) {
		g_free(self->path);
	}

	self->path = g_strdup(new_path);
	koto_indexed_album_set_album_name(self, g_path_get_basename(self->path)); // Update our album name based on the base name

	if (!self->do_initial_index) { // Not doing our initial index
		return;
	}

	koto_indexed_album_find_album_art(self); // Update our path for the album art
}

KotoIndexedAlbum* koto_indexed_album_new(KotoIndexedArtist *artist, const gchar *path) {
	gchar *artist_uuid = NULL;
	g_object_get(artist, "uuid", &artist_uuid, NULL);

	KotoIndexedAlbum* album = g_object_new(KOTO_TYPE_INDEXED_ALBUM,
		"artist-uuid", artist_uuid,
		"uuid", g_strdup(g_uuid_string_random()),
		"do-initial-index", TRUE,
		"path", path,
		NULL
	);

	koto_indexed_album_commit(album);
	koto_indexed_album_find_tracks(album, NULL, NULL); // Scan for tracks now that we committed to the database (hopefully)

	return album;
}

KotoIndexedAlbum* koto_indexed_album_new_with_uuid(KotoIndexedArtist *artist, const gchar *uuid) {
	gchar *artist_uuid = NULL;
	g_object_get(artist, "uuid", &artist_uuid, NULL);

	return g_object_new(KOTO_TYPE_INDEXED_ALBUM,
		"artist-uuid", artist,
		"uuid", g_strdup(uuid),
		"do-initial-index", FALSE,
		NULL
	);
}
