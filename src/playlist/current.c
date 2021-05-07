/* current.c
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

#include <glib-2.0/glib-object.h>
#include "current.h"

enum {
	PROP_0,
	PROP_CURRENT_PLAYLIST,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

KotoCurrentPlaylist *current_playlist = NULL;

struct _KotoCurrentPlaylist {
	GObject parent_class;
	KotoPlaylist *current_playlist;
};

G_DEFINE_TYPE(KotoCurrentPlaylist, koto_current_playlist, G_TYPE_OBJECT);

static void koto_current_playlist_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_current_playlist_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_current_playlist_class_init(KotoCurrentPlaylistClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_current_playlist_set_property;
	gobject_class->get_property = koto_current_playlist_get_property;

	props[PROP_CURRENT_PLAYLIST] = g_param_spec_object(
		"current-playlist",
		"Current Playlist",
		"Current Playlist",
		KOTO_TYPE_PLAYLIST,
		G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_current_playlist_init(KotoCurrentPlaylist *self) {
	self->current_playlist = NULL;
}

void koto_current_playlist_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoCurrentPlaylist *self = KOTO_CURRENT_PLAYLIST(obj);

	switch (prop_id) {
		case PROP_CURRENT_PLAYLIST:
			g_value_set_object(val, self->current_playlist);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_current_playlist_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoCurrentPlaylist *self = KOTO_CURRENT_PLAYLIST(obj);

	switch (prop_id) {
		case PROP_CURRENT_PLAYLIST:
			koto_current_playlist_set_playlist(self, (KotoPlaylist*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

KotoPlaylist* koto_current_playlist_get_playlist(KotoCurrentPlaylist *self) {
	return self->current_playlist;
}

void koto_current_playlist_set_playlist(KotoCurrentPlaylist *self, KotoPlaylist *playlist) {
	if (!KOTO_IS_CURRENT_PLAYLIST(self)) {
		return;
	}

	if (!KOTO_IS_PLAYLIST(playlist)) { // Not a playlist
		return;
	}

	if (self->current_playlist != NULL && KOTO_IS_PLAYLIST(self->current_playlist)) {
		gboolean *is_temp = FALSE;
		g_object_get(self->current_playlist, "ephemeral", &is_temp, NULL); // Get the current ephemeral value

		if (is_temp) { // Is a temporary playlist
			koto_playlist_unmap(self->current_playlist); // Unmap the playlist if needed
		} else { // Not a temporary playlist
			koto_playlist_commit(self->current_playlist); // Save the current playlist
		}

		g_object_unref(self->current_playlist); // Unreference
	}

	self->current_playlist = playlist;
	// TODO: Saved state
	koto_playlist_set_position(self->current_playlist, -1); // Reset our position, use -1 since "next" song is then 0
	g_object_ref(playlist); // Increment the reference
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_CURRENT_PLAYLIST]);
}

KotoCurrentPlaylist* koto_current_playlist_new() {
	return g_object_new(KOTO_TYPE_CURRENT_PLAYLIST, NULL);
}
