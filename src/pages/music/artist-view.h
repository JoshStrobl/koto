/* artist-view.h
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
#include <gtk-4.0/gtk/gtk.h>
#include "../../indexer/structs.h"

G_BEGIN_DECLS

#define KOTO_TYPE_ARTIST_VIEW (koto_artist_view_get_type())

G_DECLARE_FINAL_TYPE(KotoArtistView, koto_artist_view, KOTO, ARTIST_VIEW, GObject)

KotoArtistView* koto_artist_view_new(KotoArtist * artist);

void koto_artist_view_add_album(
	KotoArtistView * self,
	KotoAlbum * album
);

GtkWidget * koto_artist_view_get_main(KotoArtistView * self);

void koto_artist_view_handle_album_added(
	KotoArtist * artist,
	KotoAlbum * album,
	gpointer user_data
);

void koto_artist_view_handle_album_removed(
	KotoArtist * artist,
	gchar * album_uuid,
	gpointer user_data
);

void koto_artist_view_handle_artist_name_changed(
	KotoArtist * artist,
	guint prop_id,
	KotoArtistView * self
);

void koto_artist_view_handle_has_no_albums(
	KotoArtist * artist,
	gpointer user_data
);

void koto_artist_view_set_artist(
	KotoArtistView * self,
	KotoArtist * artist
);

void koto_artist_view_toggle_playback(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer data
);

G_END_DECLS
