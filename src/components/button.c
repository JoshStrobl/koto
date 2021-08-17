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

#include <gtk-4.0/gtk/gtk.h>
#include "config/config.h"
#include "button.h"
#include "../koto-window.h"
#include "../koto-utils.h"

extern KotoWindow * main_window;

struct _PixbufSize {
	guint size;
};

typedef struct _PixbufSize PixbufSize;

static PixbufSize * pixbuf_sizes = NULL;
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
	PROP_BTN_0,
	PROP_PIX_SIZE,
	PROP_TEXT,
	PROP_BADGE_TEXT,
	PROP_USE_FROM_FILE,
	PROP_IMAGE_FILE_PATH,
	PROP_ICON_NAME,
	PROP_ALT_ICON_NAME,
	PROP_RESOURCE_PATH,
	N_BTN_PROPERTIES
};

static GParamSpec * btn_props[N_BTN_PROPERTIES] = {
	NULL,
};

struct _KotoButton {
	GtkBox parent_instance;
	guint pix_size;

	gpointer arbitrary_data;

	GtkWidget * button_pic;
	GtkWidget * badge_label;
	GtkWidget * button_label;

	GtkGesture * left_click_gesture;
	GtkGesture * right_click_gesture;

	gchar * image_file_path;
	gchar * badge_text;
	gchar * icon_name;
	gchar * alt_icon_name;
	gchar * resource_path;
	gchar * text;

	KotoButtonImagePosition image_position;
	gboolean use_from_file;
	gboolean currently_showing_alt;
};

