/* track-item.h
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
#include "indexer/structs.h"

G_BEGIN_DECLS

#define KOTO_TYPE_TRACK_ITEM (koto_track_item_get_type())

G_DECLARE_FINAL_TYPE(KotoTrackItem, koto_track_item, KOTO, TRACK_ITEM, GtkBox)

KotoTrackItem* koto_track_item_new(KotoTrack * track);
void koto_track_item_handle_add_to_playlist_button_click(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

KotoTrack * koto_track_item_get_track(KotoTrackItem * self);

void koto_track_item_set_track(
	KotoTrackItem * self,
	KotoTrack * track
);

G_END_DECLS
