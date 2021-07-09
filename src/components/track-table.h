/* track-table.h
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
#include "../playlist/playlist.h"
#include "koto-action-bar.h"

G_BEGIN_DECLS

#define KOTO_TYPE_TRACK_TABLE koto_track_table_get_type()
#define KOTO_TRACK_TABLE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), KOTO_TYPE_TRACK_TABLE, KotoTrackTable))
typedef struct _KotoTrackTable KotoTrackTable;
typedef struct _KotoTrackTableClass KotoTrackTableClass;

GLIB_AVAILABLE_IN_ALL
GType koto_track_type_get_type(void) G_GNUC_CONST;

#define KOTO_IS_TRACK_TABLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_TRACK_TABLE))

void koto_track_table_bind_track_item(
	GtkListItemFactory * factory,
	GtkListItem * item,
	KotoTrackTable * self
);

void koto_track_table_create_tracks_header(KotoTrackTable * self);

GtkWidget * koto_track_table_get_main(KotoTrackTable * self);

void koto_track_table_handle_action_bar_closed(
	KotoActionBar * bar,
	gpointer data
);

void koto_track_table_handle_track_album_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_track_table_handle_track_artist_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_track_table_handle_track_name_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_track_table_handle_track_name_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_track_table_handle_track_num_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_track_table_handle_tracks_selected(
	GtkSelectionModel * model,
	guint position,
	guint n_items,
	gpointer user_data
);

void koto_track_table_set_model(
	KotoTrackTable * self,
	KotoPreferredModelType model
);

void koto_track_table_set_playlist_model(
	KotoTrackTable * self,
	KotoPreferredModelType model
);

void koto_track_table_set_playlist(
	KotoTrackTable * self,
	KotoPlaylist * playlist
);

void koto_track_table_setup_track_item(
	GtkListItemFactory * factory,
	GtkListItem * item,
	gpointer user_data
);

KotoTrackTable * koto_track_table_new();

G_END_DECLS