/* create-modify-dialog.h
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

#define KOTO_TYPE_CREATE_MODIFY_PLAYLIST_DIALOG koto_create_modify_playlist_dialog_get_type()
G_DECLARE_FINAL_TYPE(KotoCreateModifyPlaylistDialog, koto_create_modify_playlist_dialog, KOTO, CREATE_MODIFY_PLAYLIST_DIALOG, GtkBox);
#define KOTO_IS_CURRENT_MODIFY_PLAYLIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_CREATE_MODIFY_PLAYLIST_DIALOG))

/**
 * Functions
 **/

KotoCreateModifyPlaylistDialog * koto_create_modify_playlist_dialog_new();

void koto_create_modify_playlist_dialog_handle_chooser_response(
	GtkNativeDialog * native,
	int response,
	gpointer user_data
);

void koto_create_modify_playlist_dialog_handle_create_click(
	GtkButton * button,
	gpointer user_data
);

gboolean koto_create_modify_playlist_dialog_handle_drop(
	GtkDropTarget * target,
	const GValue * val,
	double x,
	double y,
	gpointer user_data
);

void koto_create_modify_playlist_dialog_handle_image_click(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_create_modify_playlist_dialog_reset(KotoCreateModifyPlaylistDialog * self);

void koto_create_modify_playlist_dialog_set_playlist_uuid(
	KotoCreateModifyPlaylistDialog * self,
	gchar * playlist_uuid
);

G_END_DECLS
