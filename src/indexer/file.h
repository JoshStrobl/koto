/* file.h
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
#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define KOTO_TYPE_INDEXED_FILE koto_indexed_file_get_type()
G_DECLARE_FINAL_TYPE(KotoIndexedFile, koto_indexed_file, KOTO, INDEXED_FILE, GObject);

KotoIndexedFile* koto_indexed_file_new(const gchar *path, guint *cd);

void koto_indexed_file_parse_name(KotoIndexedFile *self);
void koto_indexed_file_set_file_name(KotoIndexedFile *self, gchar *new_file_name);
void koto_indexed_file_set_cd(KotoIndexedFile *self, guint cd);
void koto_indexed_file_set_parsed_name(KotoIndexedFile *self, gchar *new_parsed_name);
void koto_indexed_file_set_position(KotoIndexedFile *self, guint pos);
void koto_indexed_file_update_metadata(KotoIndexedFile *self);
void koto_indexed_file_update_path(KotoIndexedFile *self, const gchar *new_path);

G_END_DECLS
