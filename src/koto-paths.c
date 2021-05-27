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

gchar * koto_rev_dns;
gchar * koto_path_cache;
gchar * koto_path_config;

gchar * koto_path_to_conf;
gchar * koto_path_to_db;

void koto_paths_setup() {
	koto_rev_dns = "com.github.joshstrobl.koto";
	const gchar *user_cache_dir = g_get_user_data_dir();
	const gchar * user_config_dir = g_get_user_config_dir();

	koto_path_cache = g_build_path(G_DIR_SEPARATOR_S, user_cache_dir, koto_rev_dns,NULL);
	koto_path_config = g_build_path(G_DIR_SEPARATOR_S, user_config_dir, koto_rev_dns, NULL);
	koto_path_to_conf = g_build_filename(koto_path_config, "config.toml", NULL);
	koto_path_to_db = g_build_filename( koto_path_cache, "db", NULL);

	mkdir(user_cache_dir, 0755);
	mkdir(user_config_dir, 0755);
	mkdir(koto_path_cache, 0755);
	mkdir(koto_path_config, 0755);

	chown(user_cache_dir, getuid(), getgid());
	chown(user_config_dir, getuid(), getgid());
	chown(koto_path_cache, getuid(), getgid());
	chown(koto_path_config, getuid(), getgid());
}