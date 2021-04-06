/* mpris.h
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
#include <glib-2.0/gio/gio.h>
#include "../indexer/structs.h"

void koto_push_track_info_to_builder(GVariantBuilder *builder, KotoIndexedTrack *track);
void koto_update_mpris_info_for_track(KotoIndexedTrack *track);
void handle_main_mpris_method_call(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,	const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data);
GVariant* handle_get_property(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,	const gchar *property_name,	GError **error,	gpointer user_data);
void on_main_mpris_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data);
void setup_mpris_interfaces();
