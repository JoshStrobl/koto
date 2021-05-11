/* media-keys.h
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
#include <glib-2.0/gio/gio.h>
#include <gtk-4.0/gtk/gtk.h>

G_BEGIN_DECLS

void grab_media_keys();

void handle_media_keys_async_done(
	GObject * source_object,
	GAsyncResult * res,
	gpointer user_data
);

void handle_media_keys_signal(
	GDBusProxy * proxy,
	const gchar * sender_name,
	const gchar * signal_name,
	GVariant * parameters,
	gpointer user_data
);

void handle_window_enter(
	GtkEventControllerFocus * controller,
	gpointer user_data
);

void handle_window_leave(
	GtkEventControllerFocus * controller,
	gpointer user_data
);

void release_media_keys();

void setup_mediakeys_interface();

G_END_DECLS
