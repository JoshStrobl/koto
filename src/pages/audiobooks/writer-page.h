/* writers-page.h
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
#include <gtk-4.0/gtk/gtk.h>

G_BEGIN_DECLS

#define KOTO_TYPE_WRITER_PAGE (koto_writer_page_get_type())
G_DECLARE_FINAL_TYPE(KotoWriterPage, koto_writer_page, KOTO, WRITER_PAGE, GObject)

GtkWidget* koto_writer_page_create_item(
	gpointer item,
	gpointer user_data
);

GtkWidget * koto_writer_page_get_main(KotoWriterPage * self);

void koto_writer_page_set_artist(
	KotoWriterPage * self,
	KotoArtist * artist
);

KotoWriterPage * koto_writer_page_new(KotoArtist * artist);

G_END_DECLS