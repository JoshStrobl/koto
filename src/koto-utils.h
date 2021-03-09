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

GtkWidget* koto_utils_create_image_from_filepath(gchar *filepath, gchar *fallback_icon, guint width, guint height);
gchar* koto_utils_get_filename_without_extension(gchar *filename);
gchar* koto_utils_unquote_string(gchar *s);

G_END_DECLS
