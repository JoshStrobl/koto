/* create-modify-dialog.c
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
#include <magic.h>
#include "../db/cartographer.h"
#include "../playlist/playlist.h"
#include "../koto-utils.h"
#include "../koto-window.h"
#include "create-modify-dialog.h"

extern KotoCartographer * koto_maps;
extern KotoWindow * main_window;

enum {
	PROP_DIALOG_0,
	PROP_PLAYLIST_UUID,
	N_PROPS
};

static GParamSpec * dialog_props[N_PROPS] = {
	NULL,
};


struct _KotoCreateModifyPlaylistDialog {
	GtkBox parent_instance;
	GtkWidget * playlist_image;
	GtkWidget * name_entry;

	GtkWidget * create_button;

	gchar * playlist_image_path;
	gchar * playlist_uuid;
};

G_DEFINE_TYPE(KotoCreateModifyPlaylistDialog, koto_create_modify_playlist_dialog, GTK_TYPE_BOX);

KotoCreateModifyPlaylistDialog * playlist_create_modify_dialog;

static void koto_create_modify_playlist_dialog_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_create_modify_playlist_dialog_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_create_modify_playlist_dialog_class_init(KotoCreateModifyPlaylistDialogClass * c) {
	GObjectClass * gobject_class;


	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_create_modify_playlist_dialog_set_property;
	gobject_class->get_property = koto_create_modify_playlist_dialog_get_property;

	dialog_props[PROP_PLAYLIST_UUID] = g_param_spec_string(
		"playlist-uuid",
		"Playlist UUID",
		"Playlist UUID",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPS, dialog_props);
}

static void koto_create_modify_playlist_dialog_init(KotoCreateModifyPlaylistDialog * self) {
	self->playlist_image_path = NULL;

	gtk_widget_set_halign(GTK_WIDGET(self), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(self), GTK_ALIGN_CENTER);

	self->playlist_image = gtk_image_new_from_icon_name("insert-image-symbolic");
	gtk_image_set_pixel_size(GTK_IMAGE(self->playlist_image), 220);
	gtk_widget_set_size_request(self->playlist_image, 220, 220);
	gtk_box_append(GTK_BOX(self), self->playlist_image); // Add our image

	GtkDropTarget * target = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);


	g_signal_connect(GTK_EVENT_CONTROLLER(target), "drop", G_CALLBACK(koto_create_modify_playlist_dialog_handle_drop), self);
	gtk_widget_add_controller(self->playlist_image, GTK_EVENT_CONTROLLER(target));

	GtkGesture * image_click_controller = gtk_gesture_click_new(); // Create a click gesture for the image clicking


	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(image_click_controller), 1); // Only allow left click
	g_signal_connect(GTK_EVENT_CONTROLLER(image_click_controller), "pressed", G_CALLBACK(koto_create_modify_playlist_dialog_handle_image_click), self);

	gtk_widget_add_controller(self->playlist_image, GTK_EVENT_CONTROLLER(image_click_controller));

	self->name_entry = gtk_entry_new(); // Create our text entry for the name of the playlist
	gtk_entry_set_placeholder_text(GTK_ENTRY(self->name_entry), "Name of playlist");
	gtk_entry_set_input_purpose(GTK_ENTRY(self->name_entry), GTK_INPUT_PURPOSE_NAME);
	gtk_entry_set_input_hints(GTK_ENTRY(self->name_entry), GTK_INPUT_HINT_SPELLCHECK & GTK_INPUT_HINT_NO_EMOJI & GTK_INPUT_HINT_PRIVATE);

	gtk_box_append(GTK_BOX(self), self->name_entry);

	self->create_button = gtk_button_new_with_label("Create");
	gtk_widget_add_css_class(self->create_button, "suggested-action");
	g_signal_connect(self->create_button, "clicked", G_CALLBACK(koto_create_modify_playlist_dialog_handle_create_click), self);
	gtk_box_append(GTK_BOX(self), self->create_button); // Add the create button
}

static void koto_create_modify_playlist_dialog_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoCreateModifyPlaylistDialog * self = KOTO_CREATE_MODIFY_PLAYLIST_DIALOG(obj);


	switch (prop_id) {
		case PROP_PLAYLIST_UUID:
			g_value_set_string(val, (self->playlist_uuid != NULL) ? g_strdup(self->playlist_uuid) : NULL);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_create_modify_playlist_dialog_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoCreateModifyPlaylistDialog * self = KOTO_CREATE_MODIFY_PLAYLIST_DIALOG(obj);


	(void) self;
	(void) val;

	switch (prop_id) {
		case PROP_PLAYLIST_UUID:
			// TODO: Implement
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_create_modify_playlist_dialog_handle_chooser_response(
	GtkNativeDialog * native,
	int response,
	gpointer user_data
) {
	if (response != GTK_RESPONSE_ACCEPT) { // Not accept
		g_object_unref(native);
		return;
	}

	KotoCreateModifyPlaylistDialog * self = user_data;


	if (!KOTO_IS_CURRENT_MODIFY_PLAYLIST(self)) {
		return;
	}

	GFile * file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(native));
	gchar * file_path = g_file_get_path(file); // Get the absolute path


	if (file_path != NULL) {
		self->playlist_image_path = g_strdup(file_path);
		gtk_image_set_from_file(GTK_IMAGE(self->playlist_image), self->playlist_image_path); // Set the file path
		g_free(file_path);
	}

	g_object_unref(file);
	g_object_unref(native);
}

void koto_create_modify_playlist_dialog_handle_create_click(
	GtkButton * button,
	gpointer user_data
) {
	(void) button;

	KotoCreateModifyPlaylistDialog * self = user_data;


	if (!KOTO_IS_CURRENT_MODIFY_PLAYLIST(self)) {
		return;
	}

	if (gtk_entry_get_text_length(GTK_ENTRY(self->name_entry)) == 0) { // No text
		gtk_widget_grab_focus(GTK_WIDGET(self->name_entry)); // Select the name entry
		return;
	}

	KotoPlaylist * playlist = NULL;
	gboolean modify_existing_playlist = koto_utils_is_string_valid(self->playlist_uuid);


	if (modify_existing_playlist) { // Modifying an existing playlist
		playlist = koto_cartographer_get_playlist_by_uuid(koto_maps, self->playlist_uuid);
	} else { // Creating a new playlist
		playlist = koto_playlist_new(); // Create a new playlist with a new UUID
	}

	if (!KOTO_IS_PLAYLIST(playlist)) { // If this isn't a playlist
		return;
	}

	koto_playlist_set_name(playlist, gtk_entry_buffer_get_text(gtk_entry_get_buffer(GTK_ENTRY(self->name_entry)))); // Set the name to the text we get from the entry buffer
	koto_playlist_set_artwork(playlist, self->playlist_image_path); // Add the playlist path if any
	koto_playlist_commit(playlist); // Save the playlist to the database

	if (!modify_existing_playlist) { // Not a new playlist
		koto_cartographer_add_playlist(koto_maps, playlist); // Add to cartographer
		koto_playlist_mark_as_finalized(playlist); // Ensure our tracks loaded finalized signal is emitted for the new playlist
	}

	koto_create_modify_playlist_dialog_reset(self);
	koto_window_hide_dialogs(main_window); // Hide the dialogs
}

gboolean koto_create_modify_playlist_dialog_handle_drop(
	GtkDropTarget * target,
	const GValue * val,
	double x,
	double y,
	gpointer user_data
) {
	(void) target;
	(void) x;
	(void) y;

	if (!G_VALUE_HOLDS(val, G_TYPE_FILE)) { // Not a file
		return FALSE;
	}

	KotoCreateModifyPlaylistDialog * self = user_data;


	if (!KOTO_IS_CURRENT_MODIFY_PLAYLIST(self)) { // No dialog
		return FALSE;
	}

	GFile * dropped_file = g_value_get_object(val); // Get the GValue
	gchar * file_path = g_file_get_path(dropped_file); // Get the absolute path


	g_object_unref(dropped_file); // Unref the file

	if (file_path == NULL) {
		return FALSE; // Failed to get the path so immediately return false
	}

	magic_t magic_cookie = magic_open(MAGIC_MIME);


	if (magic_cookie == NULL) {
		return FALSE;
	}

	if (magic_load(magic_cookie, NULL) != 0) {
		goto cookie_closure;
	}

	const char * mime_type = magic_file(magic_cookie, file_path);


	if ((mime_type != NULL) && g_str_has_prefix(mime_type, "image/")) { // Is an image
		self->playlist_image_path = g_strdup(file_path);
		gtk_image_set_from_file(GTK_IMAGE(self->playlist_image), self->playlist_image_path); // Set the file path
		g_free(file_path);
		return TRUE;
	}

cookie_closure:
	magic_close(magic_cookie);
	return FALSE;
}

void koto_create_modify_playlist_dialog_handle_image_click(
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

	KotoCreateModifyPlaylistDialog * self = user_data;

	GtkFileChooserNative* chooser = koto_utils_create_image_file_chooser("Choose playlist image");


	g_signal_connect(chooser, "response", G_CALLBACK(koto_create_modify_playlist_dialog_handle_chooser_response), self);
	gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser)); // Show our file chooser
}

void koto_create_modify_playlist_dialog_reset(KotoCreateModifyPlaylistDialog * self) {
	if (!KOTO_IS_CURRENT_MODIFY_PLAYLIST(self)) {
		return;
	}

	gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(self->name_entry)), "", -1); // Reset existing buffer to empty string
	gtk_entry_set_placeholder_text(GTK_ENTRY(self->name_entry), "Name of playlist"); // Reset placeholder
	gtk_image_set_from_icon_name(GTK_IMAGE(self->playlist_image), "insert-image-symbolic"); // Reset the image
	gtk_button_set_label(GTK_BUTTON(self->create_button), "Create");
}

void koto_create_modify_playlist_dialog_set_playlist_uuid(
	KotoCreateModifyPlaylistDialog * self,
	gchar * playlist_uuid
) {
	if (!koto_utils_is_string_valid(playlist_uuid)) { // Not a valid playlist UUID string
		return;
	}

	KotoPlaylist * playlist = koto_cartographer_get_playlist_by_uuid(koto_maps, playlist_uuid);


	if (!KOTO_IS_PLAYLIST(playlist)) {
		return;
	}

	self->playlist_uuid = g_strdup(playlist_uuid);
	gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(self->name_entry)), koto_playlist_get_name(playlist), -1); // Update the input buffer
	gtk_entry_set_placeholder_text(GTK_ENTRY(self->name_entry), ""); // Clear placeholder

	gchar * art = koto_playlist_get_artwork(playlist);


	if (!koto_utils_is_string_valid(art)) { // If art is not defined
		gtk_image_set_from_icon_name(GTK_IMAGE(self->playlist_image), "insert-image-symbolic"); // Reset the image
	} else {
		gtk_image_set_from_file(GTK_IMAGE(self->playlist_image), art);
		g_free(art);
	}

	gtk_button_set_label(GTK_BUTTON(self->create_button), "Save");
}

KotoCreateModifyPlaylistDialog * koto_create_modify_playlist_dialog_new(char * playlist_uuid) {
	(void) playlist_uuid;

	return g_object_new(KOTO_TYPE_CREATE_MODIFY_PLAYLIST_DIALOG, "orientation", GTK_ORIENTATION_VERTICAL, "spacing", 40, NULL);
}