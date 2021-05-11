/* list.h
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
#include "../../components/koto-action-bar.h"
#include "../../playlist/playlist.h"
#include "../../koto-utils.h"

G_BEGIN_DECLS

#define KOTO_TYPE_PLAYLIST_PAGE koto_playlist_page_get_type()
G_DECLARE_FINAL_TYPE(KotoPlaylistPage, koto_playlist_page, KOTO, PLAYLIST_PAGE, GObject);
#define KOTO_IS_PLAYLIST_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_PLAYLIST_PAGE))

KotoPlaylistPage * koto_playlist_page_new(gchar * playlist_uuid);

void koto_playlist_page_add_track(
	KotoPlaylistPage* self,
	const gchar * track_uuid
);

void koto_playlist_page_create_tracks_header(KotoPlaylistPage * self);

void koto_playlist_page_bind_track_item(
	GtkListItemFactory * factory,
	GtkListItem * item,
	KotoPlaylistPage * self
);

void koto_playlist_page_remove_track(
	KotoPlaylistPage * self,
	const gchar * track_uuid
);

GtkWidget * koto_playlist_page_get_main(KotoPlaylistPage * self);

void koto_playlist_page_handle_action_bar_closed(
	KotoActionBar * bar,
	gpointer data
);

void koto_playlist_page_handle_cover_art_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_playlist_page_handle_edit_button_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_playlist_page_handle_playlist_modified(
	KotoPlaylist * playlist,
	gpointer user_data
);

void koto_playlist_page_handle_track_album_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_playlist_page_handle_track_artist_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_playlist_page_handle_track_name_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_playlist_page_handle_track_num_clicked(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_playlist_page_handle_tracks_selected(
	GtkSelectionModel * model,
	guint position,
	guint n_items,
	gpointer user_data
);

void koto_playlist_page_hide_popover(KotoPlaylistPage * self);

void koto_playlist_page_setup_track_item(
	GtkListItemFactory * factory,
	GtkListItem * item,
	gpointer user_data
);

void koto_playlist_page_set_playlist_uuid(
	KotoPlaylistPage * self,
	gchar * playlist_uuid
);

void koto_playlist_page_set_playlist_model(
	KotoPlaylistPage * self,
	KotoPreferredModelType model
);

void koto_playlist_page_show_popover(KotoPlaylistPage * self);

void koto_playlist_page_update_header(KotoPlaylistPage * self);

void koto_playlist_page_handle_listitem_activate(
	GtkListView * list,
	guint position,
	gpointer user_data
);

G_END_DECLS