struct _KotoButtonClass {
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(KotoButton, koto_button, GTK_TYPE_BOX);

static void koto_button_constructed(GObject * obj);

static void koto_button_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_button_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_button_class_init(KotoButtonClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->constructed = koto_button_constructed;
	gobject_class->set_property = koto_button_set_property;
	gobject_class->get_property = koto_button_get_property;

	btn_props[PROP_PIX_SIZE] = g_param_spec_uint(
		"pixbuf-size",
		"Pixbuf Size",
		"Size of the pixbuf",
		koto_get_pixbuf_size(KOTO_BUTTON_PIXBUF_SIZE_TINY),
		koto_get_pixbuf_size(KOTO_BUTTON_PIXBUF_SIZE_GODLIKE),
		koto_get_pixbuf_size(KOTO_BUTTON_PIXBUF_SIZE_SMALL),
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	btn_props[PROP_TEXT] = g_param_spec_string(
		"button-text",
		"Button Text",
		"Text of Button",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	btn_props[PROP_BADGE_TEXT] = g_param_spec_string(
		"badge-text",
		"Badge Text",
		"Text of Badge",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	btn_props[PROP_USE_FROM_FILE] = g_param_spec_boolean(
		"use-from-file",
		"Use from a file / file name",
		"Use from a file / file name",
		FALSE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	btn_props[PROP_IMAGE_FILE_PATH] = g_param_spec_string(
		"image-file-path",
		"File path to image",
		"File path to image",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	btn_props[PROP_ICON_NAME] = g_param_spec_string(
		"icon-name",
		"Icon Name",
		"Name of Icon",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	btn_props[PROP_ALT_ICON_NAME] = g_param_spec_string(
		"alt-icon-name",
		"Name of an Alternate Icon",
		"Name of an Alternate Icon",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	btn_props[PROP_RESOURCE_PATH] = g_param_spec_string(
		"resource-path",
		"Resource Path to an Icon",
		"Resource Path to an Icon",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_BTN_PROPERTIES, btn_props);
}

static void koto_button_init(KotoButton * self) {
	self->currently_showing_alt = FALSE;
	self->image_position = KOTO_BUTTON_IMAGE_POS_LEFT;

	self->left_click_gesture = gtk_gesture_click_new(); // Set up our left click gesture
	self->right_click_gesture = gtk_gesture_click_new(); // Set up our right click gesture

	self->resource_path = NULL;

	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(self->left_click_gesture), (int) KOTO_BUTTON_CLICK_TYPE_PRIMARY); // Only allow left clicks on left click gesture
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(self->right_click_gesture), (int) KOTO_BUTTON_CLICK_TYPE_SECONDARY); // Only allow right clicks on right click gesture

	gtk_widget_add_controller(GTK_WIDGET(self), GTK_EVENT_CONTROLLER(self->left_click_gesture)); // Add our left click gesture
	gtk_widget_add_controller(GTK_WIDGET(self), GTK_EVENT_CONTROLLER(self->right_click_gesture)); // Add our right click gesture

	GtkEventController * motion = gtk_event_controller_motion_new(); // Create a new motion controller
	g_signal_connect(motion, "enter", G_CALLBACK(koto_button_handle_mouse_enter), self); // Handle mouse enter on motion
	g_signal_connect(motion, "leave", G_CALLBACK(koto_button_handle_mouse_leave), self); // Handle mouse leave on motion
	gtk_widget_add_controller(GTK_WIDGET(self), motion);
}

static void koto_button_constructed(GObject * obj) {
	KotoButton * self = KOTO_BUTTON(obj);
	GtkStyleContext * style = gtk_widget_get_style_context(GTK_WIDGET(self));

	gtk_style_context_add_class(style, "koto-button");

	G_OBJECT_CLASS(koto_button_parent_class)->constructed(obj);
}

static void koto_button_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoButton * self = KOTO_BUTTON(obj);

	switch (prop_id) {
		case PROP_IMAGE_FILE_PATH:
			g_value_set_string(val, self->image_file_path);
			break;
		case PROP_USE_FROM_FILE:
			g_value_set_boolean(val, self->use_from_file);
			break;
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
		case PROP_ALT_ICON_NAME:
			g_value_set_string(val, self->alt_icon_name);
			break;
		case PROP_RESOURCE_PATH:
			g_value_set_string(val, self->resource_path);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_button_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoButton * self = KOTO_BUTTON(obj);

	switch (prop_id) {
		case PROP_PIX_SIZE:
			koto_button_set_pixbuf_size(self, g_value_get_uint(val));
			break;
		case PROP_TEXT:
			if (koto_utils_string_is_valid(g_value_get_string(val))) {
				koto_button_set_text(self, (gchar*) g_value_get_string(val));
			}

			break;
		case PROP_BADGE_TEXT:
			koto_button_set_badge_text(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_USE_FROM_FILE:
			self->use_from_file = g_value_get_boolean(val);
			break;
		case PROP_IMAGE_FILE_PATH:
			koto_button_set_file_path(self, (gchar*) g_value_get_string(val));
			break;
		case PROP_ICON_NAME:
			koto_button_set_icon_name(self, g_strdup(g_value_get_string(val)), FALSE);
			if (!self->currently_showing_alt) { // Not showing alt
				koto_button_show_image(self, FALSE);
			}
			break;
		case PROP_ALT_ICON_NAME:
			koto_button_set_icon_name(self, g_strdup(g_value_get_string(val)), TRUE);
			if (self->currently_showing_alt) { // Currently showing the alt image
				koto_button_show_image(self, TRUE);
			}
			break;
		case PROP_RESOURCE_PATH:
			koto_button_set_resource_path(self, g_strdup(g_value_get_string(val)));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_button_add_click_handler(
	KotoButton * self,
	KotoButtonClickType button,
	GCallback handler,
	gpointer user_data
) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	if ((button != KOTO_BUTTON_CLICK_TYPE_PRIMARY) && (button != KOTO_BUTTON_CLICK_TYPE_SECONDARY)) { // Not valid type
		return;
	}

	g_signal_connect((button == KOTO_BUTTON_CLICK_TYPE_PRIMARY) ? self->left_click_gesture : self->right_click_gesture, "pressed", handler, user_data);
}

void koto_button_flip(KotoButton * self) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	koto_button_show_image(self, !self->currently_showing_alt);
}

gpointer koto_button_get_data(KotoButton * self) {
	return self->arbitrary_data;
}

void koto_button_global_page_nav_callback(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
) {
	(void) gesture;
	(void) n_press;
	(void) x;
	(void) y;

	KotoButton * btn = KOTO_BUTTON(user_data); // Cast our user data as a button

	if (!KOTO_IS_BUTTON(btn)) { // Not a button
		return;
	}

	gchar * btn_nav_uuid = (gchar*) koto_button_get_data(btn); // Get the data

	if (!koto_utils_string_is_valid(btn_nav_uuid)) { // Not a valid string
		return;
	}

	koto_window_go_to_page(main_window, btn_nav_uuid);
}

void koto_button_handle_mouse_enter(
	GtkEventControllerMotion * controller,
	double x,
	double y,
	gpointer user_data
) {
	(void) controller;
	(void) x;
	(void) y;

	KotoButton * self = user_data;

	if (!KOTO_IS_BUTTON(self)) { // Not a button
		return;
	}

	koto_button_set_pseudoactive_styling(self); // Set to be pseudoactive in styling
}

void koto_button_handle_mouse_leave(
	GtkEventControllerMotion * controller,
	gpointer user_data
) {
	(void) controller;

	KotoButton * self = user_data;

	if (!KOTO_IS_BUTTON(self)) { // Not a button
		return;
	}

	koto_button_unset_pseudoactive_styling(self); // Unset the pseudoactive styling
}

void koto_button_hide_image(KotoButton * self) {
	if (!KOTO_IS_BUTTON(self)) { // Not a button
		return;
	}

	if (GTK_IS_WIDGET(self->button_pic)) { // Is a widget
		gtk_widget_hide(self->button_pic);
	}
}

void koto_button_set_data(
	KotoButton * self,
	gpointer data
) {
	if (!KOTO_IS_BUTTON(self)) { // Not a button
		return;
	}

	self->arbitrary_data = data;
}

void koto_button_set_pseudoactive_styling(KotoButton * self) {
	if (!KOTO_IS_BUTTON(self)) { // Not a button
		return;
	}

	if (gtk_widget_has_css_class(GTK_WIDGET(self), "pseudoactive")) { // Already is pseudoactive
		return;
	}

	gtk_widget_add_css_class(GTK_WIDGET(self), "pseudoactive"); // Set to pseudoactive
}

void koto_button_set_badge_text(
	KotoButton * self,
	gchar * text
) {
	if (!KOTO_IS_BUTTON(self)) { // Not a button
		return;
	}

	if (koto_utils_string_is_valid(self->badge_text)) { // Have existing text
		g_free(self->badge_text);
	}

	if (!koto_utils_string_is_valid(text)) { // If the text is empty
		self->badge_text = NULL;

		if (GTK_IS_LABEL(self->badge_label)) { // If badge label already exists
			gtk_widget_hide(self->badge_label); // Hide the label
		}

		return;
	}

	self->badge_text = g_strdup(text);

	if (GTK_IS_LABEL(self->badge_label)) { // If badge label already exists
		gtk_label_set_text(GTK_LABEL(self->badge_label), self->badge_text);
	} else {
		self->badge_label = gtk_label_new(self->badge_text); // Create our label
		gtk_box_append(GTK_BOX(self), self->badge_label);
	}

	g_object_notify_by_pspec(G_OBJECT(self), btn_props[PROP_BADGE_TEXT]);
}

void koto_button_set_file_path(
	KotoButton * self,
	gchar * file_path
) {
	if (!KOTO_IS_BUTTON(self)) { // Not a button
		return;
	}

	if (!koto_utils_string_is_valid(file_path)) { // file path is invalid
		return;
	}

	if (g_strcmp0(self->image_file_path, file_path) == 0) { // Request setting as same file
		return;
	}

	if (koto_utils_string_is_valid(self->image_file_path)) { // image file path is valid
		g_free(self->image_file_path);
	}

	self->use_from_file = TRUE;
	self->image_file_path = g_strdup(file_path);
	koto_button_show_image(self, FALSE);
}

void koto_button_set_icon_name(
	KotoButton * self,
	gchar * icon_name,
	gboolean for_alt
) {
	if (!KOTO_IS_BUTTON(self)) { // Not a button
		return;
	}

	if (!koto_utils_string_is_valid(icon_name)) { // Not a valid icon
		return;
	}

	gchar * copied_icon_name = g_strdup(icon_name);

	if (for_alt) { // Is for the alternate icon
		if ((self->alt_icon_name != NULL) && strcmp(icon_name, self->alt_icon_name) != 0) { // If the icons are different
			g_free(self->alt_icon_name);
		}

		self->alt_icon_name = copied_icon_name;
	} else {
		if ((self->icon_name != NULL) && strcmp(icon_name, self->icon_name) != 0) {
			g_free(self->icon_name);
		}

		self->icon_name = copied_icon_name;
	}

	gboolean hide_image = FALSE;

	if (for_alt && self->currently_showing_alt && ((self->alt_icon_name == NULL) || strcmp(self->alt_icon_name, "") == 0)) { // For alt, alt is currently showing, and no longer have alt
		hide_image = TRUE;
	} else if (!for_alt && ((self->icon_name == NULL) || (strcmp(self->icon_name, "") == 0))) { // Not for alt, no icon
		hide_image = TRUE;
	}

	if (GTK_IS_IMAGE(self->button_pic)) { // If we already have a button image
		if (hide_image) { // Should hide the image
			gtk_widget_hide(self->button_pic); // Hide
		} else { // Should be showing the image
			gtk_image_set_from_icon_name(GTK_IMAGE(self->button_pic), self->icon_name); // Just update the existing image immediately
		}
	}

	g_object_notify_by_pspec(G_OBJECT(self), for_alt ? btn_props[PROP_ALT_ICON_NAME] : btn_props[PROP_ICON_NAME]);
}

void koto_button_set_image_position(
	KotoButton * self,
	KotoButtonImagePosition pos
) {
	if (!KOTO_IS_BUTTON(self)) { // Not a button
		return;
	}

	if (self->image_position == pos) { // Is a different position that currently
		return;
	}

	if (GTK_IS_WIDGET(self->button_pic)) { // Button is already defined
		if (pos == KOTO_BUTTON_IMAGE_POS_RIGHT) { // If we want to move the image to the right
			gtk_box_reorder_child_after(GTK_BOX(self), self->button_pic, self->button_label); // Move image to after label
		} else { // Moving image to left
			gtk_box_reorder_child_after(GTK_BOX(self), self->button_label, self->button_pic); // Move label to after image
		}
	}

	self->image_position = pos;
}

void koto_button_set_pixbuf_size(
	KotoButton * self,
	guint size
) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	if (size == self->pix_size) {
		return;
	}

	self->pix_size = size;
	gtk_widget_set_size_request(GTK_WIDGET(self), self->pix_size, self->pix_size);

	g_object_notify_by_pspec(G_OBJECT(self), btn_props[PROP_PIX_SIZE]);
}

void koto_button_set_resource_path(
	KotoButton * self,
	gchar * resource_path
) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	if (!koto_utils_string_is_valid(resource_path)) { // Not a valid string
		return;
	}

	if (koto_utils_string_is_valid(self->resource_path)) { // Have a resource path already
		g_free(self->resource_path); // Free it
	}

	self->resource_path = g_strdup(resource_path);

	if (GTK_IS_IMAGE(self->button_pic)) { // Already have a button image
		gtk_image_set_from_resource(GTK_IMAGE(self->button_pic), self->resource_path);
	} else {
		self->button_pic = gtk_image_new_from_resource(self->resource_path); // Create a new image from the resource
		gtk_image_set_pixel_size(GTK_IMAGE(self->button_pic), self->pix_size);
		gtk_image_set_icon_size(GTK_IMAGE(self->button_pic), GTK_ICON_SIZE_INHERIT); // Inherit height of parent widget
		gtk_box_prepend(GTK_BOX(self), self->button_pic); // Prepend to the box
	}
}

void koto_button_set_text(
	KotoButton * self,
	gchar * text
) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	if (!koto_utils_string_is_valid(text)) { // Invalid text
		return;
	}

	if (koto_utils_string_is_valid(self->text)) { // Text defined
		g_free(self->text); // Free existing text
	}

	self->text = g_strdup(text);

	if (GTK_IS_LABEL(self->button_label)) { // If we have a button label
		if (koto_utils_string_is_valid(self->text)) { // Have text set
			gtk_label_set_text(GTK_LABEL(self->button_label), self->text);
			gtk_widget_show(self->button_label); // Show the label
		} else { // Have a label but no longer text
			gtk_box_remove(GTK_BOX(self), self->button_label);
			g_free(self->button_label);
		}
	} else { // If we do not have a button label
		if (koto_utils_string_is_valid(self->text)) { // If we have text
			self->button_label = gtk_label_new(self->text); // Create our label
			gtk_widget_add_css_class(self->button_label, "button-label");
			gtk_widget_set_hexpand(self->button_label, TRUE);
			gtk_label_set_xalign(GTK_LABEL(self->button_label), 0);

			if (GTK_IS_IMAGE(self->button_pic)) { // If we have an image
				gtk_box_insert_child_after(GTK_BOX(self), self->button_label, self->button_pic);
			} else {
				gtk_box_prepend(GTK_BOX(self), GTK_WIDGET(self->button_label));
			}
		}
	}

	g_object_notify_by_pspec(G_OBJECT(self), btn_props[PROP_TEXT]);
}

void koto_button_set_text_justify(
	KotoButton * self,
	GtkJustification j
) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	if (!GTK_IS_LABEL(self->button_label)) { // If we do not have a button label
		return;
	}

	if (j == GTK_JUSTIFY_CENTER) { // Center text
		gtk_label_set_xalign(GTK_LABEL(self->button_label), 0.5);
	} else if (j == GTK_JUSTIFY_RIGHT) { // Right align
		gtk_label_set_xalign(GTK_LABEL(self->button_label), 1.0);
	} else {
		gtk_label_set_xalign(GTK_LABEL(self->button_label), 0);
	}

	gtk_label_set_justify(GTK_LABEL(self->button_label), j);
}

void koto_button_set_text_wrap(
	KotoButton * self,
	gboolean wrap
) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	if (!GTK_IS_LABEL(self->button_label)) { // If we do not have a button label
		return;
	}

	gtk_label_set_single_line_mode(GTK_LABEL(self->button_label), !wrap);
	gtk_label_set_wrap(GTK_LABEL(self->button_label), wrap);
}

void koto_button_show_image(
	KotoButton * self,
	gboolean use_alt
) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	if (self->use_from_file) { // Use from a file instead of icon name
		if (!koto_utils_string_is_valid(self->image_file_path)) { // Not set
			return;
		}

		if (GTK_IS_IMAGE(self->button_pic)) { // Already have an icon
			gtk_image_set_from_file(GTK_IMAGE(self->button_pic), self->image_file_path);
		} else { // Don't have an image yet
			self->button_pic = gtk_image_new_from_file(self->image_file_path); // Create a new image from the file
			gtk_box_prepend(GTK_BOX(self), self->button_pic); // Prepend to the box
		}
	} else { // From icon name
		if (use_alt && ((self->alt_icon_name == NULL) || (strcmp(self->alt_icon_name, "") == 0))) { // Don't have an alt icon set
			return;
		} else if (!use_alt && ((self->icon_name == NULL) || (strcmp(self->icon_name, "") == 0))) { // Don't have icon set
			return;
		}

		self->currently_showing_alt = use_alt;
		gchar * name = use_alt ? self->alt_icon_name : self->icon_name;

		if (GTK_IS_IMAGE(self->button_pic)) {
			gtk_image_set_from_icon_name(GTK_IMAGE(self->button_pic), name); // Just update the existing image
		} else { // Not an image
			self->button_pic = gtk_image_new_from_icon_name(name); // Get our new image
			gtk_box_prepend(GTK_BOX(self), self->button_pic); // Prepend to the box
		}
	}

	gtk_image_set_pixel_size(GTK_IMAGE(self->button_pic), self->pix_size);
	gtk_image_set_icon_size(GTK_IMAGE(self->button_pic), GTK_ICON_SIZE_INHERIT); // Inherit height of parent widget
	gtk_widget_show(self->button_pic); // Ensure we actually are showing the image
}

void koto_button_unflatten(KotoButton * self) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	gtk_widget_remove_css_class(GTK_WIDGET(self), "flat");
}

