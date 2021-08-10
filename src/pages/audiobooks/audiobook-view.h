/* audiobook-view.h
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

#define KOTO_TYPE_AUDIOBOOK_VIEW (koto_audiobook_view_get_type())
G_DECLARE_FINAL_TYPE(KotoAudiobookView, koto_audiobook_view, KOTO, AUDIOBOOK_VIEW, GtkBox);
#define KOTO_IS_AUDIOBOOK_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_AUDIOBOOK_VIEW))

void koto_audiobook_view_set_album(
	KotoAudiobookView * self,
	KotoAlbum * album
);

GtkWidget * koto_audiobook_view_create_track_item(
	gpointer item,
	gpointer user_data
);

void koto_audiobook_view_handle_play_clicked(
	GtkButton * button,
	gpointer user_data
);

void koto_audiobook_view_handle_playlist_updated(
	KotoPlaylist * playlist,
	gpointer user_data
);

void koto_audiobook_view_update_side_info(KotoAudiobookView * self);

void koto_audiobook_view_destroy_associated_user_data(gpointer user_data);

KotoAudiobookView * koto_audiobook_view_new();

G_END_DECLS