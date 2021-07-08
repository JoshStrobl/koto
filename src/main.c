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
#include <glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <magic.h>
#include <taglib/tag_c.h>
#include "config/config.h"
#include "db/cartographer.h"
#include "db/db.h"
#include "db/loaders.h"
#include "indexer/track-helpers.h"
#include "playback/engine.h"
#include "playback/media-keys.h"
#include "playback/mimes.h"
#include "playback/mpris.h"
#include "paths.h"

#include "config/config.h"
#include "koto-paths.h"
#include "koto-window.h"

extern KotoConfig * config;

extern guint mpris_bus_id;
extern GDBusNodeInfo * introspection_data;

extern KotoPlaybackEngine * playback_engine;
extern KotoCartographer * koto_maps;
extern sqlite3 * koto_db;

extern GHashTable * supported_mimes_hash;
extern GList * supported_mimes;

extern gchar * koto_path_to_conf;
extern gchar * koto_rev_dns;

extern gboolean created_new_db;

GVolumeMonitor * volume_monitor = NULL;
GtkApplication * app = NULL;
GtkWindow * main_window;
magic_t magic_cookie;

static void on_activate (GtkApplication * app) {
	g_assert(GTK_IS_APPLICATION(app));

	main_window = gtk_application_get_active_window(app);
	if (main_window == NULL) {
		main_window = g_object_new(KOTO_TYPE_WINDOW, "application", app, "default-width", 1200, "default-height", 675, NULL);
		setup_mpris_interfaces(); // Set up our MPRIS interfaces
		setup_mediakeys_interface(); // Set up our media key support

		if (!created_new_db) {
			read_from_db(); // Read the database, allowing us to propagate the UI with various data such as artists and playlist navigation elements
		}
	}

	gtk_window_present(main_window);
}

static void on_shutdown(GtkApplication * app) {
	(void) app;
	koto_config_save(config); // Save our config
	close_db(); // Close the database
	g_bus_unown_name(mpris_bus_id);
	g_dbus_node_info_unref(introspection_data);
}

int main (
	int argc,
	char * argv[]
) {
	int ret;

	gtk_init();
	gst_init(&argc, &argv);

	koto_track_helpers_init(); // Init our track helpers (primarily our genre replacement hashtable)
	koto_paths_setup(); // Set up our required paths

	supported_mimes_hash = g_hash_table_new(g_str_hash, g_str_equal);
	supported_mimes = NULL; // Ensure our mimes GList is initialized
	koto_playback_engine_get_supported_mimetypes(supported_mimes);

	koto_maps = koto_cartographer_new(); // Create our new cartographer and their collection of maps

	volume_monitor = g_volume_monitor_get(); // Get a VolumeMonitor

	config = koto_config_new(); // Set our config
	koto_config_load(config, koto_path_to_conf);
	playback_engine = koto_playback_engine_new(); // Initialize the engine now that the config is available, since it listens on various config signals

	taglib_id3v2_set_default_text_encoding(TagLib_ID3v2_UTF8); // Ensure our id3v2 text encoding is UTF-8
	magic_cookie = magic_open(MAGIC_MIME);

	if (magic_cookie == NULL) { // Failed to open
		g_critical("Failed to allocate a cookie pointer from libmagic.");
	}

	if (magic_load(magic_cookie, NULL) != 0) { // Failed to load data
		magic_close(magic_cookie);
		g_critical("Failed to load the system magic database.");
	}

	open_db(); // Open our database
	g_thread_new("indexing-any-necessary-libs", (void*) koto_config_load_libs, config); // Load our libraries, now that our database is set up. Note that read_from_db is called in koto-window.c

	app = gtk_application_new(koto_rev_dns, G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(on_shutdown), NULL);
	ret = g_application_run(G_APPLICATION(app), argc, argv);

	return ret;
}
