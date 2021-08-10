/* koto-utils.h
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
#include <glib-2.0/glib.h>
#include <gtk-4.0/gtk/gtk.h>

G_BEGIN_DECLS

GtkFileChooserNative * koto_utils_create_image_file_chooser(gchar * file_chooser_label);

GtkWidget * koto_utils_create_image_from_filepath(
	gchar * filepath,
	gchar * fallback_icon,
	guint width,
	guint height
);

gchar * koto_utils_gboolean_to_string(gboolean b);

gchar * koto_utils_get_filename_without_extension(gchar * filename);

gchar * koto_utils_join_string_list(
	GList * list,
	gchar * sep
);

void koto_utils_mkdir(gchar * path);

void koto_utils_push_queue_element_to_store(
	gpointer data,
	gpointer user_data
);

gchar * koto_utils_seconds_to_time_format(guint64 seconds);

gboolean koto_utils_string_contains_substring(
	gchar * s,
	gchar * sub
);

gchar * koto_utils_string_get_valid(gchar * str);

gboolean koto_utils_string_is_valid(const gchar * str);

gchar * koto_utils_string_replace_all(
	gchar * str,
	gchar * find,
	gchar * repl
);

GList * koto_utils_string_to_string_list(
	gchar * s,
	gchar * sep
);

gchar * koto_utils_string_title(const gchar * s);

gchar * koto_utils_string_unquote(gchar * s);

G_END_DECLS
