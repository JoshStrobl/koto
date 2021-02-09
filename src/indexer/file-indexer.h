/* file-indexer.h
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

#define KOTO_INDEXED_TYPE_FILE koto_indexed_file_get_type()
G_DECLARE_FINAL_TYPE(KotoIndexedFile, koto_indexed_file, KOTO, INDEXED_FILE, GObject);

#define KOTO_INDEXED_TYPE_LIBRARY koto_indexed_library_get_type()
G_DECLARE_FINAL_TYPE(KotoIndexedLibrary, koto_indexed_library, KOTO, INDEXED_LIBRARY, GObject);

KotoIndexedLibrary* koto_indexed_library_new(const gchar *path);
KotoIndexedFile* koto_indexed_file_new(gchar *path);

void start_indexing(KotoIndexedLibrary *self);
void index_folder(KotoIndexedLibrary *self, gchar *path);
void index_file(KotoIndexedLibrary *self, gchar *path);

G_END_DECLS
