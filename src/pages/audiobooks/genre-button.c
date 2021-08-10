/* genres-button.c
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
#include "../../components/button.h"
#include "../../koto-utils.h"
#include "genre-button.h"

enum {
	PROP_0,
	PROP_GENRE_ID,
	PROP_GENRE_NAME,
	N_PROPS
};

static GParamSpec * genre_button_props[N_PROPS] = {
	NULL,
};

struct _KotoAudiobooksGenreButton {
	GtkBox parent_instance;

	gchar * genre_id;
	gchar * genre_name;

	GtkWidget * bg_image;
	KotoButton * button;
};

struct _KotoAudiobooksGenreButtonClass {
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(KotoAudiobooksGenreButton, koto_audiobooks_genre_button, GTK_TYPE_BOX);

static void koto_audiobooks_genre_button_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_audiobooks_genre_button_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_audiobooks_genre_button_class_init(KotoAudiobooksGenreButtonClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_audiobooks_genre_button_set_property;
	gobject_class->get_property = koto_audiobooks_genre_button_get_property;

	genre_button_props[PROP_GENRE_ID] = g_param_spec_string(
		"genre-id",
		"Genre ID",
		"Genre ID",
		NULL,
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	genre_button_props[PROP_GENRE_NAME] = g_param_spec_string(
		"genre-name",
		"Genre Name",
		"Genre Name",
		NULL,
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPS, genre_button_props);
}

static void koto_audiobooks_genre_button_init(KotoAudiobooksGenreButton * self) {
	gtk_widget_add_css_class(GTK_WIDGET(self), "audiobook-genre-button");
	gtk_widget_set_hexpand(GTK_WIDGET(self), FALSE);
	gtk_widget_set_vexpand(GTK_WIDGET(self), FALSE);

	gtk_widget_set_size_request(GTK_WIDGET(self), 260, 120);
	GtkWidget * overlay = gtk_overlay_new(); // Create our overlay
	self->bg_image = gtk_picture_new(); // Create our new image

	self->button = koto_button_new_plain(NULL); // Create a new button
	koto_button_set_text_wrap(self->button, TRUE); // Enable text wrapping for long genre lines
	gtk_widget_set_valign(GTK_WIDGET(self->button), GTK_ALIGN_END); // Align towards bottom of button

	gtk_widget_set_hexpand(overlay, TRUE);

	gtk_overlay_set_child(GTK_OVERLAY(overlay), self->bg_image);
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), GTK_WIDGET(self->button));

	gtk_box_append(GTK_BOX(self), overlay); // Add the overlay to self
}

static void koto_audiobooks_genre_button_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoAudiobooksGenreButton * self = KOTO_AUDIOBOOKS_GENRE_BUTTON(obj);

	switch (prop_id) {
		case PROP_GENRE_ID:
			g_value_set_string(val, self->genre_id);
			break;
		case PROP_GENRE_NAME:
			g_value_set_string(val, self->genre_name);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_audiobooks_genre_button_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoAudiobooksGenreButton * self = KOTO_AUDIOBOOKS_GENRE_BUTTON(obj);

	switch (prop_id) {
		case PROP_GENRE_ID:
			koto_audiobooks_genre_button_set_id(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_GENRE_NAME:
			koto_audiobooks_genre_button_set_name(self, g_strdup(g_value_get_string(val)));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

KotoButton * koto_audiobooks_genre_button_get_button(KotoAudiobooksGenreButton * self) {
	return KOTO_IS_AUDIOBOOKS_GENRE_BUTTON(self) ? self->button : NULL;
}

gchar * koto_audiobooks_genre_button_get_id(KotoAudiobooksGenreButton * self) {
	return KOTO_IS_AUDIOBOOKS_GENRE_BUTTON(self) ? self->genre_id : NULL;
}

gchar * koto_audiobooks_genre_button_get_name(KotoAudiobooksGenreButton * self) {
	return KOTO_IS_AUDIOBOOKS_GENRE_BUTTON(self) ? self->genre_name : NULL;
}

void koto_audiobooks_genre_button_set_id(
	KotoAudiobooksGenreButton * self,
	gchar * genre_id
) {
	if (!KOTO_IS_AUDIOBOOKS_GENRE_BUTTON(self)) {
		return;
	}

	if (!koto_utils_string_is_valid(genre_id)) {
		return;
	}

	self->genre_id = genre_id;

	gtk_picture_set_resource(GTK_PICTURE(self->bg_image), g_strdup_printf("/com/github/joshstrobl/koto/%s.png", genre_id));
	gtk_widget_set_size_request(self->bg_image, 260, 120);
}

void koto_audiobooks_genre_button_set_name(
	KotoAudiobooksGenreButton * self,
	gchar * genre_name
) {
	if (!KOTO_IS_AUDIOBOOKS_GENRE_BUTTON(self)) {
		return;
	}

	if (!koto_utils_string_is_valid(genre_name)) {
		return;
	}

	if (koto_utils_string_is_valid(self->genre_name)) { // Already have a genre name
		g_free(self->genre_name);
	}

	self->genre_name = genre_name;
	koto_button_set_text(self->button, genre_name);
}

KotoAudiobooksGenreButton * koto_audiobooks_genre_button_new(
	gchar * genre_id,
	gchar * genre_name
) {
	return g_object_new(
		KOTO_TYPE_AUDIOBOOKS_GENRE_BUTTON,
		"genre-id",
		genre_id,
		"genre-name",
		genre_name,
		NULL
	);
}