/* koto-button.c
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
#include "koto-button.h"
#include "koto-config.h"

struct _PixbufSize {
	guint size;
};

typedef struct _PixbufSize PixbufSize;

static PixbufSize *pixbuf_sizes = NULL;
static guint pixbuf_sizes_allocated = 0;

static void init_pixbuf_sizes() {
	if (pixbuf_sizes == NULL) {
		pixbuf_sizes = g_new(PixbufSize, NUM_BUILTIN_SIZES);

		pixbuf_sizes[KOTO_BUTTON_PIXBUF_SIZE_INVALID].size = 0;
		pixbuf_sizes[KOTO_BUTTON_PIXBUF_SIZE_TINY].size = 16;
		pixbuf_sizes[KOTO_BUTTON_PIXBUF_SIZE_SMALL].size = 24;
		pixbuf_sizes[KOTO_BUTTON_PIXBUF_SIZE_NORMAL].size = 32;
		pixbuf_sizes[KOTO_BUTTON_PIXBUF_SIZE_LARGE].size = 64;
		pixbuf_sizes[KOTO_BUTTON_PIXBUF_SIZE_MASSIVE].size = 96;
		pixbuf_sizes[KOTO_BUTTON_PIXBUF_SIZE_GODLIKE].size = 128;
		pixbuf_sizes_allocated = NUM_BUILTIN_SIZES;
	}
}

guint koto_get_pixbuf_size(KotoButtonPixbufSize s) {
	init_pixbuf_sizes();

	return pixbuf_sizes[s].size;
}

enum {
	PROP_0,
	PROP_PIX_SIZE,
	PROP_TEXT,
	PROP_BADGE_TEXT,
	PROP_PIX,
	PROP_ICON_NAME,
	N_PROPERTIES
};

static GParamSpec *props[N_PROPERTIES] = { NULL, };

struct _KotoButton {
	GtkBox parent_instance;
	guint pix_size;

	GtkWidget *button_image;
	GtkWidget *badge_label;
	GtkWidget *button_label;

	gchar *badge_text;
	gchar *icon_name;
	gchar *text;

	GdkPixbuf *pix;
};

struct _KotoButtonClass {
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(KotoButton, koto_button, GTK_TYPE_BOX);

static void koto_button_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec);
static void koto_button_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec);

static void koto_button_class_init(KotoButtonClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_button_set_property;
	gobject_class->get_property = koto_button_get_property;

	props[PROP_PIX_SIZE] = g_param_spec_uint(
		"pixbuf-size",
		"Pixbuf Size",
		"Size of the pixbuf",
		koto_get_pixbuf_size(KOTO_BUTTON_PIXBUF_SIZE_TINY),
		koto_get_pixbuf_size(KOTO_BUTTON_PIXBUF_SIZE_GODLIKE),
		koto_get_pixbuf_size(KOTO_BUTTON_PIXBUF_SIZE_SMALL),
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_TEXT] = g_param_spec_string(
		"button-text",
		"Button Text",
		"Text of Button",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_BADGE_TEXT] = g_param_spec_string(
		"badge-text",
		"Badge Text",
		"Text of Badge",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_PIX] = g_param_spec_object(
		"pixbuf",
		"Pixbuf",
		"Pixbuf",
		GDK_TYPE_PIXBUF,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	props[PROP_ICON_NAME] = g_param_spec_string(
		"icon-name",
		"Icon Name",
		"Name of Icon",
		NULL,
		G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_button_init(KotoButton *self) {
	GtkStyleContext *style = gtk_widget_get_style_context(GTK_WIDGET(self));
	gtk_style_context_add_class(style, "koto-button");
}

static void koto_button_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *spec) {
	KotoButton *self = KOTO_BUTTON(obj);

	switch (prop_id) {
		case PROP_PIX_SIZE:
			g_value_set_uint(val, self->pix_size);
			break;
		case PROP_TEXT:
			g_value_set_string(val, self->text);
			break;
		case PROP_BADGE_TEXT:
			g_value_set_string(val, self->badge_text);
			break;
		case PROP_ICON_NAME:
			g_value_set_string(val, self->icon_name);
			break;
		case PROP_PIX:
			g_value_set_object(val, self->pix);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_button_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *spec) {
	KotoButton *self = KOTO_BUTTON(obj);

	switch (prop_id) {
		case PROP_PIX_SIZE:
			koto_button_set_pixbuf_size(self, g_value_get_uint(val));
			break;
		case PROP_TEXT:
			koto_button_set_text(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_BADGE_TEXT:
			koto_button_set_badge_text(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_PIX:
			koto_button_set_pixbuf(self, (GdkPixbuf*) g_value_get_object(val));
			break;
		case PROP_ICON_NAME:
			koto_button_set_icon_name(self, g_strdup(g_value_get_string(val)));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_button_set_badge_text(KotoButton *self, gchar *text) {
	if ((text == NULL) || (strcmp(text, "") == 0)) { // If the text is empty
		self->badge_text = g_strdup("");
	} else {
		g_free(self->badge_text);
		self->badge_text = g_strdup(text);
	}

	if (GTK_IS_LABEL(self->badge_label)) { // If badge label already exists
		gtk_label_set_text(GTK_LABEL(self->badge_label), self->badge_text);
	} else {
		self->badge_label = gtk_label_new(self->badge_text); // Create our label
		gtk_box_pack_end(GTK_BOX(self), self->badge_label, FALSE, FALSE, 0); // Add to the end of the box
	}

	if (strcmp(self->badge_text, "") != 0) { // Empty badge
		gtk_widget_hide(self->badge_label); // Hide our badge
	} else { // Have some text
		gtk_widget_show(self->badge_label); // Show our badge
	}

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_BADGE_TEXT]);
}

void koto_button_set_icon_name(KotoButton *self, gchar *icon_name) {
	gchar *copied_icon_name = g_strdup(icon_name);
	g_message("icon name set in koto button: %s", icon_name);
	g_free(self->icon_name);
	self->icon_name = copied_icon_name;

	if ((self->icon_name == NULL) || (strcmp(self->icon_name,"") == 0)) { // Have no icon name now
		g_message("Have no icon name now?");
		if (GTK_IS_IMAGE(self->button_image)) { // If we already have a button image
			gtk_widget_hide(self->button_image); // Hide
		}

		return;
	}

	GtkIconTheme *theme = gtk_icon_theme_get_default(); // Get the default icon theme
	GdkPixbuf* icon_pix = gtk_icon_theme_load_icon_for_scale(theme, self->icon_name, (gint) self->pix_size, 1, GTK_ICON_LOOKUP_USE_BUILTIN & GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
	g_return_if_fail(GDK_IS_PIXBUF(icon_pix)); // Return if not a pixbuf
	koto_button_set_pixbuf(self, icon_pix);

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_ICON_NAME]);
}

void koto_button_set_pixbuf(KotoButton *self, GdkPixbuf *pix) {
	g_return_if_fail(self->pix_size != 0); // Return if don't have a pixbuf size to indicate what to scale to

	if (!GDK_IS_PIXBUF(pix)) {
		return;
	}

	self->pix = pix;

	GdkPixbuf *use_pix = pix; // Default to using our provided pixbuf
	if (gdk_pixbuf_get_height(pix) > (int) self->pix_size) { // If our height is greater than our desired pixel size
		use_pix = gdk_pixbuf_scale_simple(pix, -1, self->pix_size, GDK_INTERP_BILINEAR); // Scale using bilnear
		g_return_if_fail(use_pix != NULL); // Return if we could not allocate memory for the scaling
	}

	if (GTK_IS_IMAGE(self->button_image)) { // If we already have a button image
		gtk_image_set_from_pixbuf(GTK_IMAGE(self->button_image), use_pix); // Update our pixbuf
		gtk_widget_show(self->button_image); // Ensure we show the image
	} else { // No image set
		GtkWidget *new_image = gtk_image_new_from_pixbuf(use_pix); // Create our new image
		g_return_if_fail(GTK_IS_IMAGE(new_image)); // Return if we failed to create our image
		self->button_image = new_image;
		gtk_box_pack_start(GTK_BOX(self), self->button_image, FALSE, FALSE, 0); // Prepend the image
		gtk_box_reorder_child(GTK_BOX(self), self->button_image, 0);
	}

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_PIX]);
}

void koto_button_set_pixbuf_size(KotoButton *self, guint size) {
	g_return_if_fail(size != self->pix_size); // If the sizes aren't different, return

	self->pix_size = size;

	if (GDK_PIXBUF(self->pix)) { // If we already have a pixbuf set and we're changing the size
		koto_button_set_pixbuf(self, self->pix);
	}

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_PIX_SIZE]);
}

void koto_button_set_text(KotoButton *self, gchar *text) {
	gchar *copied_text = g_strdup(text); // Copy our text

	if (strcmp(copied_text, "") == 0) { // Clearing our text
		g_free(self->text); // Free existing text
	}

	self->text = copied_text;

	if (GTK_IS_LABEL(self->button_label)) { // If we have a button label
		if (strcmp(self->text, "") != 0) { // Have text set
			gtk_label_set_text(GTK_LABEL(self->button_label), self->text);
			gtk_widget_show(self->button_label); // Show the label
		} else { // Have a label but no longer text
			gtk_container_remove(GTK_CONTAINER(self), self->button_label); // Remove the label
			g_free(self->button_label);
		}
	} else { // If we do not have a button label
		if (strcmp(self->text, "") != 0) { // If we have text
			self->button_label = gtk_label_new(self->text); // Create our label
			gtk_label_set_xalign(GTK_LABEL(self->button_label), 0);
			gtk_box_pack_start(GTK_BOX(self), self->button_label, FALSE, FALSE, 0); // Add to the beginning of the box

			if (GTK_IS_IMAGE(self->button_image)) { // If we have an image
				gtk_box_reorder_child(GTK_BOX(self), self->button_image, 0); // Move to the beginning
			}
		}
	}

	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_TEXT]);
}

KotoButton* koto_button_new_plain(gchar *label) {
	return g_object_new(KOTO_TYPE_BUTTON,
		"button-text", label,
		"orientation", GTK_ORIENTATION_HORIZONTAL,
		NULL
	);
}

KotoButton* koto_button_new_with_icon(gchar *label, gchar *icon_name, KotoButtonPixbufSize size) {
	return g_object_new(KOTO_TYPE_BUTTON,
		"button-text", label,
		"icon-name", icon_name,
		"orientation", GTK_ORIENTATION_HORIZONTAL,
		"pixbuf-size", koto_get_pixbuf_size(size),
		NULL
	);
}

KotoButton* koto_button_new_with_pixbuf(gchar *label, GdkPixbuf *pix, KotoButtonPixbufSize size) {
	return g_object_new(KOTO_TYPE_BUTTON,
		"button-text", label,
		"pixbuf", pix,
		"orientation", GTK_ORIENTATION_HORIZONTAL,
		"pixbuf-size", koto_get_pixbuf_size(size),
		NULL
	);
}
