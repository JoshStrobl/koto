/* genres-banner.h
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

G_BEGIN_DECLS

#define KOTO_TYPE_AUDIOBOOKS_GENRE_BANNER koto_audiobooks_genres_banner_get_type()
#define KOTO_AUDIOBOOKS_GENRES_BANNER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), KOTO_TYPE_AUDIOBOOKS_GENRE_BANNER, KotoAudiobooksGenresBanner))
typedef struct _KotoAudiobooksGenresBanner KotoAudiobooksGenresBanner;
typedef struct _KotoAudiobooksGenresBannerClass KotoAudiobooksGenresBannerClass;

GLIB_AVAILABLE_IN_ALL
GType koto_audiobooks_genres_banner_get_type(void) G_GNUC_CONST;

#define KOTO_IS_AUDIOBOOKS_GENRES_BANNER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_AUDIOBOOKS_GENRE_BANNER))

void koto_audiobooks_genres_banner_add_genre(
	KotoAudiobooksGenresBanner * self,
	gchar * genre_id
);

GtkWidget * koto_audiobooks_genres_banner_get_main(KotoAudiobooksGenresBanner * self);

KotoAudiobooksGenresBanner * koto_audiobooks_genres_banner_new();

G_END_DECLS