/* main.c
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

#include <glib/gi18n.h>
#include <gstreamer-1.0/gst/gst.h>
#include "db/cartographer.h"
#include "db/db.h"
#include "playback/mimes.h"
#include "playback/mpris.h"

#include "koto-config.h"
#include "koto-window.h"

extern guint mpris_bus_id;
extern GDBusNodeInfo *introspection_data;

extern KotoCartographer *koto_maps;
extern sqlite3 *koto_db;

extern GHashTable *supported_mimes_hash;
extern GList *supported_mimes;

GtkApplication *app = NULL;
GtkWindow *main_window;

static void on_activate (GtkApplication *app) {
	g_assert(GTK_IS_APPLICATION (app));

	main_window = gtk_application_get_active_window (app);
	if (main_window == NULL) {
		main_window = g_object_new(KOTO_TYPE_WINDOW,  "application", app,  "default-width", 1200,  "default-height", 675, NULL);
	}

	gtk_window_present(main_window);
}

static void on_shutdown(GtkApplication *app) {
	(void) app;
	close_db(); // Close the database
	g_bus_unown_name(mpris_bus_id);
	g_dbus_node_info_unref(introspection_data);
}

int main (int argc, char *argv[]) {
	int ret;

	/* Set up gettext translations */
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gtk_init();
	gst_init(&argc, &argv);

	supported_mimes_hash = g_hash_table_new(g_str_hash, g_str_equal);
	supported_mimes = NULL; // Ensure our mimes GList is initialized
	koto_playback_engine_get_supported_mimetypes(supported_mimes);

	g_message("Length: %d", g_list_length(supported_mimes));

	koto_maps = koto_cartographer_new(); // Create our new cartographer and their collection of maps
	open_db(); // Open our database
	setup_mpris_interfaces(); // Set up our MPRIS interfaces

	GList *md;
	md = NULL;
	for (md = supported_mimes; md != NULL; md = md->next) {
		g_message("Mimetype: %s", (gchar*) md->data);
	}
	g_list_free(md);

	app = gtk_application_new ("com.github.joshstrobl.koto", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(on_shutdown), NULL);
	ret = g_application_run (G_APPLICATION (app), argc, argv);

	return ret;
}
