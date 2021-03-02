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

sqlite3 *koto_db = NULL;

void close_db() {
	sqlite3_close(koto_db);
}

void create_db_tables() {
	char *tables_creation_queries = "CREATE TABLE IF NOT EXISTS artists(id string UNIQUE PRIMARY KEY, path string, type int, name string, art_path string);"
		"CREATE TABLE IF NOT EXISTS albums(id string UNIQUE PRIMARY KEY, path string, artist_id string, name string, art_path string, FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE);"
		"CREATE TABLE IF NOT EXISTS tracks(id string UNIQUE PRIMARY KEY, path string, type int, artist_id string, album_id string, file_name string, name string, disc int, position int, FOREIGN KEY(artist_id) REFERENCES artists(id) ON DELETE CASCADE);";

	gchar *create_tables_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, tables_creation_queries, 0,0, &create_tables_errmsg);

	if (rc != SQLITE_OK) {
		g_critical("Failed to create required tables: %s", create_tables_errmsg);
	}
}

void enable_foreign_keys() {
	gchar *enable_foreign_keys_err = NULL;
	int rc = sqlite3_exec(koto_db, "PRAGMA foreign_keys = ON;", 0, 0, &enable_foreign_keys_err);

	if (rc != SQLITE_OK) {
		g_critical("Failed to enable foreign key support. Ensure your sqlite3 is compiled with neither SQLITE_OMIT_FOREIGN_KEY or SQLITE_OMIT_TRIGGER defined: %s", enable_foreign_keys_err);
	}

	g_free(enable_foreign_keys_err);
}

void open_db() {
	const gchar *data_home = g_get_user_data_dir();
	gchar *data_dir = g_build_path(G_DIR_SEPARATOR_S, data_home, "com.github.joshstrobl.koto", NULL);
	mkdir(data_home, 0755);
	mkdir(data_dir, 0755);
	chown(data_dir, getuid(), getgid());

	gchar *db_path = g_build_filename(data_dir, "db", NULL); // Build out our path using XDG_DATA_HOME (e.g. .local/share/) + our namespace + db as the file name
	int rc = sqlite3_open(db_path, &koto_db);

	if (rc) {
		g_critical("Failed to open or create database: %s", sqlite3_errmsg(koto_db));
		return;
	}

	enable_foreign_keys(); // Enable FOREIGN KEY support

	create_db_tables(); // Attempt to create our database tables
}
