/* koto-cover-art-button.c
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

#include <glib-2.0/glib.h>
#include <gtk-4.0/gtk/gtk.h>
#include "koto-cover-art-button.h"
#include "../koto-button.h"
#include "../koto-utils.h"

struct _KotoCoverArtButton {
	GObject parent_instance;

	GtkWidget * art;
	GtkWidget * main;
	GtkWidget * revealer;
	KotoButton * play_pause_button;

	guint height;
	guint width;
};

G_DEFINE_TYPE(KotoCoverArtButton, koto_cover_art_button, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_DESIRED_HEIGHT,
	PROP_DESIRED_WIDTH,
	PROP_ART_PATH,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};
static void koto_cover_art_button_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_cover_art_button_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_cover_art_button_class_init(KotoCoverArtButtonClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->get_property = koto_cover_art_button_get_property;
	gobject_class->set_property = koto_cover_art_button_set_property;

	props[PROP_DESIRED_HEIGHT] = g_param_spec_uint(
		"desired-height",
		"Desired height",
		"Desired height",
		0,
		G_MAXUINT,
		0,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	props[PROP_DESIRED_WIDTH] = g_param_spec_uint(
		"desired-width",
		"Desired width",
		"Desired width",
		0,
		G_MAXUINT,
		0,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	props[PROP_ART_PATH] = g_param_spec_string(
		"art-path",
		"Path to art",
		"Path to art",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_cover_art_button_init(KotoCoverArtButton * self) {
	self->main = gtk_overlay_new(); // Create our overlay container
	gtk_widget_add_css_class(self->main, "cover-art-button");
	self->revealer = gtk_revealer_new(); // Create a new revealer
	gtk_revealer_set_transition_type(GTK_REVEALER(self->revealer), GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
	gtk_revealer_set_transition_duration(GTK_REVEALER(self->revealer), 400);

	GtkWidget * controls = gtk_center_box_new(); // Create a center box for the controls

	self->play_pause_button = koto_button_new_with_icon("", "media-playback-start-symbolic", "media-playback-pause-symbolic", KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	gtk_center_box_set_center_widget(GTK_CENTER_BOX(controls), GTK_WIDGET(self->play_pause_button));

	gtk_revealer_set_child(GTK_REVEALER(self->revealer), controls);
	koto_cover_art_button_hide_overlay_controls(NULL, self); // Hide by default
	gtk_overlay_add_overlay(GTK_OVERLAY(self->main), self->revealer); // Add our revealer as the overlay

	GtkEventController * motion_controller = gtk_event_controller_motion_new(); // Create our new motion event controller to track mouse leave and enter

	g_signal_connect(motion_controller, "enter", G_CALLBACK(koto_cover_art_button_show_overlay_controls), self);
	g_signal_connect(motion_controller, "leave", G_CALLBACK(koto_cover_art_button_hide_overlay_controls), self);
	gtk_widget_add_controller(self->main, motion_controller);
}

static void koto_cover_art_button_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	(void) val;

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_cover_art_button_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoCoverArtButton * self = KOTO_COVER_ART_BUTTON(obj);

	switch (prop_id) {
		case PROP_ART_PATH:
			koto_cover_art_button_set_art_path(self, (gchar*) g_value_get_string(val)); // Get the value and call our set_art_path with it
			break;
		case PROP_DESIRED_HEIGHT:
			koto_cover_art_button_set_dimensions(self, g_value_get_uint(val), 0);
			break;
		case PROP_DESIRED_WIDTH:
			koto_cover_art_button_set_dimensions(self, 0, g_value_get_uint(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_cover_art_button_hide_overlay_controls(
	GtkEventControllerFocus * controller,
	gpointer data
) {
	(void) controller;
	KotoCoverArtButton * self = data;

	gtk_revealer_set_reveal_child(GTK_REVEALER(self->revealer), FALSE);
}

KotoButton * koto_cover_art_button_get_button(KotoCoverArtButton * self) {
	if (!KOTO_IS_COVER_ART_BUTTON(self)) {
		return NULL;
	}

	return self->play_pause_button;
}

GtkWidget * koto_cover_art_button_get_main(KotoCoverArtButton * self) {
	if (!KOTO_IS_COVER_ART_BUTTON(self)) {
		return NULL;
	}

	return self->main;
}

void koto_cover_art_button_set_art_path(
	KotoCoverArtButton * self,
	gchar * art_path
) {
	if (!KOTO_IS_COVER_ART_BUTTON(self)) {
		return;
	}

	gboolean defined_artwork = koto_utils_is_string_valid(art_path);

	if (GTK_IS_IMAGE(self->art)) { // Already have an image
		if (!defined_artwork) { // No art path or empty string
			gtk_image_set_from_icon_name(GTK_IMAGE(self->art), "audio-x-generic-symbolic");
		} else { // Have an art path
			gtk_image_set_from_file(GTK_IMAGE(self->art), g_strdup(art_path)); // Set from the file
		}
	} else { // If we don't have an image
		self->art = koto_utils_create_image_from_filepath(defined_artwork ? g_strdup(art_path) : NULL, "audio-x-generic-symbolic", self->width, self->height);
		gtk_overlay_set_child(GTK_OVERLAY(self->main), self->art); // Set the child
	}
}

void koto_cover_art_button_set_dimensions(
	KotoCoverArtButton * self,
	guint height,
	guint width
) {
	if (!KOTO_IS_COVER_ART_BUTTON(self)) {
		return;
	}

	if (height != 0) {
		self->height = height;
	}

	if (width != 0) {
		self->width = width;
	}

	if ((self->height != 0) && (self->width != 0)) { // Both height and width set
		gtk_widget_set_size_request(self->main, self->width, self->height); // Update our widget

		if (GTK_IS_IMAGE(self->art)) { // Art is defined
			gtk_widget_set_size_request(self->art, self->width, self->height); // Update our image as well
		}
	}
}

void koto_cover_art_button_show_overlay_controls(
	GtkEventControllerFocus * controller,
	gpointer data
) {
	(void) controller;
	KotoCoverArtButton * self = data;

	gtk_revealer_set_reveal_child(GTK_REVEALER(self->revealer), TRUE);
}

KotoCoverArtButton * koto_cover_art_button_new(
	guint height,
	guint width,
	gchar * art_path
) {
	return g_object_new(
		KOTO_TYPE_COVER_ART_BUTTON,
		"desired-height",
		height,
		"desired-width",
		width,
		"art-path",
		art_path,
		NULL
	);
}