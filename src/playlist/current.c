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

KotoCurrentPlaylist *current_playlist = NULL;

struct _KotoCurrentPlaylist {
	GObject parent_class;
	KotoPlaylist *current_playlist;
};

G_DEFINE_TYPE(KotoCurrentPlaylist, koto_current_playlist, G_TYPE_OBJECT);

static void koto_current_playlist_class_init(KotoCurrentPlaylistClass *c) {
	(void) c;
}

static void koto_current_playlist_init(KotoCurrentPlaylist *self) {
	self->current_playlist = NULL;
}

KotoPlaylist* koto_current_playlist_get_playlist(KotoCurrentPlaylist *self) {
	return self->current_playlist;
}

void koto_current_playlist_set_playlist(KotoCurrentPlaylist *self, KotoPlaylist *playlist) {
	if (!KOTO_IS_CURRENT_PLAYLIST(self)) {
		return;
	}

	if (KOTO_IS_PLAYLIST(self->current_playlist)) {
		// TODO: Save current playlist state if needed
		g_object_unref(self->current_playlist); // Unreference
	}

	self->current_playlist = playlist;
	g_object_ref(playlist); // Increment the reference
	g_object_notify(G_OBJECT(self), "current-playlist-changed");
	koto_playlist_debug(self->current_playlist);
}

KotoCurrentPlaylist* koto_current_playlist_new() {
	return g_object_new(KOTO_TYPE_CURRENT_PLAYLIST, NULL);
}
