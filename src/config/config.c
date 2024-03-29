/* config.c
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
#include <errno.h>
#include <toml.h>

#include "../db/cartographer.h"
#include "../playback/engine.h"
#include "../koto-paths.h"
#include "../koto-utils.h"

#include "config.h"

extern int errno;
extern const gchar * koto_config_template;
extern KotoCartographer * koto_maps;
extern KotoPlaybackEngine * playback_engine;

enum {
	PROP_0,
	PROP_PLAYBACK_CONTINUE_ON_PLAYLIST,
	PROP_PLAYBACK_LAST_USED_VOLUME,
	PROP_PLAYBACK_MAINTAIN_SHUFFLE,
	PROP_PLAYBACK_JUMP_BACKWARDS_INCREMENT,
	PROP_PLAYBACK_JUMP_FORWARDS_INCREMENT,
	PROP_PREFERRED_ALBUM_SORT_TYPE,
	PROP_UI_ALBUM_INFO_SHOW_DESCRIPTION,
	PROP_UI_ALBUM_INFO_SHOW_GENRES,
	PROP_UI_ALBUM_INFO_SHOW_NARRATOR,
	PROP_UI_ALBUM_INFO_SHOW_YEAR,
	PROP_UI_THEME_DESIRED,
	PROP_UI_THEME_OVERRIDE,
	N_PROPS,
};

static GParamSpec * config_props[N_PROPS] = {
	0
};

struct _KotoConfig {
	GObject parent_instance;
	toml_table_t * toml_ref;

	GFile * config_file;
	GFileMonitor * config_file_monitor;

	gchar * path;
	gboolean finalized;

	/* Library Attributes */

	// These are useful for when we need to determine separately if we need to index initial builtin folders that did not exist previously (during load)
	gboolean has_type_audiobook;
	gboolean has_type_music;
	gboolean has_type_podcast;

	/* Playback Settings */

	gboolean playback_continue_on_playlist;
	gdouble playback_last_used_volume;
	gboolean playback_maintain_shuffle;
	guint playback_jump_backwards_increment;
	guint playback_jump_forwards_increment;

	/* Misc Prefs */

	KotoPreferredAlbumSortType preferred_album_sort_type;

	/* UI Settings */

	gboolean ui_album_info_show_description;
	gboolean ui_album_info_show_genres;
	gboolean ui_album_info_show_narrator;
	gboolean ui_album_info_show_year;

	gchar * ui_theme_desired;
	gboolean ui_theme_override;
};

struct _KotoConfigClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE(KotoConfig, koto_config, G_TYPE_OBJECT);

KotoConfig * config;

static void koto_config_constructed(GObject * obj);

