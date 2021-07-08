/* track-helpers.h
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

void koto_track_helpers_init();

gchar * koto_track_helpers_get_corrected_genre(gchar * original_genre);

gchar * koto_track_helpers_get_name_for_file(
	const gchar * path,
	gchar * optional_artist_name
);

guint64 koto_track_helpers_get_position_based_on_file_name(const gchar * file_name);

gint koto_track_helpers_sort_track_items(
	gconstpointer track1_item,
	gconstpointer track2_item,
	gpointer user_data
);

gint koto_track_helpers_sort_tracks(
	gconstpointer track1,
	gconstpointer track2,
	gpointer user_data
);

gint koto_track_helpers_sort_tracks_by_uuid(
	gconstpointer track1_uuid,
	gconstpointer track2_uuid,
	gpointer user_data
);