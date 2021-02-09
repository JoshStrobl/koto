/* koto-headerbar.h
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

#define KOTO_TYPE_HEADERBAR (koto_headerbar_get_type())

G_DECLARE_FINAL_TYPE (KotoHeaderBar, koto_headerbar, KOTO, HEADERBAR, GtkHeaderBar)

KotoHeaderBar* koto_headerbar_new (void);

G_END_DECLS
