/* koto-utils.c
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

extern GtkWindow * main_window;

GtkFileChooserNative * koto_utils_create_image_file_chooser(gchar * file_chooser_label) {
	GtkFileChooserNative * chooser = gtk_file_chooser_native_new(
		file_chooser_label,
		main_window,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		"Choose",
		"Cancel"
	);

	GtkFileFilter * image_filter = gtk_file_filter_new(); // Create our file filter

	gtk_file_filter_add_mime_type(image_filter, "image/*"); // Only allow for images
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(chooser), image_filter); // Only allow picking images
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), FALSE);

	return chooser;
}

GtkWidget * koto_utils_create_image_from_filepath(
	gchar * filepath,
	gchar * fallback_icon,
	guint width,
	guint height
) {
	GtkWidget * image = NULL;

	if ((filepath != NULL) && (strcmp(filepath, "") != 0)) { // If we have a filepath
		if (g_file_test(filepath, G_FILE_TEST_EXISTS)) { // File exists
			image = gtk_image_new_from_file(filepath); // Load from the filepath
		}
	}

	if (!GTK_IS_IMAGE(image)) { // If we failed to get the image or never passed a valid filepath to begin with
		image = gtk_image_new_from_icon_name(fallback_icon); // Set to the fallback icon
	}

	gtk_image_set_icon_size(GTK_IMAGE(image), GTK_ICON_SIZE_INHERIT);
	gtk_image_set_pixel_size(GTK_IMAGE(image), width);
	gtk_widget_set_size_request(image, width, height);

	return image;
}

gchar * koto_utils_gboolean_to_string(gboolean b) {
	return g_strdup(b ? "true" : "false");
}

gchar * koto_utils_get_filename_without_extension(gchar * filename) {
	gchar * trimmed_file_name = g_strdup(filename);
	gchar ** split = g_strsplit(filename, ".", -1); // Split every time we see .

	g_free(trimmed_file_name);
	guint len_of_extension_split = g_strv_length(split);

	if (len_of_extension_split == 2) { // Only have two elements
		trimmed_file_name = g_strdup(split[0]); // Get the first element
	} else {
		gchar * new_parsed_name = "";
		for (guint i = 0; i < len_of_extension_split - 1; i++) { // Iterate over everything except the last item
			if (g_strcmp0(new_parsed_name, "") == 0) { // Currently empty
				new_parsed_name = g_strdup(split[i]); // Just duplicate this string
			} else {
				gchar * tmp_copy = g_strdup(new_parsed_name);
				g_free(new_parsed_name); // Free the old
				new_parsed_name = g_strjoin(".", tmp_copy, split[i], NULL); // Join the two strings with a . again and duplicate it, setting it to our new_parsed_name
				g_free(tmp_copy); // Free our temporary copy
			}
		}

		trimmed_file_name = g_strdup(new_parsed_name);
		g_free(new_parsed_name);
	}

	gchar * stripped_file_name = g_strstrip(g_strdup(trimmed_file_name)); // Strip leading and trailing whitespace

	g_free(trimmed_file_name);
	return stripped_file_name;
}

gboolean koto_utils_is_string_valid(gchar * str) {
	return ((str != NULL) && (g_strcmp0(str, "") != 0));
}

void koto_utils_push_queue_element_to_store(
	gpointer data,
	gpointer user_data
) {
	g_list_store_append(G_LIST_STORE(user_data), data);
}

gchar * koto_utils_replace_string_all(
	gchar * str,
	gchar * find,
	gchar * repl
) {
	gchar * cleaned_string = "";
	gchar ** split = g_strsplit(str, find, -1); // Split on find

	for (guint i = 0; i < g_strv_length(split); i++) { // For each split
		cleaned_string = g_strjoin(repl, cleaned_string, split[i], NULL); // Join the strings with our replace string
	}

	g_strfreev(split);
	return cleaned_string;
}

gchar * koto_utils_unquote_string(gchar * s) {
	gchar * new_s = NULL;

	if (g_str_has_prefix(s, "'") && g_str_has_suffix(s, "'")) { // Begins and ends with '
		new_s = g_utf8_substring(s, 1, g_utf8_strlen(s, -1) - 1); // Start at 1 and end at n-1
	} else {
		new_s = g_strdup(s);
	}

	gchar ** split_on_double_single = g_strsplit(new_s, "''", -1); // Split on instances of ''

	new_s = g_strjoinv("'", split_on_double_single); // Rejoin as '
	g_strfreev(split_on_double_single); // Free our array

	return new_s;
}
