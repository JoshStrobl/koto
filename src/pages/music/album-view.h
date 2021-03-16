/* album-view.h
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

#define KOTO_TYPE_ALBUM_VIEW (koto_album_view_get_type())

G_DECLARE_FINAL_TYPE(KotoAlbumView, koto_album_view, KOTO, ALBUM_VIEW, GObject)

KotoAlbumView* koto_album_view_new(KotoIndexedAlbum *album);
GtkWidget* koto_album_view_get_main(KotoAlbumView *self);
void koto_album_view_add_track_to_listbox(KotoIndexedAlbum *self, KotoIndexedTrack *track);
void koto_album_view_hide_overlay_controls(GtkEventControllerFocus *controller, gpointer data);
void koto_album_view_set_album(KotoAlbumView *self, KotoIndexedAlbum *album);
void koto_album_view_show_overlay_controls(GtkEventControllerFocus *controller, gpointer data);
void koto_album_view_toggle_album_playback(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
int koto_album_view_sort_discs(GtkListBoxRow *track1, GtkListBoxRow *track2, gpointer user_data);

G_END_DECLS
