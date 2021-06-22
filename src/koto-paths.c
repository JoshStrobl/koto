/* koto-paths.c
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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "koto-paths.h"
#include "koto-utils.h"

gchar * koto_rev_dns;
gchar * koto_path_cache;
gchar * koto_path_config;

gchar * koto_path_to_conf;
gchar * koto_path_to_db;

void koto_paths_setup() {
	koto_rev_dns = "com.github.joshstrobl.koto";
	gchar * user_cache_dir = g_strdup(g_get_user_data_dir());
	gchar * user_config_dir = g_strdup(g_get_user_config_dir());

	koto_path_cache = g_build_path(G_DIR_SEPARATOR_S, user_cache_dir, koto_rev_dns, NULL);
	koto_path_config = g_build_path(G_DIR_SEPARATOR_S, user_config_dir, koto_rev_dns, NULL);
	koto_path_to_conf = g_build_filename(koto_path_config, "config.toml", NULL);
	koto_path_to_db = g_build_filename( koto_path_cache, "db", NULL);

	koto_utils_mkdir(user_cache_dir);
	koto_utils_mkdir(user_config_dir);
	koto_utils_mkdir(koto_path_cache);
	koto_utils_mkdir(koto_path_config);
}