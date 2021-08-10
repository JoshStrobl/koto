/* album-info.c
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
#include "../config/config.h"
#include "../db/cartographer.h"
#include "../indexer/structs.h"
#include "../koto-utils.h"
#include "album-info.h"

extern KotoCartographer * koto_maps;
extern KotoConfig * config;

enum {
	PROP_0,
	PROP_TYPE,
	N_PROPERTIES
};

static GParamSpec * props[N_PROPERTIES] = {
	NULL,
};

struct _KotoAlbumInfo {
	GtkBox parent_instance;
	KotoAlbumInfoType type;

	KotoAlbum * album;

	GtkWidget * name_year_box;
	GtkWidget * genres_tags_list;

	GtkWidget * name_label;
	GtkWidget * year_badge;
	GtkWidget * narrator_label;

	GtkWidget * description_label;
};

struct _KotoAlbumInfoClass {
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(KotoAlbumInfo, koto_album_info, GTK_TYPE_BOX);

static void koto_album_info_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
);

static void koto_album_info_class_init(KotoAlbumInfoClass * c) {
	GObjectClass * gobject_class;
	gobject_class = G_OBJECT_CLASS(c);
	gobject_class->set_property = koto_album_info_set_property;

	props[PROP_TYPE] = g_param_spec_string(
		"type",
		"Type of AlbumInfo component",
		"Type of AlbumInfo component",
		"album",
		G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_WRITABLE
	);

	g_object_class_install_properties(gobject_class, N_PROPERTIES, props);
}

static void koto_album_info_init(KotoAlbumInfo * self) {
	gtk_widget_add_css_class(GTK_WIDGET(self), "album-info");
	self->type = KOTO_ALBUM_INFO_TYPE_ALBUM; // Default to Album here

	self->name_year_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); // Create out name box for the name and year
	gtk_widget_add_css_class(self->name_year_box, "album-title-year-combo");
	self->name_label = gtk_label_new(NULL); // Create our name label
	gtk_widget_set_halign(self->name_label, GTK_ALIGN_START);
	gtk_widget_set_valign(self->name_label, GTK_ALIGN_START);
	gtk_widget_add_css_class(self->name_label, "album-title");

	self->year_badge = gtk_label_new(NULL); // Create our year badge
	gtk_widget_add_css_class(self->year_badge, "album-year");
	gtk_widget_add_css_class(self->year_badge, "label-badge");
	gtk_widget_set_valign(self->year_badge, GTK_ALIGN_CENTER); // Center vertically align the year badge

	self->narrator_label = gtk_label_new(NULL); // Create our narrator label
	gtk_widget_add_css_class(self->narrator_label, "album-narrator");
	gtk_widget_set_halign(self->narrator_label, GTK_ALIGN_START);

	self->description_label = gtk_label_new(NULL); // Create our description label
	gtk_widget_add_css_class(self->description_label, "album-description");
	gtk_widget_set_halign(self->description_label, GTK_ALIGN_START);

	gtk_box_append(GTK_BOX(self->name_year_box), self->name_label); // Add the name label to the name + year box
	gtk_box_append(GTK_BOX(self->name_year_box), self->year_badge); // Add the year badge to the name + year box

	gtk_box_append(GTK_BOX(self), self->name_year_box);
	gtk_box_append(GTK_BOX(self), self->narrator_label);
	gtk_box_append(GTK_BOX(self), self->description_label);

	g_signal_connect(config, "notify::ui-info-show-description", G_CALLBACK(koto_album_info_apply_configuration_state), self);
	g_signal_connect(config, "notify::ui-info-show-genres", G_CALLBACK(koto_album_info_apply_configuration_state), self);
	g_signal_connect(config, "notify::ui-info-show-narrator", G_CALLBACK(koto_album_info_apply_configuration_state), self);
	g_signal_connect(config, "notify::ui-info-show-year", G_CALLBACK(koto_album_info_apply_configuration_state), self);
}

static void koto_album_info_set_property(
	GObject * obj,
	guint prop_id,
	const GValue * val,
	GParamSpec * spec
) {
	KotoAlbumInfo * self = KOTO_ALBUM_INFO(obj);

	switch (prop_id) {
		case PROP_TYPE:
			koto_album_info_set_type(self, g_value_get_string(val));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, spec);
			break;
	}
}

void koto_album_info_apply_configuration_state(
	KotoConfig * c,
	guint prop_id,
	KotoAlbumInfo * self
) {
	(void) c;
	(void) prop_id;

	gboolean show_description = TRUE;
	gboolean show_genres = TRUE;
	gboolean show_narrator = TRUE;
	gboolean show_year = TRUE;

	g_object_get(
		config,
		"ui-album-info-show-description",
		&show_description,
		"ui-album-info-show-genres",
		&show_genres,
		"ui-album-info-show-narrator",
		&show_narrator,
		"ui-album-info-show-year",
		&show_year,
		NULL
	);

	if (self->type != KOTO_ALBUM_INFO_TYPE_AUDIOBOOK) { // If the type is NOT an audiobook
		show_narrator = FALSE; // Narrator should never be shown. Only really applicable to audiobook
	}

	if (self->type == KOTO_ALBUM_INFO_TYPE_PODCAST) { // If the type is podcast
		show_year = FALSE; // Year isn't really applicable to podcast, at least not the "global" year
	}

	if (koto_utils_string_is_valid(koto_album_get_description(self->album)) && show_description) { // Have description to show in the first place, and should show it
		gtk_widget_show(self->description_label);
	} else { // Don't have content, just hide it
		gtk_widget_hide(self->description_label);
	}

	if (!KOTO_IS_ALBUM(self->album)) { // Don't have an album defined
		return;
	}

	if (GTK_IS_FLOW_BOX(self->genres_tags_list)) {
		if ((g_list_length(koto_album_get_genres(self->album)) > 0) && show_genres) { // Have genres to show in the first place, and should show it
			gtk_widget_show(self->genres_tags_list);
		} else {
			gtk_widget_hide(self->genres_tags_list);
		}
	}

	if (koto_utils_string_is_valid(koto_album_get_narrator(self->album)) && show_narrator) { // Have narrator and should show it
		gtk_widget_show(self->narrator_label);
	} else {
		gtk_widget_hide(self->narrator_label);
	}

	if ((koto_album_get_year(self->album) != 0) && show_year) { // Have year and should show it
		gtk_widget_show(self->year_badge);
	} else {
		gtk_widget_hide(self->year_badge);
	}
}

void koto_album_info_set_album_uuid(
	KotoAlbumInfo * self,
	gchar * album_uuid
) {
	if (!KOTO_IS_ALBUM_INFO(self)) {
		return;
	}

	if (!koto_utils_string_is_valid(album_uuid)) {
		return;
	}

	KotoAlbum * album = koto_cartographer_get_album_by_uuid(koto_maps, album_uuid); // Get the album

	if (!KOTO_IS_ALBUM(album)) { // Is not an album
		return;
	}

	self->album = album;

	gtk_label_set_text(GTK_LABEL(self->name_label), koto_album_get_name(self->album)); // Set the name label text to the album
	gtk_label_set_text(GTK_LABEL(self->year_badge), g_strdup_printf("%li", koto_album_get_year(self->album))); // Set the year label text to any year for the album

	gchar * narrator = koto_album_get_narrator(self->album);

	if (koto_utils_string_is_valid(narrator)) { // Have a narrator
		gtk_label_set_text(GTK_LABEL(self->narrator_label), g_strdup_printf("Narrated by %s", koto_album_get_narrator(self->album))); // Set narrated by string
	}

	gchar * description = koto_album_get_description(self->album);

	if (koto_utils_string_is_valid(description)) { // Have a description
		gtk_label_set_markup(GTK_LABEL(self->description_label), description); // Set the markup so we treat it more like HTML and let pango do its thing
	}

	if (GTK_IS_BOX(self->genres_tags_list)) { // Genres Flow
		gtk_box_remove(GTK_BOX(self), self->genres_tags_list); // Remove the genres flow from the box
	}

	self->genres_tags_list = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); // Create our flow box for the genres
	gtk_widget_add_css_class(self->genres_tags_list, "genres-tag-list");

	GList * album_genres = koto_album_get_genres(self->album);
	if (g_list_length(album_genres) != 0) { // Have genres
		GList * cur_list;
		for (cur_list = album_genres; cur_list != NULL; cur_list = cur_list->next) { // Iterate over each genre
			GtkWidget * genre_label = gtk_label_new(koto_utils_string_title(cur_list->data));
			gtk_widget_add_css_class(genre_label, "label-badge"); // Add label badge styling
			gtk_box_append(GTK_BOX(self->genres_tags_list), genre_label); // Append to the genre tags list
		}
	} else {
		gtk_widget_hide(self->genres_tags_list); // Hide the genres tag list
	}

	gtk_box_insert_child_after(GTK_BOX(self), self->genres_tags_list, self->name_year_box); // Add after the name+year flowbox

	koto_album_info_apply_configuration_state(NULL, 0, self); // Apply our configuration state immediately so we know what to show and hide for this album based on the config
}

void koto_album_info_set_type(
	KotoAlbumInfo * self,
	const gchar * type
) {
	if (!KOTO_IS_ALBUM_INFO(self)) {
		return;
	}

	if (g_strcmp0(type, "audiobook") == 0) { // If this is an audiobook
		self->type = KOTO_ALBUM_INFO_TYPE_AUDIOBOOK;
	} else if (g_strcmp0(type, "podcast") == 0) { // If this is a podcast
		self->type = KOTO_ALBUM_INFO_TYPE_PODCAST;
	} else {
		self->type = KOTO_ALBUM_INFO_TYPE_ALBUM;
	}
}

KotoAlbumInfo * koto_album_info_new(gchar * type) {
	return g_object_new(
		KOTO_TYPE_ALBUM_INFO,
		"orientation",
		GTK_ORIENTATION_VERTICAL,
		"type",
		type,
		NULL
	);
}