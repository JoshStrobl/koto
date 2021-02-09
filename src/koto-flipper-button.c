/* koto-flipper-button.c
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

#include <gtk-3.0/gtk/gtk.h>
#include "koto-config.h"
#include "koto-flipper-button.h"

enum {
	PROP_0,
	PROP_SIZE,
	PROP_INITIAL_IMAGE,
	PROP_FLIPPED_IMAGE,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

struct _KotoFlipperButton {
	GtkButton parent_instance;
	GtkIconSize size;
	gboolean flipped;

	gchar *initial_image;
	gchar *flipped_image;
};

struct _KotoFlipperButtonClass {
	GtkButtonClass parent_class;
};

G_DEFINE_TYPE(KotoFlipperButton, koto_flipper_button, GTK_TYPE_BUTTON);
static void koto_flipper_button_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_flipper_button_class_init(KotoFlipperButtonClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_flipper_button_set_property;

	props[PROP_SIZE] = g_param_spec_enum(
		"size",
		"Icon Size",
		"Size of the icon",
		GTK_TYPE_ICON_SIZE,
		GTK_ICON_SIZE_MENU,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_WRITABLE
	);

	props[PROP_INITIAL_IMAGE] = g_param_spec_string(
		"initial-image",
		"Initial Image",
		"Image to use initially for the button",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_WRITABLE
	);

	props[PROP_FLIPPED_IMAGE] = g_param_spec_string(
		"flipped-image",
		"Flipped Image",
		"Image to use when the button is flipped",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_WRITABLE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_flipper_button_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoFlipperButton *self = KOTO_FLIPPER_BUTTON(obj);

	if (prop_id == PROP_INITIAL_IMAGE) {
		g_free(self->initial_image);
		self->initial_image = g_strdup(g_value_get_string(val));
		koto_flipper_button_set_icon(self, self->initial_image);
	} else if (prop_id == PROP_FLIPPED_IMAGE) {
		g_free(self->flipped_image);
		self->flipped_image = g_strdup(g_value_get_string(val));
	} else if (prop_id == PROP_SIZE) {
		self->size = (GtkIconSize) g_value_get_enum(val);

		if (self->initial_image != NULL && !self->flipped) {
			koto_flipper_button_set_icon(self, self->flipped ? self->initial_image : self->flipped_image);
		}
	} else {
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
		return;
	}

	g_object_notify_by_pspec(obj, props[prop_id]);
}

static void koto_flipper_button_init(KotoFlipperButton *self) {
	self->flipped = FALSE;

	GtkStyleContext *button_style = gtk_widget_get_style_context(GTK_WIDGET(self)); // Get the styling for the flipper button
	gtk_style_context_add_class(button_style, "flat"); // Set it to flat
}

void koto_flipper_button_flip(KotoFlipperButton *self) {
	koto_flipper_button_set_icon(self, self->flipped ? self->initial_image : self->flipped_image);
	self->flipped = !self->flipped;
}

void koto_flipper_button_set_icon(KotoFlipperButton *self, gchar *name) {
	GtkWidget *new_image = gtk_image_new_from_icon_name(name, self->size);
	gtk_button_set_image(GTK_BUTTON(self), new_image);
}

KotoFlipperButton* koto_flipper_button_new(gchar *initial_icon, gchar *flipped_icon, GtkIconSize size) {
	return g_object_new(KOTO_TYPE_FLIPPER_BUTTON,
		"initial-image", initial_icon,
		"flipped-image", flipped_icon,
		"size", size,
		NULL
	);
}
