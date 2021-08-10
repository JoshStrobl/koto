/* library.c
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

#include "../config/config.h"
#include "../koto-utils.h"
#include "structs.h"

extern KotoConfig * config;
extern GVolumeMonitor * volume_monitor;

enum {
	PROP_0,
	PROP_TYPE,
	PROP_UUID,
	PROP_STORAGE_UUID,
	PROP_CONSTRUCTION_PATH,
	PROP_NAME,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};

enum {
	SIGNAL_NOW_AVAILABLE,
	SIGNAL_NOW_UNAVAILABLE,
	N_SIGNALS
};

static guint library_signals[N_SIGNALS] = {
	0
};

struct _KotoLibrary {
	GObject parent_instance;
	gchar * uuid;

	KotoLibraryType type;
	gchar * directory;
	gchar * storage_uuid;

	GMount * mount;
	gulong mount_unmounted_handler;
	gchar * mount_path;

	gboolean should_index;

	gchar * path;
	gchar * relative_path;
	gchar * name;
};

struct _KotoLibraryClass {
	GObjectClass parent_class;

	void (* now_available) (KotoLibrary * library);

	void (* now_unavailable) (KotoLibrary * library);
};

G_DEFINE_TYPE(KotoLibrary, koto_library, G_TYPE_OBJECT);

static void koto_library_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
);

static void koto_library_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_library_class_init(KotoLibraryClass * c) {
	GObjectClass * gobject_class;

	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_library_set_property;
	gobject_class->get_property = koto_library_get_property;

	library_signals[SIGNAL_NOW_AVAILABLE] = g_signal_new(
		"now-available",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoLibraryClass, now_available),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	library_signals[SIGNAL_NOW_UNAVAILABLE] = g_signal_new(
		"now-unavailable",
		G_TYPE_FROM_CLASS(gobject_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET(KotoLibraryClass, now_unavailable),
		NULL,
		NULL,
		NULL,
		G_TYPE_NONE,
		0
	);

	props[PROP_UUID] = g_param_spec_string(
		"uuid",
		"UUID of Library",
		"UUID of Library",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_TYPE] = g_param_spec_string(
		"type",
		"Type of Library",
		"Type of Library",
		koto_library_type_to_string(KOTO_LIBRARY_TYPE_MUSIC),
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_STORAGE_UUID] = g_param_spec_string(
		"storage-uuid",
		"Storage UUID to associated Mount of Library",
		"Storage UUID to associated Mount of Library",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_READWRITE
	);

	props[PROP_CONSTRUCTION_PATH] = g_param_spec_string(
		"construction-path",
		"Construction Path",
		"Path to this library during construction",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	props[PROP_NAME] = g_param_spec_string(
		"name",
		"Name of the Library",
		"Name of the Library",
		NULL,
		G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_library_init(KotoLibrary * self) {
	(void) self;
}

static void koto_library_get_property(
	GObject * obj,
	guint prop_id,
	GValue * val,
	GParamSpec * spec
) {
	KotoLibrary * self = KOTO_LIBRARY(obj);

	switch (prop_id) {
		case PROP_NAME:
			g_value_set_string(val, g_strdup(self->name));
			break;
		case PROP_UUID:
			g_value_set_string(val, self->uuid);
			break;
		case PROP_STORAGE_UUID:
			g_value_set_string(val, self->storage_uuid);
			break;
		case PROP_TYPE:
			g_value_set_string(val, koto_library_type_to_string(self->type));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

static void koto_library_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoLibrary * self = KOTO_LIBRARY(obj);

	switch (prop_id) {
		case PROP_UUID:
			self->uuid = g_strdup(g_value_get_string(val));
			break;
		case PROP_TYPE:
			self->type = koto_library_type_from_string(g_strdup(g_value_get_string(val)));
			break;
		case PROP_STORAGE_UUID:
			koto_library_set_storage_uuid(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_CONSTRUCTION_PATH:
			koto_library_set_path(self, g_strdup(g_value_get_string(val)));
			break;
		case PROP_NAME:
			koto_library_set_name(self, g_strdup(g_value_get_string(val)));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

gchar * koto_library_get_path(KotoLibrary * self) {
	if (!KOTO_IS_LIBRARY(self)) {
		return NULL;
	}

	if (G_IS_MOUNT(self->mount)) {

	}

	return self->path;
}

gchar * koto_library_get_relative_path_to_file(
	KotoLibrary * self,
	gchar * full_path
) {
	if (!KOTO_IS_LIBRARY(self)) {
		return NULL;
	}

	gchar * appended_slash_to_library_path = g_str_has_suffix(self->path, G_DIR_SEPARATOR_S) ? g_strdup(self->path) : g_strdup_printf("%s%s", g_strdup(self->path), G_DIR_SEPARATOR_S);
	gchar * cleaned_path = koto_utils_string_replace_all(full_path, appended_slash_to_library_path, ""); // Replace the full path

	g_free(appended_slash_to_library_path);
	return cleaned_path;
}

gchar * koto_library_get_storage_uuid(KotoLibrary * self) {
	if (!KOTO_IS_LIBRARY(self)) {
		return NULL;
	}

	return self->storage_uuid;
}

KotoLibraryType koto_library_get_lib_type(KotoLibrary * self) {
	if (!KOTO_IS_LIBRARY(self)) {
		return KOTO_LIBRARY_TYPE_UNKNOWN;
	}

	return self->type;
}

gchar * koto_library_get_uuid(KotoLibrary * self) {
	if (!KOTO_IS_LIBRARY(self)) {
		return NULL;
	}

	return self->uuid;
}

void koto_library_index(KotoLibrary * self) {
	if (!KOTO_IS_LIBRARY(self) || !self->should_index) { // Not a library or should not index
		return;
	}

	index_folder(self, self->path, 0); // Start index operation at the top
}

gboolean koto_library_is_available(KotoLibrary * self) {
	if (!KOTO_IS_LIBRARY(self)) {
		return FALSE;
	}

	return FALSE;
}

void koto_library_set_name(
	KotoLibrary * self,
	gchar * library_name
) {
	if (!KOTO_IS_LIBRARY(self)) {
		return;
	}

	if (!koto_utils_string_is_valid(library_name)) { // Not a string
		return;
	}

	if (koto_utils_string_is_valid(self->name)) { // Name already set
		g_free(self->name); // Free the existing value
	}

	self->name = g_strdup(library_name);
	g_object_notify_by_pspec(G_OBJECT(self), props[PROP_NAME]);
}

void koto_library_set_path(
	KotoLibrary * self,
	gchar * path
) {
	if (!KOTO_IS_LIBRARY(self)) {
		return;
	}

	if (!koto_utils_string_is_valid(path)) { // Not a valid string
		return;
	}

	if (koto_utils_string_is_valid(self->path)) {
		g_free(self->path);
	}

	self->relative_path = g_path_is_absolute(path) ? koto_utils_string_replace_all(path, self->mount_path, "") : path; // Ensure path is relative to our mount, even if the mount is really our own system partition
	self->path = g_build_path(G_DIR_SEPARATOR_S, self->mount_path, self->relative_path, NULL); // Ensure our path is to whatever the current path of the mount + relative path is
}

void koto_library_set_storage_uuid(
	KotoLibrary * self,
	gchar * storage_uuid
) {
	if (!KOTO_IS_LIBRARY(self)) {
		return;
	}

	if (G_IS_MOUNT(self->mount)) { // Already have a mount
		g_signal_handler_disconnect(self->mount, self->mount_unmounted_handler); // Stop listening to the unmounted signal for this existing mount
		g_object_unref(self->mount); // Dereference the mount
		g_free(self->mount_path);
	}

	if (!koto_utils_string_is_valid(storage_uuid)) { // Not a valid string, which actually is allowed for built-ins
		self->mount = NULL;
		self->mount_path = g_strdup_printf("%s%s", g_get_home_dir(), G_DIR_SEPARATOR_S); // Set mount path to user's home directory
		self->storage_uuid = NULL;
		return;
	}

	GMount * mount = g_volume_monitor_get_mount_for_uuid(volume_monitor, storage_uuid); // Attempt to get the mount by this UUID

	if (!G_IS_MOUNT(mount)) {
		g_warning("Failed to get mount for UUID: %s", storage_uuid);
		self->mount = NULL;
		return;
	}

	if (g_mount_is_shadowed(mount)) { // Is shadowed and should not use
		g_warning("This mount is considered \"shadowed\" and will not be used.");
		return;
	}

	GFile * mount_file = g_mount_get_default_location(mount); // Get the file for the entry location of the mount

	self->mount = mount;
	self->mount_path = g_strdup(g_file_get_path(mount_file)); // Set the mount path to the path defined for the Mount File
	self->storage_uuid = g_strdup(storage_uuid);
}

gchar * koto_library_to_config_string(KotoLibrary * self) {
	GStrvBuilder * lib_builder = g_strv_builder_new(); // Create  new strv builder
	g_strv_builder_add(lib_builder, g_strdup("[[library]]")); // Add our library array header

	g_strv_builder_add(lib_builder, g_strdup_printf("\tdirectory=\"%s\"", self->relative_path)); // Add the directory

	if (koto_utils_string_is_valid(self->name)) { // Have a library name
		g_strv_builder_add(lib_builder, g_strdup_printf("\tname=\"%s\"", self->name)); // Add the name
	}

	if (koto_utils_string_is_valid(self->storage_uuid)) { // Have a storage UUID (not applicable to built-ins)
		g_strv_builder_add(lib_builder, g_strdup_printf("\tstorage_uuid=\"%s\"", self->storage_uuid)); // Add the storage UUID
	}

	g_strv_builder_add(lib_builder, g_strdup_printf("\ttype=\"%s\"", koto_library_type_to_string(self->type))); // Add the type
	g_strv_builder_add(lib_builder, g_strdup_printf("\tuuid=\"%s\"", self->uuid));

	GStrv lines = g_strv_builder_end(lib_builder); // Get all the lines as a GStrv which is a gchar **
	gchar * content = g_strjoinv("\n", lines); // Separate all lines with newline
	g_strfreev(lines); // Free our lines

	g_strv_builder_unref(lib_builder); // Unref our builder
	return g_strdup(content);
}

KotoLibrary * koto_library_new(
	KotoLibraryType type,
	const gchar * storage_uuid,
	const gchar * path
) {
	KotoLibrary * lib = g_object_new(
		KOTO_TYPE_LIBRARY,
		"type",
		koto_library_type_to_string(type),
		"uuid",
		g_uuid_string_random(), // Create a new Library with a new UUID
		"storage-uuid",
		storage_uuid,
		"construction-path",
		path,
		NULL
	);

	lib->should_index = TRUE;
	return lib;
}

KotoLibrary * koto_library_new_from_toml_table(toml_table_t * lib_datum) {
	toml_datum_t uuid_datum = toml_string_in(lib_datum, "uuid"); // Get the library UUID

	if (!uuid_datum.ok) { // No UUID defined
		g_warning("No UUID set for this library. Ignoring");
		return NULL;
	}

	gchar * uuid = g_strdup(uuid_datum.u.s); // Duplicate our UUID

	toml_datum_t type_datum = toml_string_in(lib_datum, "type");

	if (!type_datum.ok) { // No type defined
		g_warning("Unknown type for library with UUID of %s", uuid);
		return NULL;
	}

	gchar * lib_type_as_str = g_strdup(type_datum.u.s);
	KotoLibraryType lib_type = koto_library_type_from_string(lib_type_as_str); // Get the library's type

	if (lib_type == KOTO_LIBRARY_TYPE_UNKNOWN) { // Known type
		return NULL;
	}

	toml_datum_t dir_datum = toml_string_in(lib_datum, "directory");

	if (!dir_datum.ok) {
		g_critical("Failed to get directory path for library with UUID of %s", uuid);
		return NULL;
	}

	gchar * path = g_strdup(dir_datum.u.s); // Duplicate the path string

	toml_datum_t storage_uuid_datum = toml_string_in(lib_datum, "storage_uuid"); // Get the datum for the storage UUID
	gchar * storage_uuid = g_strdup((storage_uuid_datum.ok) ? storage_uuid_datum.u.s : "");

	toml_datum_t name_datum = toml_string_in(lib_datum, "name"); // Get the datum for the name
	gchar * name = g_strdup((name_datum.ok) ? name_datum.u.s : "");

	KotoLibrary * lib = g_object_new(
		KOTO_TYPE_LIBRARY,
		"type",
		lib_type_as_str,
		"uuid",
		uuid,
		"storage-uuid",
		storage_uuid,
		"construction-path",
		path,
		"name",
		name,
		NULL
	);

	lib->should_index = FALSE;
	return lib;
}

KotoLibraryType koto_library_type_from_string(gchar * t) {
	if (
		(g_strcmp0(t, "audiobooks") == 0) ||
		(g_strcmp0(t, "audiobook") == 0)
	) {
		return KOTO_LIBRARY_TYPE_AUDIOBOOK;
	} else if (g_strcmp0(t, "music") == 0) {
		return KOTO_LIBRARY_TYPE_MUSIC;
	} else if (
		(g_strcmp0(t, "podcasts") == 0) ||
		(g_strcmp0(t, "podcast") == 0)
	) {
		return KOTO_LIBRARY_TYPE_PODCAST;
	}


	g_warning("Invalid type provided for koto_library_type_from_string: %s", t);
	return KOTO_LIBRARY_TYPE_UNKNOWN;
}

gchar * koto_library_type_to_string(KotoLibraryType t) {
	switch (t) {
		case KOTO_LIBRARY_TYPE_AUDIOBOOK:
			return g_strdup("audiobook");
		case KOTO_LIBRARY_TYPE_MUSIC:
			return g_strdup("music");
		case KOTO_LIBRARY_TYPE_PODCAST:
			return g_strdup("podcast");
		default:
			return g_strdup("UNKNOWN");
	}
}