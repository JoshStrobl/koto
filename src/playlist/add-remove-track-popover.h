/* add-remove-track-popover.h
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
#include "../db/cartographer.h"
#include "playlist.h"

G_BEGIN_DECLS

/**
 * Type Definition
 **/

#define KOTO_TYPE_ADD_REMOVE_TRACK_POPOVER koto_add_remove_track_popover_get_type()
G_DECLARE_FINAL_TYPE(KotoAddRemoveTrackPopover, koto_add_remove_track_popover, KOTO, ADD_REMOVE_TRACK_POPOVER, GtkPopover);
#define KOTO_JS_ADD_REMOVE_TRACK_POPOVER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_ADD_REMOVE_TRACK_POPOVER))

/**
 * Functions
 **/

KotoAddRemoveTrackPopover * koto_add_remove_track_popover_new();

void koto_add_remove_track_popover_add_playlist(
	KotoAddRemoveTrackPopover * self,
	KotoPlaylist * playlist
);

void koto_add_remove_track_popover_clear_tracks(KotoAddRemoveTrackPopover * self);

void koto_add_remove_track_popover_remove_playlist(
	KotoAddRemoveTrackPopover * self,
	gchar * playlist_uuid
);

void koto_add_remove_track_popover_handle_checkbutton_toggle(
	GtkCheckButton * btn,
	gpointer user_data
);

void koto_add_remove_track_popover_handle_playlist_added(
	KotoCartographer * carto,
	KotoPlaylist * playlist,
	gpointer user_data
);

void koto_add_remove_track_popover_handle_playlist_removed(
	KotoCartographer * carto,
	gchar * playlist_uuid,
	gpointer user_data
);

void koto_add_remove_track_popover_set_pointing_to_widget(
	KotoAddRemoveTrackPopover * self,
	GtkWidget * widget,
	GtkPositionType pos
);

void koto_add_remove_track_popover_set_tracks(
	KotoAddRemoveTrackPopover * self,
	GList * tracks
);

G_END_DECLS