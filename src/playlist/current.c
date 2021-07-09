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
	SIGNAL_PLAYLIST_CHANGED,
	N_SIGNALS
};

static guint signals[N_SIGNALS] = {
	0
};

KotoCurrentPlaylist * current_playlist = NULL;

struct _KotoCurrentPlaylist {
	GObject parent_class;
	KotoPlaylist * current_playlist;
};

struct _KotoCurrentPlaylistClass {
	GObjectClass parent_class;

	void (* playlist_changed) (
		KotoCurrentPlaylist * self,
		KotoPlaylist * playlist,
		gboolean play_immediately
	);
};

static void koto_current_playlist_class_init(KotoCurrentPlaylistClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);

	signals[SIGNAL_PLAYLIST_CHANGED] = g_signal_new(
		"playlist-changed",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoCurrentPlaylistClass, playlist_changed),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		2,
		KOTO_TYPE_PLAYLIST,
		G_TYPE_BOOLEAN
	);
}

G_DEFINE_TYPE(KotoCurrentPlaylist, koto_current_playlist, G_TYPE_OBJECT);

static void koto_current_playlist_init(KotoCurrentPlaylist * self) {
	self->current_playlist = NULL;
}

KotoPlaylist * koto_current_playlist_get_playlist(KotoCurrentPlaylist * self) {
	return self->current_playlist;
}

void koto_current_playlist_set_playlist(
	KotoCurrentPlaylist * self,
	KotoPlaylist * playlist,
	gboolean play_immediately
) {
	if (!KOTO_IS_CURRENT_PLAYLIST(self)) {
		return;
	}

	if (!KOTO_IS_PLAYLIST(playlist)) { // Not a playlist
		return;
	}

	if (KOTO_IS_PLAYLIST(self->current_playlist)) {
		gboolean * is_temp = FALSE;
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
	g_signal_emit(
		self,
		signals[SIGNAL_PLAYLIST_CHANGED],
		0,
		playlist,
		play_immediately
	);
}

KotoCurrentPlaylist * koto_current_playlist_new() {
	return g_object_new(KOTO_TYPE_CURRENT_PLAYLIST, NULL);
}
