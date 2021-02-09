/* koto-expander.c
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

#include <glib/gi18n.h>
#include <gtk-3.0/gtk/gtk.h>
#include "koto-config.h"
#include "koto-button.h"
#include "koto-flipper-button.h"
#include "koto-expander.h"
#include "koto-utils.h"

enum {
	PROP_0,
	PROP_HEADER_ICON_NAME,
	PROP_HEADER_LABEL,
	PROP_HEADER_SECONDARY_BUTTON,
	PROP_CONTENT,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

struct _KotoExpander {
	GtkBox parent_instance;
	gboolean constructed;
	GtkWidget *header;
	KotoButton *header_button;

	gchar *icon_name;
	gchar *label;

	GtkWidget *header_secondary_button;
	KotoFlipperButton *header_expand_button;

	GtkWidget *revealer;
	GtkWidget *content;
};

struct _KotoExpanderClass {
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(KotoExpander, koto_expander, GTK_TYPE_BOX);

static void koto_expander_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_expander_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_expander_class_init(KotoExpanderClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_expander_set_property;
	gobject_class->get_property = koto_expander_get_property;

	props[PROP_HEADER_ICON_NAME] = g_param_spec_string(
		"icon-name",
		"Icon Name",
		"Name of the icon to use in the Expander",
		"emblem-favorite-symbolic",
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_HEADER_LABEL] = g_param_spec_string(
		"label",
		"Label",
		"Label for the Expander",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_HEADER_SECONDARY_BUTTON] = g_param_spec_object(
		"secondary-button",
		"Secondary Button",
		"Secondary Button to be placed next to Expander button",
		GTK_TYPE_WIDGET,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_CONTENT] = g_param_spec_object(
		"content",
		"Content",
		"Content inside the Expander",
		GTK_TYPE_WIDGET,
		G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_expander_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoExpander *self = KOTO_EXPANDER(obj);

	switch (prop_id) {
		case PROP_HEADER_ICON_NAME:
			g_value_set_string(val, self->icon_name);
			break;
		case PROP_HEADER_LABEL:
			g_value_set_string(val, self->label);
			break;
		case PROP_HEADER_SECONDARY_BUTTON:
			g_value_set_object(val, (GObject *) self->header_secondary_button);
			break;
		case PROP_CONTENT:
			g_value_set_object(val, (GObject *) self->content);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_expander_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
		KotoExpander *self = KOTO_EXPANDER(obj);

		if (!GTK_IS_WIDGET(self->header_button)) { // Header Button is not a widget
			KotoButton *new_button = koto_button_new_with_icon("Temporary Text", "emblem-favorite-symbolic", KOTO_BUTTON_PIXBUF_SIZE_SMALL);

			if (GTK_IS_WIDGET(new_button)) { // Created our widget successfully
				self->header_button = new_button;
				gtk_box_pack_start(GTK_BOX(self->header), GTK_WIDGET(self->header_button), TRUE, TRUE, 0); // Add it
			}
		}

		switch (prop_id) {
			case PROP_HEADER_ICON_NAME:
				g_return_if_fail(GTK_IS_WIDGET(self->header_button));
				koto_button_set_icon_name(self->header_button, g_strdup(g_value_get_string(val)));
				break;
			case PROP_HEADER_LABEL:
				g_return_if_fail(GTK_IS_WIDGET(self->header_button));
				koto_button_set_text(self->header_button, g_strdup(g_value_get_string(val)));
				break;
			case PROP_HEADER_SECONDARY_BUTTON:
				koto_expander_set_secondary_button(self, (GtkWidget*) g_value_get_object(val));
				break;
			case PROP_CONTENT:
				koto_expander_set_content(self, (GtkWidget*) g_value_get_object(val));
				break;
			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
				break;
		}
}

static void koto_expander_init(KotoExpander *self) {
	GtkSettings *settings = gtk_settings_get_default();
	g_object_set(settings, "gtk_application_prefer_dark_theme", TRUE, NULL);

	GtkStyleContext *style = gtk_widget_get_style_context(GTK_WIDGET(self));
	gtk_style_context_add_class(style, "expander");

	self->header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

	GtkStyleContext *header_style = gtk_widget_get_style_context(self->header);
	gtk_style_context_add_class(header_style, "expander-header");

	self->revealer = gtk_revealer_new();
	gtk_revealer_set_reveal_child(GTK_REVEALER(self->revealer), TRUE); // Set to be revealed by default
	gtk_revealer_set_transition_type(GTK_REVEALER(self->revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);

	self->header_expand_button = koto_flipper_button_new("pan-down-symbolic", "pan-up-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_box_pack_end(GTK_BOX(self->header), GTK_WIDGET(self->header_expand_button), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(self), self->header, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(self), self->revealer, TRUE, TRUE, 0);

	self->constructed = TRUE;

	g_signal_connect(self->header_expand_button, "clicked", G_CALLBACK(koto_expander_toggle_content), self);
}

void koto_expander_set_secondary_button(KotoExpander *self, GtkWidget *new_button) {
	if (!self->constructed) {
		return;
	}

	if (!GTK_IS_BUTTON(new_button)) {
		return;
	}

	if (GTK_IS_BUTTON(self->header_secondary_button)) { // Already have a button
		gtk_container_remove(GTK_CONTAINER(self->header), self->header_secondary_button);
		//g_free(self->header_secondary_button);
	}

	self->header_secondary_button = new_button;
	gtk_box_pack_end(GTK_BOX(self->header), self->header_secondary_button, FALSE, FALSE, 0);

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_HEADER_SECONDARY_BUTTON]);
}

void koto_expander_set_content(KotoExpander *self, GtkWidget *new_content) {
	if (!self->constructed) {
		return;
	}

	if (self->content != NULL) { // Already have content
		gtk_container_remove(GTK_CONTAINER(self->revealer), self->content);
		g_free(self->content);
	}

	self->content = new_content;
	gtk_container_add(GTK_CONTAINER(self->revealer), self->content);

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_CONTENT]);
}

void koto_expander_toggle_content(GtkWidget *button, gpointer data) {
	KotoExpander* self = data;
	koto_flipper_button_flip(KOTO_FLIPPER_BUTTON(button));
	GtkRevealer* rev = GTK_REVEALER(self->revealer);
	gtk_revealer_set_reveal_child(rev, !gtk_revealer_get_reveal_child(rev)); // Invert our values
}

KotoExpander* koto_expander_new(gchar *primary_icon_name, gchar *primary_label_text) {
	return g_object_new(KOTO_TYPE_EXPANDER,
		"orientation", GTK_ORIENTATION_VERTICAL,
		"icon-name", primary_icon_name,
		"label", primary_label_text,
		NULL
	);
}

KotoExpander* koto_expander_new_with_button(gchar *primary_icon_name, gchar *primary_label_text, GtkWidget *secondary_button) {
	return g_object_new(KOTO_TYPE_EXPANDER,
		"orientation", GTK_ORIENTATION_VERTICAL,
		"icon-name", primary_icon_name,
		"label", primary_label_text,
		"secondary-button", secondary_button,
		NULL
	);
}