static void koto_config_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_config_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_config_class_init(KotoConfigClass * c) {
	GObjectClass * gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->constructed = koto_config_constructed;
	gobject_class->get_property = koto_config_get_property;
	gobject_class->set_property = koto_config_set_property;

	config_props[PROP_PLAYBACK_CONTINUE_ON_PLAYLIST] = g_param_spec_boolean(
		"playback-continue-on-playlist",
		"Continue Playback of Playlist",
		"Continue playback of a Playlist after playing a specific track in the playlist",
		FALSE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_PLAYBACK_LAST_USED_VOLUME] = g_param_spec_double(
		"playback-last-used-volume",
		"Last Used Volume",
		"Last Used Volume",
		0,
		1,
		0.5, // 50%
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_PLAYBACK_MAINTAIN_SHUFFLE] = g_param_spec_boolean(
		"playback-maintain-shuffle",
		"Maintain Shuffle on Playlist Change",
		"Maintain shuffle setting when changing playlists",
		TRUE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_PLAYBACK_JUMP_BACKWARDS_INCREMENT] = g_param_spec_uint(
		"playback-jump-backwards-increment",
		"Jump Backwards Increment",
		"Jump Backwards Increment",
		5, // 5s
		90, // 1min30s
		10, // 10s
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_PLAYBACK_JUMP_FORWARDS_INCREMENT] = g_param_spec_uint(
		"playback-jump-forwards-increment",
		"Jump Forwards Increment",
		"Jump Forwards Increment",
		5, // 5s
		90, // 1min30s
		30, // 30s
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_PREFERRED_ALBUM_SORT_TYPE] = g_param_spec_string(
		"artist-preferred-album-sort-type",
		"Preferred album sort type (chronological or alphabetical-only)",
		"Preferred album sort type (chronological or alphabetical-only)",
		"default",
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_UI_ALBUM_INFO_SHOW_DESCRIPTION] = g_param_spec_boolean(
		"ui-album-info-show-description",
		"Show Description in Album Info",
		"Show Description in Album Info",
		TRUE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_UI_ALBUM_INFO_SHOW_GENRES] = g_param_spec_boolean(
		"ui-album-info-show-genres",
		"Show Genres in Album Info",
		"Show Genres in Album Info",
		TRUE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_UI_ALBUM_INFO_SHOW_NARRATOR] = g_param_spec_boolean(
		"ui-album-info-show-narrator",
		"Show Narrator in Album Info",
		"Show Narrator in Album Info",
		TRUE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_UI_ALBUM_INFO_SHOW_YEAR] = g_param_spec_boolean(
		"ui-album-info-show-year",
		"Show Year in Album Info",
		"Show Year in Album Info",
		TRUE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_UI_THEME_DESIRED] = g_param_spec_string(
		"ui-theme-desired",
		"Desired Theme",
		"Desired Theme",
		"dark", // Like my soul
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	config_props[PROP_UI_THEME_OVERRIDE] = g_param_spec_boolean(
		"ui-theme-override",
		"Override built-in theming",
		"Override built-in theming",
		FALSE,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	g_object_class_install_properties(gobject_class, N_PROPS, config_props);
}

static void koto_config_init(KotoConfig * self) {
	self->finalized = FALSE;
	self->has_type_audiobook = FALSE;
	self->has_type_music = FALSE;
	self->has_type_podcast = FALSE;
}

static void koto_config_constructed(GObject * obj) {
	KotoConfig * self = KOTO_CONFIG(obj);
	self->finalized = TRUE;
}

static void koto_config_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoConfig * self = KOTO_CONFIG(obj);

	switch (prop_id) {
		case PROP_PLAYBACK_CONTINUE_ON_PLAYLIST:
			g_value_set_boolean(val, self->playback_continue_on_playlist);
			break;
		case PROP_PLAYBACK_LAST_USED_VOLUME:
			g_value_set_double(val, self->playback_last_used_volume);
			break;
		case PROP_PLAYBACK_MAINTAIN_SHUFFLE:
			g_value_set_boolean(val, self->playback_maintain_shuffle);
			break;
		case PROP_PLAYBACK_JUMP_BACKWARDS_INCREMENT:
			g_value_set_uint(val, self->playback_jump_backwards_increment);
			break;
		case PROP_PLAYBACK_JUMP_FORWARDS_INCREMENT:
			g_value_set_uint(val, self->playback_jump_forwards_increment);
			break;
		case PROP_UI_ALBUM_INFO_SHOW_DESCRIPTION:
			g_value_set_boolean(val, self->ui_album_info_show_description);
			break;
		case PROP_UI_ALBUM_INFO_SHOW_GENRES:
			g_value_set_boolean(val, self->ui_album_info_show_genres);
			break;
		case PROP_UI_ALBUM_INFO_SHOW_NARRATOR:
			g_value_set_boolean(val, self->ui_album_info_show_narrator);
			break;
		case PROP_UI_ALBUM_INFO_SHOW_YEAR:
			g_value_set_boolean(val, self->ui_album_info_show_year);
			break;
		case PROP_UI_THEME_DESIRED:
			g_value_set_string(val, g_strdup(self->ui_theme_desired));
			break;
		case PROP_UI_THEME_OVERRIDE:
			g_value_set_boolean(val, self->ui_theme_override);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_config_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoConfig * self = KOTO_CONFIG(obj);

	switch (prop_id) {
		case PROP_PLAYBACK_CONTINUE_ON_PLAYLIST:
			self->playback_continue_on_playlist = g_value_get_boolean(val);
			break;
		case PROP_PLAYBACK_LAST_USED_VOLUME:
			self->playback_last_used_volume = g_value_get_double(val);
			break;
		case PROP_PLAYBACK_MAINTAIN_SHUFFLE:
			self->playback_maintain_shuffle = g_value_get_boolean(val);
			break;
		case PROP_PLAYBACK_JUMP_BACKWARDS_INCREMENT:
			self->playback_jump_backwards_increment = g_value_get_uint(val);
			break;
		case PROP_PLAYBACK_JUMP_FORWARDS_INCREMENT:
			self->playback_jump_forwards_increment = g_value_get_uint(val);
			break;
		case PROP_PREFERRED_ALBUM_SORT_TYPE:
			self->preferred_album_sort_type = g_strcmp0(g_value_get_string(val), "alphabetical") ? KOTO_PREFERRED_ALBUM_ALWAYS_ALPHABETICAL : KOTO_PREFERRED_ALBUM_SORT_TYPE_DEFAULT;
			break;
		case PROP_UI_ALBUM_INFO_SHOW_DESCRIPTION:
			self->ui_album_info_show_description = g_value_get_boolean(val);
			break;
		case PROP_UI_ALBUM_INFO_SHOW_GENRES:
			self->ui_album_info_show_genres = g_value_get_boolean(val);
			break;
		case PROP_UI_ALBUM_INFO_SHOW_NARRATOR:
			self->ui_album_info_show_narrator = g_value_get_boolean(val);
			break;
		case PROP_UI_ALBUM_INFO_SHOW_YEAR:
			self->ui_album_info_show_year = g_value_get_boolean(val);
			break;
		case PROP_UI_THEME_DESIRED:
			self->ui_theme_desired = g_strdup(g_value_get_string(val));
			break;
		case PROP_UI_THEME_OVERRIDE:
			self->ui_theme_override = g_value_get_boolean(val);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}

	if (self->finalized) { // Loaded the config
		g_object_notify_by_pspec(obj, config_props[prop_id]); // Notify that a change happened
	}
}

toml_table_t * koto_config_get_library(
	KotoConfig * self,
	gchar * library_uuid
) {
	toml_array_t * libraries = toml_array_in(self->toml_ref, "library"); // Get the array of tables
	for (int i = 0; i < toml_array_nelem(libraries); i++) { // For each library
		toml_table_t * library = toml_table_at(libraries, i); // Get this library
		toml_datum_t uuid_datum = toml_string_in(library, "uuid"); // Get the datum for the uuid

		gchar * lib_uuid = (uuid_datum.ok) ? (gchar*) uuid_datum.u.s : NULL;

		if (koto_utils_string_is_valid(lib_uuid) && (g_strcmp0(library_uuid, lib_uuid) == 0)) { // Is a valid string and the libraries match
			return library;
		}
	}

	return NULL;
}

/**
 * Load our TOML file from the specified path into our KotoConfig
 **/
void koto_config_load(
	KotoConfig * self,
	gchar * path
) {
	if (!koto_utils_string_is_valid(path)) { // Path is not valid
		return;
	}

	self->path = g_strdup(path);

	self->config_file = g_file_new_for_path(path);
	gboolean config_file_exists = g_file_query_exists(self->config_file, NULL);

	if (!config_file_exists) { // File does not exist
		GError * create_err;
		GFileOutputStream * stream = g_file_create(
			self->config_file,
			G_FILE_CREATE_PRIVATE,
			NULL,
			&create_err
		);

		if (create_err != NULL) {
			if (create_err->code != G_IO_ERROR_EXISTS) { // Not an error indicating the file already exists
				g_warning("Failed to create or open file: %s", create_err->message);
				return;
			}
		}

		g_object_unref(stream);
	}

	GError * file_info_query_err;

	GFileInfo * file_info = g_file_query_info(
		// Get the size of our TOML file
		self->config_file,
		G_FILE_ATTRIBUTE_STANDARD_SIZE,
		G_FILE_QUERY_INFO_NONE,
		NULL,
		&file_info_query_err
	);

	if (file_info != NULL) { // Got info
		goffset size = g_file_info_get_size(file_info); // Get the size from info
		g_object_unref(file_info); // Unref immediately

		if (size == 0) { // If we don't have any file contents (new file), skip parsing
			goto monitor;
		}
	} else { // Failed to get the info
		g_warning("Failed to get size info of %s: %s", self->path, file_info_query_err->message);
	}

	FILE * file;
	file = fopen(self->path, "r"); // Open the file as read only

	if (file == NULL) { // Failed to get the file
		/** Handle error checking here*/
		return;
	}

	char errbuf[200];
	toml_table_t * conf = toml_parse_file(file, errbuf, sizeof(errbuf));
	fclose(file); // Close the file

	if (!conf) {
		g_error("Failed to read our config file. %s", errbuf);
		return;
	}

	self->toml_ref = conf;

	/** Supplemental Libraries (Excludes Built-in) */

	toml_array_t * libraries = toml_array_in(conf, "library"); // Get all of our libraries
	if (libraries) { // If we have libraries already
		for (int i = 0; i < toml_array_nelem(libraries); i++) { // Iterate over each library
			toml_table_t * lib = toml_table_at(libraries, i); // Get the library datum
			KotoLibrary * koto_lib = koto_library_new_from_toml_table(lib); // Get the library based on the TOML data for this specific type

			if (!KOTO_IS_LIBRARY(koto_lib)) { // Something wrong with it, not a library
				continue;
			}

			koto_cartographer_add_library(koto_maps, koto_lib); // Add library to Cartographer
			KotoLibraryType lib_type = koto_library_get_lib_type(koto_lib); // Get the type

			if (lib_type == KOTO_LIBRARY_TYPE_AUDIOBOOK) { // Is an audiobook lib
				self->has_type_audiobook = TRUE;
			} else if (lib_type == KOTO_LIBRARY_TYPE_MUSIC) { // Is a music lib
				self->has_type_music = TRUE;
			} else if (lib_type == KOTO_LIBRARY_TYPE_PODCAST) { // Is a podcast lib
				self->has_type_podcast = TRUE;
			}
		}
	}

	/** Playback Section */

	toml_table_t * playback_section = toml_table_in(conf, "playback");

	if (playback_section) { // Have playback section
		toml_datum_t continue_on_playlist = toml_bool_in(playback_section, "continue-on-playlist");
		toml_datum_t jump_backwards_increment = toml_int_in(playback_section, "jump-backwards-increment");
		toml_datum_t jump_forwards_increment = toml_int_in(playback_section, "jump-forwards-increment");
		toml_datum_t last_used_volume = toml_double_in(playback_section, "last-used-volume");
		toml_datum_t maintain_shuffle = toml_bool_in(playback_section, "maintain-shuffle");

		if (continue_on_playlist.ok && (self->playback_continue_on_playlist != continue_on_playlist.u.b)) { // If we have a continue-on-playlist set and they are different
			g_object_set(self, "playback-continue-on-playlist", continue_on_playlist.u.b, NULL);
		}

		if (jump_backwards_increment.ok && (self->playback_jump_backwards_increment != jump_backwards_increment.u.i)) { // If we have a jump-backwards-increment set and it is different
			g_object_set(self, "playback-jump-backwards-increment", (guint) jump_backwards_increment.u.i, NULL);
		}

		if (jump_forwards_increment.ok && (self->playback_jump_forwards_increment != jump_forwards_increment.u.i)) { // If we have a jump-backwards-increment set and it is different
			g_object_set(self, "playback-jump-forwards-increment", (guint) jump_forwards_increment.u.i, NULL);
		}

		if (last_used_volume.ok && (self->playback_last_used_volume != last_used_volume.u.d)) { // If we have last-used-volume set and they are different
			g_object_set(self, "playback-last-used-volume", last_used_volume.u.d, NULL);
		}

		if (maintain_shuffle.ok && (self->playback_maintain_shuffle != maintain_shuffle.u.b)) { // If we have a "maintain shuffle set" and they are different
			g_object_set(self, "playback-maintain-shuffle", maintain_shuffle.u.b, NULL);
		}
	}

	/* UI Section */

	toml_table_t * ui_section = toml_table_in(conf, "ui");

	if (ui_section) { // Have UI section
		toml_datum_t album_info_show_description = toml_bool_in(ui_section, "album-info-show-description");

		if (album_info_show_description.ok && (album_info_show_description.u.b != self->ui_album_info_show_description)) { // Changed if we are disabling description
			g_object_set(self, "ui-album-info-show-description", album_info_show_description.u.b, NULL);
		}

		toml_datum_t album_info_show_genres = toml_bool_in(ui_section, "album-info-show-genres");

		if (album_info_show_genres.ok && (album_info_show_genres.u.b != self->ui_album_info_show_genres)) { // Changed if we are disabling description
			g_object_set(self, "ui-album-info-show-genres", album_info_show_genres.u.b, NULL);
		}

		toml_datum_t album_info_show_narrator = toml_bool_in(ui_section, "album-info-show-narrator");

		if (album_info_show_narrator.ok && (album_info_show_narrator.u.b != self->ui_album_info_show_narrator)) { // Changed if we are disabling description
			g_object_set(self, "ui-album-info-show-narrator", album_info_show_description.u.b, NULL);
		}

		toml_datum_t album_info_show_year = toml_bool_in(ui_section, "album-info-show-year");

		if (album_info_show_year.ok && (album_info_show_year.u.b != self->ui_album_info_show_year)) { // Changed if we are disabling description
			g_object_set(self, "ui-album-info-show-year", album_info_show_year.u.b, NULL);
		}

		toml_datum_t name = toml_string_in(ui_section, "theme-desired");

		if (name.ok && (g_strcmp0(name.u.s, self->ui_theme_desired) != 0)) { // Have a name specified and they are different
			g_object_set(self, "ui-theme-desired", g_strdup(name.u.s), NULL);
			free(name.u.s);
		}

		toml_datum_t override_app = toml_bool_in(ui_section, "theme-override");

		if (override_app.ok && (override_app.u.b != self->ui_theme_override)) { // Changed if we are overriding theme
			g_object_set(self, "ui-theme-override", override_app.u.b, NULL);
		}
	}

monitor:
	if (self->config_file_monitor != NULL) { // If we already have a file monitor for the file
		return;
	}

	self->config_file_monitor = g_file_monitor_file(
		self->config_file,
		G_FILE_MONITOR_NONE,
		NULL,
		NULL
	);

	g_signal_connect(self->config_file_monitor, "changed", G_CALLBACK(koto_config_monitor_handle_changed), self); // Monitor changes to our config file

	if (!config_file_exists) { // File did not originally exist
		koto_config_save(self); // Save immediately
	}
}

void koto_config_load_libs(KotoConfig * self) {
	gchar * home_dir = g_strdup(g_get_home_dir()); // Get the home directory

	if (!self->has_type_audiobook) { // If we do not have a KotoLibrary for Audiobooks
		gchar * audiobooks_path = g_build_path(G_DIR_SEPARATOR_S, home_dir, "Audiobooks", NULL);
		koto_utils_mkdir(audiobooks_path); // Create the directory just in case
		KotoLibrary * lib = koto_library_new(KOTO_LIBRARY_TYPE_AUDIOBOOK, NULL, audiobooks_path); // Audiobooks relative to home directory

		if (KOTO_IS_LIBRARY(lib)) { // Created built-in audiobooks lib successfully
			koto_cartographer_add_library(koto_maps, lib);
			koto_config_save(config);
			koto_library_index(lib); // Index this library
		}

		g_free(audiobooks_path);
	}

	if (!self->has_type_music) { // If we do not have a KotoLibrary for Music
		KotoLibrary * lib = koto_library_new(KOTO_LIBRARY_TYPE_MUSIC, NULL, g_get_user_special_dir(G_USER_DIRECTORY_MUSIC)); // Create a library using the user's MUSIC directory defined

		if (KOTO_IS_LIBRARY(lib)) { // Created built-in music lib successfully
			koto_cartographer_add_library(koto_maps, lib);
			koto_config_save(config);
			koto_library_index(lib); // Index this library
		}
	}

	if (!self->has_type_podcast) { // If we do not have a KotoLibrary for Podcasts
		gchar * podcasts_path = g_build_path(G_DIR_SEPARATOR_S, home_dir, "Podcasts", NULL);
		koto_utils_mkdir(podcasts_path); // Create the directory just in case
		KotoLibrary * lib = koto_library_new(KOTO_LIBRARY_TYPE_PODCAST, NULL, podcasts_path); // Podcasts relative to home dir

		if (KOTO_IS_LIBRARY(lib)) { // Created built-in podcasts lib successfully
			koto_cartographer_add_library(koto_maps, lib);
			koto_config_save(config);
			koto_library_index(lib); // Index this library
		}

		g_free(podcasts_path);
	}

	g_free(home_dir);
	g_thread_exit(0);
}

void koto_config_monitor_handle_changed(
	GFileMonitor * monitor,
	GFile * file,
	GFile * other_file,
	GFileMonitorEvent ev,
	gpointer user_data
) {
	(void) monitor;
	(void) file;
	(void) other_file;
	KotoConfig * config = user_data;

	if (
		(ev == G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED) || // Attributes changed
		(ev == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) // Changes done
	) {
		koto_config_refresh(config); // Refresh the config
	}
}

KotoPreferredAlbumSortType koto_config_get_preferred_album_sort_type(KotoConfig * self) {
	return self->preferred_album_sort_type;
}

/**
 * Refresh will handle any FS notify change on our Koto config file and call load
 **/
void koto_config_refresh(KotoConfig * self) {
	koto_config_load(self, self->path);
}

/**
 * Save will write our config back out
 **/
void koto_config_save(KotoConfig * self) {
	GStrvBuilder * root_builder = g_strv_builder_new(); // Create a new strv builder

	/* Iterate over our libraries */

	GList * libs = koto_cartographer_get_libraries(koto_maps); // Get our libraries
	GList * current_libs;

	for (current_libs = libs; current_libs != NULL; current_libs = current_libs->next) { // Iterate over our libraries
		KotoLibrary * lib = current_libs->data;
		gchar * lib_config = koto_library_to_config_string(lib); // Get the config string
		g_strv_builder_add(root_builder, lib_config); // Add the config to our string builder
		g_free(lib_config);
	}

	GParamSpec ** props_list = g_object_class_list_properties(G_OBJECT_GET_CLASS(self), NULL); // Get the propreties associated with our settings

	GHashTable * sections_to_prop_keys = g_hash_table_new(g_str_hash, g_str_equal); // Create our section to hold our various sections based on props

	/* Section Hashes*/

	gchar * playback_hash = g_strdup("playback");
	gchar * ui_hash = g_strdup("ui");

	gdouble current_playback_volume = 1.0;

	if (KOTO_IS_PLAYBACK_ENGINE(playback_engine)) { // Have a playback engine (useful since it may not be initialized before the config performs saving on first application load)
		current_playback_volume = koto_playback_engine_get_volume(playback_engine); // Get the last used volume in the playback engine
	}

	self->playback_last_used_volume = current_playback_volume; // Update our value so we have it during save

	int i;
	for (i = 0; i < N_PROPS; i++) { // For each property
		GParamSpec * spec = props_list[i]; // Get the prop

		if (!G_IS_PARAM_SPEC(spec)) { // Not a spec
			continue; // Skip
		}

		const gchar * prop_name = g_param_spec_get_name(spec);

		gpointer respective_prop = NULL;

		if (g_str_has_prefix(prop_name, "playback")) { // Is playback
			respective_prop = playback_hash;
		} else if (g_str_has_prefix(prop_name, "ui")) { // Is UI
			respective_prop = ui_hash;
		}

		if (respective_prop == NULL) { // No property
			continue;
		}

		GList * keys;

		if (g_hash_table_contains(sections_to_prop_keys, respective_prop)) { // Already has list
			keys = g_hash_table_lookup(sections_to_prop_keys, respective_prop); // Get the list
		} else { // Don't have list
			keys = NULL;
		}

		keys = g_list_append(keys, g_strdup(prop_name)); // Add the name in full
		g_hash_table_insert(sections_to_prop_keys, respective_prop, keys); // Replace list (or add it)
	}

	GHashTableIter iter;
	gpointer section_name, section_props;

	g_hash_table_iter_init(&iter, sections_to_prop_keys);

	while (g_hash_table_iter_next(&iter, &section_name, &section_props)) {
		GStrvBuilder * section_builder = g_strv_builder_new(); // Make our string builder
		g_strv_builder_add(section_builder, g_strdup_printf("[%s]", (gchar*) section_name)); // Add section as [section]

		GList * current_section_keyname;
		for (current_section_keyname = section_props; current_section_keyname != NULL; current_section_keyname = current_section_keyname->next) { // Iterate over property names
			GValue prop_val_raw = G_VALUE_INIT; // Initialize our GValue
			g_object_get_property(G_OBJECT(self), current_section_keyname->data, &prop_val_raw);
			gchar * prop_val = g_strdup_value_contents(&prop_val_raw);

			if ((g_strcmp0(prop_val, "TRUE") == 0) || (g_strcmp0(prop_val, "FALSE") == 0)) { // TRUE or FALSE from a boolean type
				prop_val = g_utf8_strdown(prop_val, -1); // Change it to be lowercased
			}

			gchar * key_name = g_strdup(current_section_keyname->data);
			gchar * key_name_replaced = koto_utils_string_replace_all(key_name, g_strdup_printf("%s-", (gchar*) section_name), ""); // Remove SECTIONNAME-

			const gchar * line = g_strdup_printf("\t%s = %s", key_name_replaced, prop_val);

			g_strv_builder_add(section_builder, line); // Add the line
			g_free(key_name_replaced);
			g_free(key_name);
		}

		GStrv lines = g_strv_builder_end(section_builder); // Get all the lines as a GStrv which is a gchar **
		gchar * content = g_strjoinv("\n", lines); // Separate all lines with newline
		g_strfreev(lines); // Free our lines

		g_strv_builder_add(root_builder, content); // Add section content to root builder
		g_strv_builder_unref(section_builder); // Unref our builder
	}

	g_hash_table_unref(sections_to_prop_keys); // Free our hash table

	GStrv lines = g_strv_builder_end(root_builder); // Get all the lines as a GStrv which is a gchar **
	gchar * content = g_strjoinv("\n", lines); // Separate all lines with newline
	g_strfreev(lines); // Free our lines

	g_strv_builder_unref(root_builder); // Unref our root builder

	ulong file_content_length = g_utf8_strlen(content, -1);

	g_file_replace_contents(
		self->config_file,
		content,
		file_content_length,
		NULL,
		FALSE,
		G_FILE_CREATE_PRIVATE,
		NULL,
		NULL,
		NULL
	);
}

KotoConfig * koto_config_new() {
	return g_object_new(KOTO_TYPE_CONFIG, NULL);
}