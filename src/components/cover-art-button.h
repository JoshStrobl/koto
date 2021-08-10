/* cover-art-button.h
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
#include "button.h"

G_BEGIN_DECLS

#define KOTO_TYPE_COVER_ART_BUTTON (koto_cover_art_button_get_type())
G_DECLARE_FINAL_TYPE(KotoCoverArtButton, koto_cover_art_button, KOTO, COVER_ART_BUTTON, GObject);
#define KOTO_IS_COVER_ART_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_COVER_ART_BUTTON))

/**
 * Cover Art Functions
 **/

KotoCoverArtButton * koto_cover_art_button_new(
	guint height,
	guint width,
	gchar * art_path
);

KotoButton * koto_cover_art_button_get_button(KotoCoverArtButton * self);

GtkWidget * koto_cover_art_button_get_main(KotoCoverArtButton * self);

void koto_cover_art_button_hide_overlay_controls(
	GtkEventControllerFocus * controller,
	gpointer data
);

void koto_cover_art_button_set_art_path(
	KotoCoverArtButton * self,
	gchar * art_path
);

void koto_cover_art_button_set_dimensions(
	KotoCoverArtButton * self,
	guint height,
	guint width
);

void koto_cover_art_button_show_overlay_controls(
	GtkEventControllerFocus * controller,
	gpointer data
);

G_END_DECLS