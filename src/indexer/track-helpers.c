/* track-helpers.c
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
#include <taglib/tag_c.h>
#include "../db/cartographer.h"
#include  "../koto-track-item.h"
#include "../koto-utils.h"
#include "structs.h"

extern KotoCartographer * koto_maps;

GHashTable * genre_replacements;

void koto_track_helpers_init() {
	genre_replacements = g_hash_table_new(g_str_hash, g_str_equal);

	gchar * corrected_genre_hiphop = g_strdup("hip-hop");
	gchar * correct_genre_indie = g_strdup("indie");
	gchar * correct_genre_scifi = g_strdup("sci-fi");

	g_hash_table_insert(genre_replacements, g_strdup("alternative/indie"), correct_genre_indie); // Change Alternative/Indie (lowercased) to indie
	g_hash_table_insert(genre_replacements, g_strdup("rap-&-hip-hop"), corrected_genre_hiphop); // Change rap-&-hip-hop to just hip-hop
	g_hash_table_insert(genre_replacements, g_strdup("science-fiction"), correct_genre_scifi); // Change science-fiction to sci-fi
}

gchar * koto_track_helpers_get_corrected_genre(gchar * original_genre) {
	gchar * lookedup_genre = g_hash_table_lookup(genre_replacements, original_genre); // Look up the genre
	return koto_utils_is_string_valid(lookedup_genre) ? lookedup_genre : original_genre;
}

gchar * koto_track_helpers_get_name_for_file(
	const gchar * path,
	gchar * optional_artist_name
) {
	gchar * file_name = NULL;
	TagLib_File * t_file = taglib_file_new(path); // Get a taglib file for this file

	if ((t_file != NULL) && taglib_file_is_valid(t_file)) { // If we got the taglib file and it is valid
		TagLib_Tag * tag = taglib_file_tag(t_file); // Get our tag
		file_name = g_strdup(taglib_tag_title(tag)); // Get the tag title and duplicate it
	}

	taglib_tag_free_strings(); // Free strings
	taglib_file_free(t_file); // Free the file

	if (koto_utils_is_string_valid(file_name)) { // File name not set yet
		return file_name;
	}

	gchar * file_name_without_path = koto_utils_get_filename_without_extension((gchar*) path); // Get the "default" file name without the extension or path (strips all but basename)

	if (koto_utils_is_string_valid(optional_artist_name)) { // Was provided an optional artist name
		gchar * removed_artist_name = koto_utils_replace_string_all(file_name_without_path, optional_artist_name, ""); // Remove the artist
		g_free(file_name_without_path);
		file_name_without_path = removed_artist_name;
	}

	gchar ** split = g_regex_split_simple("^([\\d]+)", file_name_without_path, G_REGEX_JAVASCRIPT_COMPAT, 0); // Split based on any possible position
	if (g_strv_length(split) > 1) { // Has positional info at the beginning of the file name
		g_free(file_name_without_path); // Free the prior name
		file_name_without_path = g_strdup(split[2]); // Set to our second item which is the rest of the song name without the prefixed numbers
	}

	g_strfreev(split);
	split = g_strsplit(file_name_without_path, " - ", -1); // Split whenever we encounter " - "
	file_name_without_path = g_strjoinv("", split); // Remove entirely
	g_strfreev(split);

	split = g_strsplit(file_name_without_path, "-", -1); // Split whenever we encounter -
	file_name_without_path = g_strjoinv("", split); // Remove entirely
	g_strfreev(split);

	file_name = g_strdup(file_name_without_path); // Set file_name to the duplicate of the file name without the path, now all cleaned up
	g_free(file_name_without_path); // Free old reference
	return file_name;
}

guint64 koto_track_helpers_get_position_based_on_file_name(const gchar * file_name) {
	guint64 position = 0; // Default to position 0
	gchar ** split = g_regex_split_simple("^([\\d]+)", file_name, G_REGEX_JAVASCRIPT_COMPAT, 0);

	if (g_strv_length(split) > 1) { // Has positional info at the beginning of the file
		gchar * num = split[1];

		if ((strcmp(num, "0") != 0) && (strcmp(num, "00") != 0)) { // Is not zero
			guint64 potential_pos = g_ascii_strtoull(num, NULL, 10); // Attempt to convert

			if (potential_pos != 0) { // Got a legitimate position
				position = potential_pos; // Update position
			}
		}
	}

	return position;
}

gint koto_track_helpers_sort_tracks(
	gconstpointer track1,
	gconstpointer track2,
	gpointer user_data
) {
	(void) user_data;
	KotoTrack * track1_real = (KotoTrack*) track1;
	KotoTrack * track2_real = (KotoTrack*) track2;

	if (!KOTO_IS_TRACK(track1_real) && !KOTO_IS_TRACK(track2_real)) { // Neither tracks actually exist
		return 0;
	} else if (KOTO_IS_TRACK(track1_real) && !KOTO_IS_TRACK(track2_real)) { // Only track2 does not exist
		return -1;
	} else if (!KOTO_IS_TRACK(track1_real) && KOTO_IS_TRACK(track2_real)) { // Only track1 does not exist
		return 1;
	}

	guint track1_disc = koto_track_get_disc_number(track1_real);
	guint track2_disc = koto_track_get_disc_number(track2_real);

	if (track1_disc < track2_disc) { // Track 2 is in a later CD / Disc
		return -1;
	} else if (track1_disc > track2_disc) { // Track1 is later
		return 1;
	}

	guint track1_pos = koto_track_get_position(track1_real);
	guint track2_pos = koto_track_get_position(track2_real);

	if (track1_pos == track2_pos) { // Identical positions (like reported as 0)
		return g_utf8_collate(koto_track_get_name(track1_real), koto_track_get_name(track2_real));
	} else if (track1_pos < track2_pos) {
		return -1;
	} else {
		return 1;
	}
}

gint koto_track_helpers_sort_track_items(
	gconstpointer track1_item,
	gconstpointer track2_item,
	gpointer user_data
) {
	KotoTrack * track1 = koto_track_item_get_track((KotoTrackItem*) track1_item);
	KotoTrack * track2 = koto_track_item_get_track((KotoTrackItem*) track2_item);
	return koto_track_helpers_sort_tracks(track1, track2, user_data);
}

gint koto_track_helpers_sort_tracks_by_uuid(
	gconstpointer track1_uuid,
	gconstpointer track2_uuid,
	gpointer user_data
) {
	KotoTrack * track1 = koto_cartographer_get_track_by_uuid(koto_maps, (gchar*) track1_uuid);
	KotoTrack * track2 = koto_cartographer_get_track_by_uuid(koto_maps, (gchar*) track2_uuid);
	return koto_track_helpers_sort_tracks(track1, track2, user_data);
}