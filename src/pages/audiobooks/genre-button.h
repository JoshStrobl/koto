/* genres-button.h
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
#include <gtk-4.0/gtk/gtk.h>
#include "../../components/button.h"

G_BEGIN_DECLS

#define KOTO_TYPE_AUDIOBOOKS_GENRE_BUTTON koto_audiobooks_genre_button_get_type()
G_DECLARE_FINAL_TYPE(KotoAudiobooksGenreButton, koto_audiobooks_genre_button, KOTO, AUDIOBOOKS_GENRE_BUTTON, GtkBox)
#define KOTO_IS_AUDIOBOOKS_GENRE_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_AUDIOBOOKS_GENRE_BUTTON))

KotoButton * koto_audiobooks_genre_button_get_button(KotoAudiobooksGenreButton * self);

gchar * koto_audiobooks_genre_button_get_id(KotoAudiobooksGenreButton * self);

gchar * koto_audiobooks_genre_button_get_name(KotoAudiobooksGenreButton * self);

void koto_audiobooks_genre_button_set_id(
	KotoAudiobooksGenreButton * self,
	gchar * genre_id
);

void koto_audiobooks_genre_button_set_name(
	KotoAudiobooksGenreButton * self,
	gchar * genre_name
);

KotoAudiobooksGenreButton * koto_audiobooks_genre_button_new(
	gchar * genre_id,
	gchar * genre_name
);

G_END_DECLS