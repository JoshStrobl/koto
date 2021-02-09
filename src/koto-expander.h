/* koto-expander.h
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

#include <gtk-3.0/gtk/gtk.h>

G_BEGIN_DECLS

#define KOTO_TYPE_EXPANDER (koto_expander_get_type())

G_DECLARE_FINAL_TYPE (KotoExpander, koto_expander, KOTO, EXPANDER, GtkBox)

KotoExpander* koto_expander_new(gchar *primary_icon_name, gchar *primary_label_text);
KotoExpander* koto_expander_new_with_button(gchar *primary_icon_name, gchar *primary_label_text, GtkWidget *secondary_button);
void koto_expander_set_icon_name(KotoExpander *self, const gchar *in);
void koto_expander_set_label(KotoExpander *self, const gchar *label);
void koto_expander_set_secondary_button(KotoExpander *self, GtkWidget *new_button);
void koto_expander_set_content(KotoExpander *self, GtkWidget *new_content);
void koto_expander_toggle_content(GtkWidget *button, gpointer data);

G_END_DECLS