void koto_button_unset_pseudoactive_styling(KotoButton * self) {
	if (!KOTO_IS_BUTTON(self)) {
		return;
	}

	if (!gtk_widget_has_css_class(GTK_WIDGET(self), "pseudoactive")) { // Don't have the CSS class
		return;
	}

	gtk_widget_remove_css_class(GTK_WIDGET(self), "pseudoactive"); // Remove pseudoactive class
}

KotoButton * koto_button_new_plain(gchar * label) {
	return g_object_new(
		KOTO_TYPE_BUTTON,
		"button-text",
		label,
		NULL
	);
}

KotoButton * koto_button_new_with_icon(
	gchar * label,
	gchar * icon_name,
	gchar * alt_icon_name,
	KotoButtonPixbufSize size
) {
	return g_object_new(
		KOTO_TYPE_BUTTON,
		"button-text",
		label,
		"icon-name",
		icon_name,
		"alt-icon-name",
		alt_icon_name,
		"pixbuf-size",
		koto_get_pixbuf_size(size),
		NULL
	);
}

KotoButton * koto_button_new_with_file(
	gchar * label,
	gchar * file_path,
	KotoButtonPixbufSize size
) {
	return g_object_new(
		KOTO_TYPE_BUTTON,
		"button-text",
		label,
		"use-from-file",
		TRUE,
		"image-file-path",
		file_path,
		"pixbuf-size",
		koto_get_pixbuf_size(size),
		NULL
	);
}

KotoButton * koto_button_new_with_resource (
	gchar * resource_path,
	KotoButtonPixbufSize size
) {
	return g_object_new(
		KOTO_TYPE_BUTTON,
		"resource-path",
		resource_path,
		"pixbuf-size",
		koto_get_pixbuf_size(size),
		NULL
	);
}
