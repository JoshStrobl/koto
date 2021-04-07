/* create-dialog.h
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

G_BEGIN_DECLS

/**
 * Type Definition
**/

#define KOTO_TYPE_CREATE_PLAYLIST_DIALOG koto_create_playlist_dialog_get_type()
G_DECLARE_FINAL_TYPE(KotoCreatePlaylistDialog, koto_create_playlist_dialog, KOTO, CREATE_PLAYLIST_DIALOG, GObject);

/**
 * Create Dialog Functions
**/

KotoCreatePlaylistDialog* koto_create_playlist_dialog_new();
GtkWidget* koto_create_playlist_dialog_get_content(KotoCreatePlaylistDialog *self);
void koto_create_playlist_dialog_handle_close(KotoCreatePlaylistDialog *self);
void koto_create_playlist_dialog_handle_create(KotoCreatePlaylistDialog *self);
void koto_create_playlist_dialog_handle_image_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data);

G_END_DECLS
