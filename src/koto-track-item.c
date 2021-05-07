/* koto-track-item.c
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
#include "indexer/structs.h"
#include "playlist/add-remove-track-popover.h"
#include "koto-button.h"
#include "koto-track-item.h"

extern KotoAddRemoveTrackPopover *koto_add_remove_track_popup;

struct _KotoTrackItem {
	GtkBox parent_instance;
	KotoIndexedTrack *track;

	GtkWidget *track_label;
};

struct _KotoTrackItemClass {
	GtkBoxClass parent_class;
};

enum {
	PROP_0,
	PROP_TRACK,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE(KotoTrackItem, koto_track_item, GTK_TYPE_BOX);

static void koto_track_item_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_track_item_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_track_item_class_init(KotoTrackItemClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_track_item_set_property;
	gobject_class->get_property = koto_track_item_get_property;

	props[PROP_TRACK] = g_param_spec_object(
		"track",
		"Track",
		"Track",
		KOTO_TYPE_INDEXED_TRACK,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_track_item_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoTrackItem *self = KOTO_TRACK_ITEM(obj);

	switch (prop_id) {
		case PROP_TRACK:
			g_value_set_object(val, self->track);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_track_item_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoTrackItem *self = KOTO_TRACK_ITEM(obj);

	switch (prop_id) {
		case PROP_TRACK:
			koto_track_item_set_track(self, (KotoIndexedTrack*) g_value_get_object(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_track_item_init(KotoTrackItem *self) {
	self->track_label = gtk_label_new(NULL); // Create with no track name
	gtk_label_set_xalign(GTK_LABEL(self->track_label), 0.0);

	gtk_widget_add_css_class(GTK_WIDGET(self), "track-item");
	gtk_widget_set_hexpand(GTK_WIDGET(self), TRUE);
	gtk_widget_set_hexpand(GTK_WIDGET(self->track_label), TRUE);

	gtk_box_prepend(GTK_BOX(self), self->track_label);
}

KotoIndexedTrack* koto_track_item_get_track(KotoTrackItem *self) {
	return self->track;
}

void koto_track_item_set_track(KotoTrackItem *self, KotoIndexedTrack *track) {
	if (track == NULL) { // Not a track
		return;
	}

	self->track = track;
	gchar *track_name;
	g_object_get(self->track, "parsed-name", &track_name, NULL);
	gtk_label_set_text(GTK_LABEL(self->track_label), track_name); // Update the text
}

KotoTrackItem* koto_track_item_new(KotoIndexedTrack *track) {
	return g_object_new(KOTO_TYPE_TRACK_ITEM,
		"track",
		track,
		NULL
	);
}
