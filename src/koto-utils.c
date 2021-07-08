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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "koto-utils.h"

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
	gchar * trimmed_file_name = g_strdup(g_path_get_basename(filename)); // Ensure the filename provided is the base name of any possible path and duplicate it
	gchar ** split = g_strsplit(trimmed_file_name, ".", -1); // Split every time we see .

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

gchar * koto_utils_join_string_list (
	GList * list,
	gchar * sep
) {
	gchar * liststring = NULL;
	GList * cur_list;
	for (cur_list = list; cur_list != NULL; cur_list = cur_list->next) { // For each item in the list
		gchar * current_item = cur_list->data;
		if (!koto_utils_is_string_valid(current_item)) { // Not a valid string
			continue;
		}

		gchar * item_plus_sep = g_strdup_printf("%s%s", current_item, sep);

		if (koto_utils_is_string_valid(liststring)) { // Is a valid string
			gchar * new_string = g_strconcat(liststring, item_plus_sep, NULL);
			g_free(liststring);
			liststring = new_string;
		} else { // Don't have any content yet
			liststring = item_plus_sep;
		}
	}

	return (liststring == NULL) ? g_strdup("") : liststring;
}

gboolean koto_utils_is_string_valid(gchar * str) {
	return ((str != NULL) && (g_strcmp0(str, "") != 0));
}

void koto_utils_mkdir(gchar * path) {
	mkdir(path, 0755);
	chown(path, getuid(), getgid());
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
	gchar ** split = g_strsplit(str, find, -1); // Split on find

	guint split_len = g_strv_length(split);

	if (split_len == 1) { // Only one item
		g_strfreev(split);
		return g_strdup(str); // Just set to the string we were provided
	}

	return g_strdup(g_strjoinv(repl, split));
}

gboolean koto_utils_string_contains_substring(
	gchar * s,
	gchar * sub
) {
	gchar ** separated_string = g_strsplit(s, sub, -1); // Split on our substring
	gboolean contains = (g_strv_length(separated_string) > 1);
	g_strfreev(separated_string);
	return contains;
}

GList * koto_utils_string_to_string_list(
	gchar * s,
	gchar * sep
) {
	GList * list = NULL;
	gchar ** separated_strings = g_strsplit(s, sep, -1); // Split on separator for the string

	for (guint i = 0; i < g_strv_length(separated_strings); i++) { // Iterate over each item
		gchar * item = separated_strings[i];
		list = g_list_append(list, g_strdup(item));
	}

	g_strfreev(separated_strings); // Free our strings
	return list;
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
