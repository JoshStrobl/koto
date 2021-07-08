/* file-indexer.c
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

#include <dirent.h>
#include <magic.h>
#include <stdio.h>
#include <sys/stat.h>
#include "../db/cartographer.h"
#include "../koto-utils.h"
#include "structs.h"
#include "track-helpers.h"

extern KotoCartographer * koto_maps;
extern magic_t magic_cookie;

void index_folder(
	KotoLibrary * self,
	gchar * path,
	guint depth
) {
	depth++;

	DIR * dir = opendir(path); // Attempt to open our directory

	if (dir == NULL) {
		return;
	}

	struct dirent * entry;

	while ((entry = readdir(dir))) {
		if (g_str_has_prefix(entry->d_name, ".")) { // A reference to parent dir, self, or a hidden item
			continue;
		}

		gchar * full_path = g_strdup_printf("%s%s%s", path, G_DIR_SEPARATOR_S, entry->d_name);

		if (entry->d_type == DT_DIR) { // Directory
			if (depth == 1) { // If we are following (ARTIST,AUTHOR,PODCAST)/ALBUM then this would be artist
				KotoArtist * artist = koto_artist_new(entry->d_name); // Attempt to get the artist

				if (KOTO_IS_ARTIST(artist)) {
					koto_artist_set_path(artist, self, full_path, TRUE); // Add the path for this library on this Artist and commit immediately
					koto_cartographer_add_artist(koto_maps, artist); // Add the artist to cartographer
					index_folder(self, full_path, depth); // Index this directory
					koto_artist_set_as_finalized(artist); // Indicate it is finalized
				}
			} else if (depth == 2) { // If we are following FOLDER/ARTIST/ALBUM then this would be album
				gchar * artist_name = g_path_get_basename(path); // Get the last entry from our path which is probably the artist
				KotoArtist * artist = koto_cartographer_get_artist_by_name(koto_maps, artist_name);

				if (!KOTO_IS_ARTIST(artist)) { // Not an artist
					continue;
				}

				gchar * artist_uuid = koto_artist_get_uuid(artist); // Get the artist's UUID

				KotoAlbum * album = koto_album_new(artist_uuid);

				koto_album_set_path(album, self, full_path);

				koto_cartographer_add_album(koto_maps, album); // Add our album to the cartographer
				koto_artist_add_album(artist, album); // Add the album

				index_folder(self, full_path, depth); // Index inside the album
				koto_album_commit(album); // Save to database immediately
				g_free(artist_name);
			} else if (depth == 3) { // Possibly CD within album
				gchar ** split = g_strsplit(full_path, G_DIR_SEPARATOR_S, -1);
				guint split_len = g_strv_length(split);

				if (split_len < 4) {
					g_strfreev(split);
					continue;
				}

				gchar * album_name = g_strdup(split[split_len - 2]);
				gchar * artist_name = g_strdup(split[split_len - 3]);
				g_strfreev(split);

				if (!koto_utils_is_string_valid(album_name)) {
					g_free(album_name);
					continue;
				}

				if (!koto_utils_is_string_valid(artist_name)) {
					g_free(album_name);
					g_free(artist_name);
					continue;
				}

				KotoArtist * artist = koto_cartographer_get_artist_by_name(koto_maps, artist_name);
				g_free(artist_name);

				if (!KOTO_IS_ARTIST(artist)) {
					continue;
				}

				KotoAlbum * album = koto_artist_get_album_by_name(artist, album_name); // Get the album
				g_free(album_name);

				if (!KOTO_IS_ALBUM(album)) {
					continue;
				}

				index_folder(self, full_path, depth); // Index inside the album
			}
		} else if ((entry->d_type == DT_REG)) { // Is a file in artist folder or lower in FS hierarchy
			index_file(self, full_path); // Index this audio file or weird ogg thing
		}

		g_free(full_path);
	}

	closedir(dir); // Close the directory
}

void index_file(
	KotoLibrary * lib,
	const gchar * path
) {
	const char * mime_type = magic_file(magic_cookie, path);

	if (mime_type == NULL) { // Failed to get the mimetype
		return;
	}

	if (!g_str_has_prefix(mime_type, "audio/") && !g_str_has_prefix(mime_type, "video/ogg")) { // Is not an audio file or ogg
		return;
	}

	gchar * relative_path_to_file = koto_library_get_relative_path_to_file(lib, g_strdup(path)); // Strip out library path so we have a relative path to the file
	gchar ** split_on_relative_slashes = g_strsplit(relative_path_to_file, G_DIR_SEPARATOR_S, -1); // Split based on separator (e.g. / )
	guint slash_sep_count = g_strv_length(split_on_relative_slashes);

	gchar * artist_author_podcast_name = g_strdup(split_on_relative_slashes[0]); // No matter what, artist should be first
	gchar * album_or_audiobook_name = NULL;
	gchar * file_name = koto_track_helpers_get_name_for_file(path, artist_author_podcast_name); // Get the name of the file
	guint cd = (guint) 1;

	if (slash_sep_count >= 3) { // If this it is at least "artist" + "album" + "file" (or with CD)
		album_or_audiobook_name = g_strdup(split_on_relative_slashes[1]); // Duplicate the second item as the album or audiobook name
	}

	// #region CD parsing logic

	if ((slash_sep_count == 4)) { // If is at least "artist" + "album" + "cd" + "file"
		gchar * cd_str = g_strdup(g_strstrip(koto_utils_replace_string_all(g_utf8_strdown(split_on_relative_slashes[2], -1), "cd", ""))); // Replace a lowercased version of our CD ("cd") and trim any whitespace

		cd = (guint) g_ascii_strtoull(cd_str, NULL, 10); // Attempt to convert

		if (cd == 0) { // Had an error during conversion
			cd = 1; // Set back to 1
		}
	}

	// #endregion

	g_strfreev(split_on_relative_slashes);

	gchar * sorta_uniqueish_key = NULL;

	if (koto_utils_is_string_valid(album_or_audiobook_name)) { // Have audiobook or album name
		sorta_uniqueish_key = g_strdup_printf("%s-%s-%s", artist_author_podcast_name, album_or_audiobook_name, file_name);
	} else { // No audiobook or album name
		sorta_uniqueish_key = g_strdup_printf("%s-%s", artist_author_podcast_name, file_name);
	}

	KotoTrack * track = koto_cartographer_get_track_by_uniqueish_key(koto_maps, sorta_uniqueish_key); // Attempt to get any existing KotoTrack

	if (KOTO_IS_TRACK(track)) { // Got a track already
		koto_track_set_path(track, lib, relative_path_to_file); // Add this path, which will determine the associated library within that function
	} else { // Don't already have a track for this file
		KotoArtist * artist = koto_cartographer_get_artist_by_name(koto_maps, artist_author_podcast_name); // Get the possible artist

		if (!KOTO_IS_ARTIST(artist)) { // Have an artist for this already
			return;
		}

		KotoAlbum * album = NULL;

		if (koto_utils_is_string_valid(album_or_audiobook_name)) { // Have an album or audiobook name
			KotoAlbum * possible_album = koto_artist_get_album_by_name(artist, album_or_audiobook_name);
			album = KOTO_IS_ALBUM(possible_album) ? possible_album : NULL;
		}

		gchar * album_uuid = KOTO_IS_ALBUM(album) ? koto_album_get_uuid(album) : NULL;

		track = koto_track_new(koto_artist_get_uuid(artist), album_uuid, file_name, cd);
		koto_track_set_path(track, lib, relative_path_to_file); // Immediately add the path to this file, for this Library
		koto_artist_add_track(artist, track); // Add the track to the artist in the event this is a podcast (no album) or the track is directly in the artist directory

		if (KOTO_IS_ALBUM(album)) { // Have an album
			koto_album_add_track(album, track); // Add this track since we haven't yet
		}

		koto_cartographer_add_track(koto_maps, track); // Add to our cartographer tracks hashtable
	}

	if (KOTO_IS_TRACK(track)) { // Is a track
		koto_track_commit(track); // Save the track immediately
	}
}