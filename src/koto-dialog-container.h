/* koto-dialog-container.h
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

G_BEGIN_DECLS

/**
 * Type Definition
 **/

#define KOTO_TYPE_DIALOG_CONTAINER koto_dialog_container_get_type()
G_DECLARE_FINAL_TYPE(KotoDialogContainer, koto_dialog_container, KOTO, DIALOG_CONTAINER, GtkBox);
#define KOTO_IS_DIALOG_CONTAINER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_DIALOG_CONTAINER))

/**
 * Functions
 **/

KotoDialogContainer * koto_dialog_container_new();

void koto_dialog_container_add_dialog(
	KotoDialogContainer * self,
	gchar * dialog_name,
	GtkWidget * dialog
);

void koto_dialog_container_handle_close_click(
	GtkGestureClick * gesture,
	int n_press,
	double x,
	double y,
	gpointer user_data
);

void koto_dialog_container_hide(KotoDialogContainer * self);

void koto_dialog_container_show_dialog(
	KotoDialogContainer * self,
	gchar * dialog_name
);

G_END_DECLS
