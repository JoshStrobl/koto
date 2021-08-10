/* action-bar.h
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

#include <glib-2.0/glib.h>
#include <gtk-4.0/gtk/gtk.h>
#include "../indexer/structs.h"

G_BEGIN_DECLS

typedef enum {
	KOTO_ACTION_BAR_IS_ALBUM_RELATIVE = 1,
	KOTO_ACTION_BAR_IS_PLAYLIST_RELATIVE = 2
} KotoActionBarRelative;

#define KOTO_TYPE_ACTION_BAR (koto_action_bar_get_type())
#define KOTO_IS_ACTION_BAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_ACTION_BAR))

typedef struct _KotoActionBar KotoActionBar;
typedef struct _KotoActionBarClass KotoActionBarClass;

GLIB_AVAILABLE_IN_ALL
GType koto_action_bar_get_type(void) G_GNUC_CONST;

/**
 * Action Bar Functions
 **/

KotoActionBar * koto_action_bar_new(void);

void koto_action_bar_close(KotoActionBar * self);

GtkActionBar * koto_action_bar_get_main(KotoActionBar * self);

void koto_action_bar_handle_close_button_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer data
);

void koto_action_bar_handle_go_to_artist_button_clicked(
	GtkButton * button,
	gpointer data
);

void koto_action_bar_handle_playlists_button_clicked(
	GtkButton * button,
	gpointer data
);

void koto_action_bar_handle_play_track_button_clicked(
	GtkButton * button,
	gpointer data
);

void koto_action_bar_handle_remove_from_playlist_button_clicked(
	GtkButton * button,
	gpointer data
);

void koto_action_bar_set_tracks_in_album_selection(
	KotoActionBar * self,
	gchar * album_uuid,
	GList * tracks
);

void koto_action_bar_set_tracks_in_playlist_selection(
	KotoActionBar * self,
	gchar * playlist_uuid,
	GList * tracks
);

void koto_action_bar_toggle_go_to_artist_visibility(
	KotoActionBar * self,
	gboolean visible
);

void koto_action_bar_toggle_play_button_visibility(
	KotoActionBar * self,
	gboolean visible
);

void koto_action_bar_toggle_reveal(
	KotoActionBar * self,
	gboolean state
);

G_END_DECLS