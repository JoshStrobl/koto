/* koto-dialog-container.c
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

#include <glib-2.0/glib-object.h>
#include <gtk-4.0/gtk/gtk.h>
#include "koto-button.h"
#include "koto-dialog-container.h"

struct _KotoDialogContainer {
	GtkBox parent_instance;
	KotoButton *close_button;
	GtkWidget *dialogs;
};

G_DEFINE_TYPE(KotoDialogContainer, koto_dialog_container, GTK_TYPE_BOX);

static void koto_dialog_container_class_init(KotoDialogContainerClass *c) {
	(void) c;
}

static void koto_dialog_container_init(KotoDialogContainer *self) {
	gtk_widget_add_css_class(GTK_WIDGET(self), "koto-dialog-container");

	g_object_set(GTK_WIDGET(self),
		"hexpand",
		TRUE,
		"vexpand",
		TRUE,
	NULL);

	self->close_button = koto_button_new_with_icon(NULL, "window-close-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_LARGE);
	gtk_widget_set_halign(GTK_WIDGET(self->close_button), GTK_ALIGN_END);
	gtk_box_prepend(GTK_BOX(self), GTK_WIDGET(self->close_button)); // Add our close button

	self->dialogs = gtk_stack_new();
	gtk_stack_set_transition_duration(GTK_STACK(self->dialogs), 0); // No transition timing
	gtk_stack_set_transition_type(GTK_STACK(self->dialogs), GTK_STACK_TRANSITION_TYPE_NONE); // No transition
	gtk_widget_set_halign(self->dialogs, GTK_ALIGN_CENTER);
	gtk_widget_set_hexpand(self->dialogs, TRUE);
	gtk_widget_set_valign(self->dialogs, GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(self->dialogs, TRUE);

	gtk_box_append(GTK_BOX(self), self->dialogs); // Add the dialogs stack

	koto_button_add_click_handler(self->close_button, KOTO_BUTTON_CLICK_TYPE_PRIMARY, G_CALLBACK(koto_dialog_container_handle_close_click), self);
	gtk_widget_hide(GTK_WIDGET(self)); // Hide by default
}

void koto_dialog_container_add_dialog(KotoDialogContainer *self, gchar *dialog_name, GtkWidget *dialog) {
	if (!KOTO_IS_DIALOG_CONTAINER(self)) { // Not a dialog container
		return;
	}

	gtk_stack_add_named(GTK_STACK(self->dialogs), dialog, dialog_name); // Add the dialog to the stack
}

void koto_dialog_container_handle_close_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {
	(void) gesture; (void) n_press; (void) x; (void) y;
	koto_dialog_container_hide((KotoDialogContainer*) user_data);
}

void koto_dialog_container_hide(KotoDialogContainer *self) {
	if (!KOTO_IS_DIALOG_CONTAINER(self)) { // Not a dialog container
		return;
	}

	gtk_widget_hide(GTK_WIDGET(self));
}

void koto_dialog_container_show_dialog(KotoDialogContainer *self, gchar *dialog_name) {
	if (!KOTO_IS_DIALOG_CONTAINER(self)) { // Not a dialog container
		return;
	}

	gtk_stack_set_visible_child_name(GTK_STACK(self->dialogs), dialog_name); // Set to the dialog name
	gtk_widget_show(GTK_WIDGET(self)); // Ensure we show self
}

KotoDialogContainer* koto_dialog_container_new() {
	return g_object_new(KOTO_TYPE_DIALOG_CONTAINER,
		"orientation",
		GTK_ORIENTATION_VERTICAL,
		NULL
	);
}
