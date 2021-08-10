/* db.c
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
#include <sqlite3.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "db.h"
#include "../koto-paths.h"

extern gchar * koto_path_to_db;

int KOTO_DB_SUCCESS = 0;
int KOTO_DB_NEW = 1;
int KOTO_DB_FAIL = 2;

sqlite3 * koto_db = NULL;
gchar * db_filepath = NULL;
gboolean created_new_db = FALSE;

void close_db() {
	sqlite3_close(koto_db);
}

int create_db_tables() {
	gchar * tables_creation_queries = "CREATE TABLE IF NOT EXISTS artists(id string UNIQUE PRIMARY KEY, name string, art_path string);"
									  "CREATE TABLE IF NOT EXISTS albums(id string UNIQUE PRIMARY KEY, artist_id string, name string, description string, narrator string, art_path string, genres string, year int, FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE);"
									  "CREATE TABLE IF NOT EXISTS tracks(id string UNIQUE PRIMARY KEY, artist_id string, album_id string, name string, disc int, position int, duration int, genres string, FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE);"
									  "CREATE TABLE IF NOT EXISTS libraries_albums(id string, album_id string, path string, PRIMARY KEY (id, album_id) FOREIGN KEY(album_id) REFERENCES albums(id) ON DELETE CASCADE);"
									  "CREATE TABLE IF NOT EXISTS libraries_artists(id string, artist_id string, path string, PRIMARY KEY(id, artist_id) FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE);"
									  "CREATE TABLE IF NOT EXISTS libraries_tracks(id string, track_id string, path string, PRIMARY KEY(id, track_id) FOREIGN KEY(track_id) REFERENCES tracks(id) ON DELETE CASCADE);"
									  "CREATE TABLE IF NOT EXISTS playlist_meta(id string UNIQUE PRIMARY KEY, name string, art_path string, preferred_model int, album_id string, track_id string, playback_position_of_track int);"
									  "CREATE TABLE IF NOT EXISTS playlist_tracks(position INTEGER PRIMARY KEY AUTOINCREMENT, playlist_id string, track_id string, FOREIGN KEY(playlist_id) REFERENCES playlist_meta(id), FOREIGN KEY(track_id) REFERENCES tracks(id) ON DELETE CASCADE);";

	return (new_transaction(tables_creation_queries, "Failed to create required tables", TRUE) == SQLITE_OK) ? KOTO_DB_SUCCESS : KOTO_DB_FAIL;
}

int enable_foreign_keys() {
	gchar * commit_op = g_strdup("PRAGMA foreign_keys = ON;");
	const gchar * transaction_err_msg = "Failed to enable foreign key support. Ensure your sqlite3 is compiled with neither SQLITE_OMIT_FOREIGN_KEY or SQLITE_OMIT_TRIGGER defined";

	return (new_transaction(commit_op, transaction_err_msg, FALSE) == SQLITE_OK) ? KOTO_DB_SUCCESS : KOTO_DB_FAIL;
}

int have_existing_db() {
	struct stat db_stat;
	int success = stat(koto_path_to_db, &db_stat);
	return ((success == 0) && S_ISREG(db_stat.st_mode)) ? 0 : 1;
}

int new_transaction(
	gchar * operation,
	const gchar * transaction_err_msg,
	gboolean fatal
) {
	gchar * commit_op_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, operation, 0, 0, &commit_op_errmsg);

	if (rc != SQLITE_OK) {
		(fatal) ? g_critical("%s: %s", transaction_err_msg, commit_op_errmsg) : g_warning("%s: %s", transaction_err_msg, commit_op_errmsg);
	}

	if (commit_op_errmsg == NULL) {
		g_free(commit_op_errmsg);
	}

	return rc;
}

int open_db() {
	int ret = KOTO_DB_SUCCESS; // Default to last return being SUCCESS

	if (have_existing_db() == 1) { // If we do not have an existing DB
		ret = KOTO_DB_NEW;
	}

	if (sqlite3_open(koto_path_to_db, &koto_db) != KOTO_DB_SUCCESS) { // If we failed to open the database file
		g_critical("Failed to open or create database: %s", sqlite3_errmsg(koto_db));
		return KOTO_DB_FAIL;
	}

	if (enable_foreign_keys() != KOTO_DB_SUCCESS) { // If we failed to enable foreign keys
		return KOTO_DB_FAIL;
	}

	if (create_db_tables() != KOTO_DB_SUCCESS) { // Failed to create our database tables
		return KOTO_DB_FAIL;
	}

	if (ret == KOTO_DB_NEW) {
		created_new_db = TRUE;
	}

	return ret;
}
