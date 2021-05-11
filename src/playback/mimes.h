/* mimes.h
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
#include <gstreamer-1.0/gst/gst.h>

G_BEGIN_DECLS

gboolean koto_bplayback_engine_gst_caps_iter(
	GstCapsFeatures * features,
	GstStructure * structure,
	gpointer user_data
);

void koto_playback_engine_gst_pad_iter(
	gpointer list_data,
	gpointer user_data
);

void koto_playback_engine_get_supported_mimetypes(GList * mimes);

G_END_DECLS
