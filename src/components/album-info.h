/* album-info.h
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
#include "../config/config.h"

G_BEGIN_DECLS

typedef enum {
	KOTO_ALBUM_INFO_TYPE_ALBUM,
	KOTO_ALBUM_INFO_TYPE_AUDIOBOOK,
	KOTO_ALBUM_INFO_TYPE_PODCAST
} KotoAlbumInfoType;

#define KOTO_TYPE_ALBUM_INFO (koto_album_info_get_type())
G_DECLARE_FINAL_TYPE(KotoAlbumInfo, koto_album_info, KOTO, ALBUM_INFO, GtkBox);
#define KOTO_IS_ALBUM_INFO(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), KOTO_TYPE_ALBUM_INFO))

void koto_album_info_apply_configuration_state(
	KotoConfig * c,
	guint prop_id,
	KotoAlbumInfo * self
);

void koto_album_info_set_album_uuid(
	KotoAlbumInfo * self,
	gchar * album_uuid
);

void koto_album_info_set_type(
	KotoAlbumInfo * self,
	const gchar * type
);

KotoAlbumInfo * koto_album_info_new(gchar * type);