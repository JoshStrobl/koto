/* db.h
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

extern int KOTO_DB_SUCCESS;
extern int KOTO_DB_NEW;
extern int KOTO_DB_FAIL;
extern gboolean created_new_db;

void close_db();

int create_db_tables();

gchar * get_db_path();

int enable_foreign_keys();

int have_existing_db();

int open_db();
