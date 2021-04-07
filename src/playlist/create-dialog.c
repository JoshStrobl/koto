/* create-dialog.h
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
#include "../db/cartographer.h"
#include "../db/db.h"
#include "create-dialog.h"

extern GtkWindow *main_window;

struct _KotoCreatePlaylistDialog {
	GObject parent_instance;
	GtkWidget *content;
	GtkWidget *playlist_image;
	GtkFileChooserNative *playlist_file_chooser;
	GtkWidget *name_entry;
};

G_DEFINE_TYPE(KotoCreatePlaylistDialog, koto_create_playlist_dialog, G_TYPE_OBJECT);

static void koto_create_playlist_dialog_class_init(KotoCreatePlaylistDialogClass *c) {
	(void) c;
}

static void koto_create_playlist_dialog_init(KotoCreatePlaylistDialog *self) {
	self->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign(self->content, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(self->content, GTK_ALIGN_CENTER);

	GtkIconTheme *default_icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default()); // Get the icon theme for this display

	if (default_icon_theme != NULL) {
		gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(self->content));
		GtkIconPaintable* audio_paintable = gtk_icon_theme_lookup_icon(default_icon_theme, "insert-image-symbolic", NULL, 96, scale_factor, GTK_TEXT_DIR_NONE, GTK_ICON_LOOKUP_PRELOAD);

		if (GTK_IS_ICON_PAINTABLE(audio_paintable)) {
			self->playlist_image = gtk_image_new_from_paintable(GDK_PAINTABLE(audio_paintable));
			gtk_widget_set_size_request(self->playlist_image, 96, 96);
			gtk_box_append(GTK_BOX(self->content), self->playlist_image); // Add our image to our content

			GtkGesture *image_click_controller = gtk_gesture_click_new(); // Create a click gesture for the image clicking
			gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(image_click_controller), 1); // Only allow left click
			g_signal_connect(GTK_EVENT_CONTROLLER(image_click_controller), "pressed", G_CALLBACK(koto_create_playlist_dialog_handle_image_click), self);

			gtk_widget_add_controller(self->playlist_image, GTK_EVENT_CONTROLLER(image_click_controller));
		}
	}

	self->playlist_file_chooser =  gtk_file_chooser_native_new(
		"Choose playlist image",
		main_window,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		"Choose",
		"Cancel"
	);

	GtkFileFilter *image_filter = gtk_file_filter_new(); // Create our file filter
	gtk_file_filter_add_pattern(image_filter, "image/*"); // Only allow for images
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(self->playlist_file_chooser), image_filter); // Only allow picking images
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(self->playlist_file_chooser), FALSE);

	self->name_entry = gtk_entry_new(); // Create our text entry for the name of the playlist
	gtk_entry_set_placeholder_text(GTK_ENTRY(self->name_entry), "Name of playlist");
	gtk_entry_set_input_purpose(GTK_ENTRY(self->name_entry), GTK_INPUT_PURPOSE_NAME);
	gtk_entry_set_input_hints(GTK_ENTRY(self->name_entry), GTK_INPUT_HINT_SPELLCHECK & GTK_INPUT_HINT_NO_EMOJI & GTK_INPUT_HINT_PRIVATE);

	gtk_box_append(GTK_BOX(self->content), self->name_entry);
}

GtkWidget* koto_create_playlist_dialog_get_content(KotoCreatePlaylistDialog *self) {
	return self->content;
}

void koto_create_playlist_dialog_handle_image_click(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {
	(void) gesture; (void) n_press; (void) x; (void) y;

	KotoCreatePlaylistDialog *self = user_data;

	gtk_native_dialog_show(GTK_NATIVE_DIALOG(self->playlist_file_chooser)); // Show our file chooser
}

KotoCreatePlaylistDialog* koto_create_playlist_dialog_new() {
	return g_object_new(KOTO_TYPE_CREATE_PLAYLIST_DIALOG, NULL);
}
