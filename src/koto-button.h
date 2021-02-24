/* koto-button.h
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
#include <gdk-pixbuf-2.0/gdk-pixbuf/gdk-pixbuf.h>
#include <gtk-4.0/gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	KOTO_BUTTON_PIXBUF_SIZE_INVALID,
	KOTO_BUTTON_PIXBUF_SIZE_TINY,
	KOTO_BUTTON_PIXBUF_SIZE_SMALL,
	KOTO_BUTTON_PIXBUF_SIZE_NORMAL,
	KOTO_BUTTON_PIXBUF_SIZE_LARGE,
	KOTO_BUTTON_PIXBUF_SIZE_MASSIVE,
	KOTO_BUTTON_PIXBUF_SIZE_GODLIKE
} KotoButtonPixbufSize;

#define NUM_BUILTIN_SIZES 7

#define KOTO_TYPE_BUTTON (koto_button_get_type())

G_DECLARE_FINAL_TYPE (KotoButton, koto_button, KOTO, BUTTON, GtkBox)

guint koto_get_pixbuf_size(KotoButtonPixbufSize size);

KotoButton* koto_button_new_plain(gchar *label);
KotoButton* koto_button_new_with_icon(gchar *label, gchar *icon_name, gchar *alt_icon_name, KotoButtonPixbufSize size);
KotoButton* koto_button_new_with_pixbuf(gchar *label, GdkPixbuf *pix, KotoButtonPixbufSize size);

void koto_button_flip(KotoButton *self);
void koto_button_set_badge_text(KotoButton *self, gchar *text);
void koto_button_set_icon_name(KotoButton *self, gchar *icon_name, gboolean for_alt);
void koto_button_set_pixbuf(KotoButton *self, GdkPixbuf *pix);
void koto_button_set_pixbuf_size(KotoButton *self, guint size);
void koto_button_set_text(KotoButton *self, gchar *text);
void koto_button_show_image(KotoButton *self, gboolean use_alt);

G_END_DECLS
