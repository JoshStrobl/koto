/* koto-playerbar.h
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
#include <gtk-4.0/gtk/gtk.h>

G_BEGIN_DECLS

#define KOTO_TYPE_PLAYERBAR (koto_playerbar_get_type())

G_DECLARE_FINAL_TYPE (KotoPlayerBar, koto_playerbar, KOTO, PLAYERBAR, GObject)

KotoPlayerBar* koto_playerbar_new (void);
GtkWidget* koto_playerbar_get_main(KotoPlayerBar* bar);
void koto_playerbar_create_playback_details(KotoPlayerBar* bar);
void koto_playerbar_create_primary_controls(KotoPlayerBar* bar);
void koto_playerbar_create_secondary_controls(KotoPlayerBar* bar);

G_END_DECLS
