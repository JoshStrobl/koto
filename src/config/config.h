/* config.h
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

#pragma once
#include <glib-2.0/glib.h>
#include <glib-2.0/gio/gio.h>

G_BEGIN_DECLS

/**
 * Type Definition
 **/

#define KOTO_TYPE_CONFIG (koto_config_get_type())

G_DECLARE_FINAL_TYPE(KotoConfig, koto_config, KOTO, CONFIG, GObject)

KotoConfig* koto_config_new();
void koto_config_load(KotoConfig * self, gchar *path);
void koto_config_monitor_handle_changed(GFileMonitor * monitor, GFile * file, GFile *other_file, GFileMonitorEvent ev, gpointer user_data);
void koto_config_refresh(KotoConfig * self);
void koto_config_save(KotoConfig * self);

G_END_DECLS