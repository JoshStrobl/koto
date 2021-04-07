/* mimes.c
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
#include <gstreamer-1.0/gst/gst.h>
#include "../koto-utils.h"

GHashTable *supported_mimes_hash = NULL;
GList *supported_mimes = NULL;

gboolean koto_playback_engine_gst_caps_iter(GstCapsFeatures *features, GstStructure *structure, gpointer user_data) {
	(void) features; (void) user_data;
	gchar *caps_name = (gchar*) gst_structure_get_name(structure); // Get the name, typically a mimetype

	if (g_str_has_prefix(caps_name, "unknown")) { // Is unknown
		return TRUE;
	}

	if (g_hash_table_contains(supported_mimes_hash, caps_name)) { // Found in list already
		return TRUE;
	}

	g_hash_table_add(supported_mimes_hash, g_strdup(caps_name));
	supported_mimes = g_list_prepend(supported_mimes, g_strdup(caps_name));
	supported_mimes = g_list_prepend(supported_mimes, g_strdup(koto_utils_replace_string_all(caps_name, "x-", "")));

	return TRUE;
}

void koto_playback_engine_gst_pad_iter(gpointer list_data, gpointer user_data) {
	(void) user_data;
	GstStaticPadTemplate *templ = list_data;
	if (templ->direction == GST_PAD_SINK) { // Is a sink pad
		GstCaps *capabilities = gst_static_pad_template_get_caps(templ); // Get the capabilities
		gst_caps_foreach(capabilities, koto_playback_engine_gst_caps_iter, NULL); // Iterate over and add to mimes
		gst_caps_unref(capabilities);
	}
}

void koto_playback_engine_get_supported_mimetypes() {
	// Credit for code goes to https://github.com/mopidy/mopidy/issues/812#issuecomment-75868363
	// These are GstElementFactory
	GList *elements = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_DEPAYLOADER | GST_ELEMENT_FACTORY_TYPE_DEMUXER | GST_ELEMENT_FACTORY_TYPE_PARSER | GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO, GST_RANK_NONE);

	GList *ele;
	for (ele = elements; ele != NULL; ele = ele->next) { // For each of the elements
		// GList of GstStaticPadTemplate
		GList *static_pads = (GList*) gst_element_factory_get_static_pad_templates(ele->data); // Get the pads
		g_list_foreach(static_pads, koto_playback_engine_gst_pad_iter, NULL);
	}
}
