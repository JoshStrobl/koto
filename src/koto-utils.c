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

gchar* koto_utils_get_filename_without_extension(gchar *filename) {
	gchar *trimmed_file_name = g_strdup(filename);
	gchar **split = g_strsplit(filename, ".", -1); // Split every time we see .
	g_free(trimmed_file_name);
	guint len_of_extension_split = g_strv_length(split);

	if (len_of_extension_split == 2) { // Only have two elements
		trimmed_file_name = g_strdup(split[0]); // Get the first element
	} else {
		gchar *new_parsed_name = "";
		for (guint i = 0; i < len_of_extension_split - 1; i++) { // Iterate over everything except the last item
			if (g_strcmp0(new_parsed_name, "") == 0) { // Currently empty
				new_parsed_name = g_strdup(split[i]); // Just duplicate this string
			} else {
				gchar *tmp_copy = g_strdup(new_parsed_name);
				g_free(new_parsed_name); // Free the old
				new_parsed_name = g_strdup(g_strjoin(".", tmp_copy, split[i], NULL)); // Join the two strings with a . again and duplicate it, setting it to our new_parsed_name
				g_free(tmp_copy); // Free our temporary copy
			}
		}

		trimmed_file_name = g_strdup(new_parsed_name);
		g_free(new_parsed_name);
	}

	gchar *stripped_file_name = g_strstrip(trimmed_file_name); // Strip leading and trailing whitespace
	g_free(trimmed_file_name);
	return stripped_file_name;
}