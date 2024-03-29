/* disc-view.h
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

#define KOTO_TYPE_DISC_VIEW (koto_disc_view_get_type())

G_DECLARE_FINAL_TYPE(KotoDiscView, koto_disc_view, KOTO, DISC_VIEW, GtkBox)

KotoDiscView* koto_disc_view_new(
	KotoAlbum * album,
	guint disc
);

void koto_disc_view_add_track(
	KotoDiscView * self,
	KotoTrack * track
);

void koto_disc_view_handle_selected_rows_changed(
	GtkListBox * box,
	gpointer user_data
);

void koto_disc_view_set_album(
	KotoDiscView * self,
	KotoAlbum * album
);

void koto_disc_view_set_disc_label_visible(
	KotoDiscView * self,
	gboolean visible
);

void koto_disc_view_set_disc_number(
	KotoDiscView * self,
	guint disc_number
);

gint koto_disc_view_sort_list_box_rows(
	GtkListBoxRow * row1,
	GtkListBoxRow * row2,
	gpointer user_data
);

G_END_DECLS
