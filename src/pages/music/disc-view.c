/* disc-view.c
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

#include <gtk-4.0/gtk/gtk.h>
#include "../../components/koto-action-bar.h"
#include "../../db/cartographer.h"
#include "../../indexer/track-helpers.h"
#include "../../indexer/structs.h"
#include "../../koto-track-item.h"
#include "../../koto-utils.h"
#include "disc-view.h"

extern KotoActionBar * action_bar;
extern KotoCartographer * koto_maps;

struct _KotoDiscView {
	GtkBox parent_instance;
	KotoAlbum * album;
	GtkWidget * header;
	GtkWidget * label;
	GtkWidget * list;

	GHashTable * tracks_to_items;

	guint disc_number;
};

G_DEFINE_TYPE(KotoDiscView, koto_disc_view, GTK_TYPE_BOX);

enum {
	PROP_0,
	PROP_DISC,
	PROP_ALBUM,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};

static void koto_disc_view_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_disc_view_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_disc_view_class_init(KotoDiscViewClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_disc_view_set_property;
	gobject_class->get_property = koto_disc_view_get_property;

	props[PROP_DISC] = g_param_spec_uint(
		"disc",
		"Disc",
		"Disc",
		0,
		G_MAXUINT16,
		1,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_ALBUM] = g_param_spec_object(
		"album",
		"Album",
		"Album",
		KOTO_TYPE_ALBUM,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_disc_view_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoDiscView * self = KOTO_DISC_VIEW(obj);

	switch (prop_id) {
		case PROP_DISC:
			g_value_set_uint(val, GPOINTER_TO_UINT(self->disc_number));
			break;
		case PROP_ALBUM:
			g_value_set_object(val, self->album);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_disc_view_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoDiscView * self = KOTO_DISC_VIEW(obj);

	switch (prop_id) {
		case PROP_DISC:
			koto_disc_view_set_disc_number(self, g_value_get_uint(val));
			break;
		case PROP_ALBUM:
			koto_disc_view_set_album(self, (KotoAlbum*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_disc_view_init(KotoDiscView * self) {
	self->tracks_to_items = g_hash_table_new(g_str_hash, g_str_equal);

	gtk_widget_add_css_class(GTK_WIDGET(self), "disc-view");
	gtk_widget_set_can_focus(GTK_WIDGET(self), FALSE);
	self->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand(self->header, TRUE);
	gtk_widget_set_size_request(self->header, 16, -1);

	GtkWidget * ico = gtk_image_new_from_icon_name("drive-optical-symbolic");

	gtk_box_prepend(GTK_BOX(self->header), ico);

	self->label = gtk_label_new(NULL); // Create an empty label
	gtk_widget_set_halign(self->label, GTK_ALIGN_START);
	gtk_widget_set_valign(self->label, GTK_ALIGN_CENTER);
	gtk_box_append(GTK_BOX(self->header), self->label);

	gtk_box_append(GTK_BOX(self), self->header);

	self->list = gtk_list_box_new(); // Create our list of our tracks
	gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(self->list), FALSE);
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->list), GTK_SELECTION_MULTIPLE);
	gtk_list_box_set_sort_func(GTK_LIST_BOX(self->list), koto_disc_view_sort_list_box_rows, self, NULL);
	gtk_widget_add_css_class(self->list, "track-list");
	gtk_widget_set_can_focus(self->list, FALSE);
	gtk_widget_set_focusable(self->list, FALSE);
	gtk_widget_set_size_request(self->list, 600, -1);
	gtk_box_append(GTK_BOX(self), self->list);

	g_signal_connect(self->list, "selected-rows-changed", G_CALLBACK(koto_disc_view_handle_selected_rows_changed), self);
}

void koto_disc_view_add_track(
	KotoDiscView * self,
	KotoTrack * track
) {
	if (!KOTO_IS_DISC_VIEW(self)) { // If this is not a disc view
		return;
	}

	if (!KOTO_IS_TRACK(track)) { // If this is not a track
		return;
	}

	if (self->disc_number != koto_track_get_disc_number(track)) { // Not the same disc
		return;
	}

	gchar * track_uuid = koto_track_get_uuid(track); // Get the track UUID

	if (g_hash_table_contains(self->tracks_to_items, track_uuid)) { // Already have added this track
		return;
	}

	KotoTrackItem * track_item = koto_track_item_new(track); // Create our new track item

	if (!KOTO_IS_TRACK_ITEM(track_item)) { // Failed to create a track item for this track
		g_warning("Failed to build track item for track with UUID %s", track_uuid);
		return;
	}

	gtk_list_box_append(GTK_LIST_BOX(self->list), GTK_WIDGET(track_item)); // Add to our tracks list box
	g_hash_table_replace(self->tracks_to_items, track_uuid, track_item); // Add to our hash table so we know we have added this
}

void koto_disc_view_handle_selected_rows_changed(
	GtkListBox * box,
	gpointer user_data
) {
	KotoDiscView * self = user_data;

	gchar * album_uuid = koto_album_get_album_uuid(self->album); // Get the UUID

	if (!koto_utils_is_string_valid(album_uuid)) { // Not set
		return;
	}

	GList * selected_rows = gtk_list_box_get_selected_rows(box); // Get the selected rows

	if (g_list_length(selected_rows) == 0) { // No rows selected
		koto_action_bar_toggle_reveal(action_bar, FALSE); // Close the action bar
		return;
	}

	GList * selected_tracks = NULL; // Create our list of KotoTracks
	GList * cur_selected_rows;

	for (cur_selected_rows = selected_rows; cur_selected_rows != NULL; cur_selected_rows = cur_selected_rows->next) { // Iterate over the rows
		KotoTrackItem * track_item = (KotoTrackItem*) gtk_list_box_row_get_child(cur_selected_rows->data);
		selected_tracks = g_list_append(selected_tracks, koto_track_item_get_track(track_item)); // Add the KotoTrack to our list
	}

	g_list_free(cur_selected_rows);

	koto_action_bar_set_tracks_in_album_selection(action_bar, album_uuid, selected_tracks); // Set our album selection
	koto_action_bar_toggle_reveal(action_bar, TRUE); // Show the action bar
}

void koto_disc_view_remove_track(
	KotoDiscView * self,
	gchar * track_uuid
) {
	if (!KOTO_IS_DISC_VIEW(self)) { // If this is not a disc view
		return;
	}

	if (!koto_utils_is_string_valid(track_uuid)) { // If our UUID is not a valid
		return;
	}

	KotoTrackItem * track_item = g_hash_table_lookup(self->tracks_to_items, track_uuid); // Get the track item

	if (!KOTO_IS_TRACK_ITEM(track_item)) { // Track item not valid
		return;
	}

	gtk_list_box_remove(GTK_LIST_BOX(self->list), GTK_WIDGET(track_item)); // Remove the item from the listbox
	g_hash_table_remove(self->tracks_to_items, track_uuid); // Remove the track from the hashtable
}

void koto_disc_view_set_album(
	KotoDiscView * self,
	KotoAlbum * album
) {
	if (album == NULL) {
		return;
	}

	if (self->album != NULL) {
		g_free(self->album);
	}

	self->album = album;
}

void koto_disc_view_set_disc_number(
	KotoDiscView * self,
	guint disc_number
) {
	if (disc_number == 0) {
		return;
	}

	self->disc_number = disc_number;

	gchar * disc_label = g_strdup_printf("Disc %u", disc_number);

	gtk_label_set_text(GTK_LABEL(self->label), disc_label); // Set the label

	g_free(disc_label);
}

void koto_disc_view_set_disc_label_visible(
	KotoDiscView * self,
	gboolean visible
) {
	(visible) ? gtk_widget_show(self->header) : gtk_widget_hide(self->header);
}

gint koto_disc_view_sort_list_box_rows(
	GtkListBoxRow * row1,
	GtkListBoxRow * row2,
	gpointer user_data
) {
	KotoTrackItem * track1_item = KOTO_TRACK_ITEM(gtk_list_box_row_get_child(row1));
	KotoTrackItem * track2_item = KOTO_TRACK_ITEM(gtk_list_box_row_get_child(row2));
	return koto_track_helpers_sort_track_items(track1_item, track2_item, user_data);
}

KotoDiscView * koto_disc_view_new(
	KotoAlbum * album,
	guint disc_number
) {
	return g_object_new(
		KOTO_TYPE_DISC_VIEW,
		"disc",
		disc_number,
		"album",
		album,
		"orientation",
		GTK_ORIENTATION_VERTICAL,
		NULL
	);
}
