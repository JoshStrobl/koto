/* library.h
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
#include <gtk-4.0/gtk/gtk.h>
#include "../../db/cartographer.h"
#include "../../indexer/structs.h"

G_BEGIN_DECLS

#define KOTO_TYPE_AUDIOBOOKS_LIBRARY_PAGE koto_audiobooks_library_page_get_type()
#define KOTO_AUDIOBOOKS_LIBRARY_PAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), KOTO_TYPE_AUDIOBOOKS_LIBRARY_PAGE, KotoAudiobooksLibraryPage))
typedef struct _KotoAudiobooksLibraryPage KotoAudiobooksLibraryPage;
typedef struct _KotoAudiobooksLibraryPageClass KotoAudiobooksLibraryPageClass;

GLIB_AVAILABLE_IN_ALL
GType koto_audiobooks_library_page_get_type(void) G_GNUC_CONST;

#define KOTO_IS_AUDIOBOOKS_LIBRARY_PAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_AUDIOBOOKS_LIBRARY_PAGE))

void koto_audiobooks_library_page_add_genres(
	KotoAudiobooksLibraryPage * self,
	GList * genres
);

void koto_audiobooks_library_page_handle_add_album(
	KotoCartographer * carto,
	KotoAlbum * album,
	KotoAudiobooksLibraryPage * self
);

void koto_audiobooks_library_page_handle_add_artist(
	KotoCartographer * carto,
	KotoArtist * artist,
	KotoAudiobooksLibraryPage * self
);

GtkWidget * koto_audiobooks_library_page_get_main(KotoAudiobooksLibraryPage * self);

KotoAudiobooksLibraryPage * koto_audiobooks_library_page_new();