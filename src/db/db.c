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
	char *tables_creation_queries = "CREATE TABLE IF NOT EXISTS artists(id string UNIQUE, path string UNIQUE, type int, name string, art_path string);"
		"CREATE TABLE IF NOT EXISTS albums(id string UNIQUE, path string UNIQUE, artist_id int, name string, art_path string);"
		"CREATE TABLE IF NOT EXISTS tracks(id string UNIQUE, path string UNIQUE, type int, artist_id int, album_id int, file_name string, name string, disc int, position int);";

	gchar *create_tables_errmsg = NULL;
	int rc = sqlite3_exec(koto_db, tables_creation_queries, 0,0, &create_tables_errmsg);

	if (rc != SQLITE_OK) {
		g_critical("Failed to create required tables: %s", create_tables_errmsg);
	}
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

	create_db_tables(); // Attempt to create our database tables
}