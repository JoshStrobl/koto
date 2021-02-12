/* koto-playerbar.c
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

#include "koto-button.h"
#include "koto-config.h"
#include "koto-playerbar.h"

struct _KotoPlayerBar {
	GtkBox parent_instance;

	/* Sections */
	GtkWidget *playback_section;
	GtkWidget *primary_controls_section;
	GtkWidget *secondary_controls_section;

	/* Primary Buttons */
	KotoButton *back_button;
	KotoButton *play_pause_button;
	KotoButton *forward_button;
	KotoButton *repeat_button;
	KotoButton *shuffle_button;
	KotoButton *playlist_button;
	KotoButton *eq_button;
	GtkWidget *volume_button;

	/* Selected Playback Section */

	GtkWidget *playback_details_section;
	GtkWidget *artwork;
	GtkWidget *playback_title;
	GtkWidget *playback_album;
	GtkWidget *playback_artist;
};

struct _KotoPlayerBarClass {
	GtkBoxClass parent_class;
};

G_DEFINE_TYPE(KotoPlayerBar, koto_playerbar, GTK_TYPE_BOX);

static void koto_playerbar_constructed(GObject *obj);

static void koto_playerbar_class_init(KotoPlayerBarClass *c) {
	GObjectClass *gobject_class;
	gobject_class = G_OBJECT_CLASS(c);

	gobject_class->constructed = koto_playerbar_constructed;
}

static void koto_playerbar_constructed(GObject *obj) {
	KotoPlayerBar *self = KOTO_PLAYERBAR(obj);
	self->playback_section = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	self->primary_controls_section = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	self->secondary_controls_section = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	gtk_box_pack_start(GTK_BOX(self), GTK_WIDGET(self->primary_controls_section), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self), GTK_WIDGET(self->playback_section), TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(self), GTK_WIDGET(self->secondary_controls_section), FALSE, FALSE, 0);

	koto_playerbar_create_playback_details(self);
	koto_playerbar_create_primary_controls(self);
	koto_playerbar_create_secondary_controls(self);

	gtk_widget_set_margin_top(GTK_WIDGET(self), 10);
	gtk_widget_set_margin_bottom(GTK_WIDGET(self), 10);
	gtk_widget_set_margin_start(GTK_WIDGET(self), 10);
	gtk_widget_set_margin_end(GTK_WIDGET(self), 10);
}

static void koto_playerbar_init(KotoPlayerBar *self) {
	gtk_widget_show_all(GTK_WIDGET(self));
}

KotoPlayerBar* koto_playerbar_new(void) {
	return g_object_new(KOTO_TYPE_PLAYERBAR, "orientation", GTK_ORIENTATION_HORIZONTAL, NULL);
}

void koto_playerbar_create_playback_details(KotoPlayerBar* bar) {
	bar->playback_details_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	GtkIconTheme* default_icon_theme = gtk_icon_theme_get_default();

	if (default_icon_theme != NULL) {
		gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(bar));
		GdkPixbuf* audio_pixbuf = gtk_icon_theme_load_icon_for_scale(default_icon_theme, "audio-x-generic-symbolic", 96, scale_factor, GTK_ICON_LOOKUP_USE_BUILTIN & GTK_ICON_LOOKUP_FORCE_SIZE, NULL);

		if (audio_pixbuf != NULL) {
			bar->artwork = gtk_image_new_from_pixbuf(audio_pixbuf);
		}
	}

	if (bar->artwork == NULL) {
		bar->artwork = gtk_image_new_from_icon_name("audio-x-generic-symbolic", GTK_ICON_SIZE_DIALOG);
	}

	if (bar->artwork != NULL) {
		gtk_widget_set_margin_end(bar->artwork, 10);
		gtk_box_pack_start(GTK_BOX(bar->playback_section), bar->artwork, FALSE, FALSE, 0);
	}

	bar->playback_title = gtk_label_new("Title");
	bar->playback_album = gtk_label_new("Album");
	bar->playback_artist = gtk_label_new("Artist");

	gtk_box_pack_start(GTK_BOX(bar->playback_details_section), GTK_WIDGET(bar->playback_title), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(bar->playback_details_section), GTK_WIDGET(bar->playback_album), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(bar->playback_details_section), GTK_WIDGET(bar->playback_artist), TRUE, TRUE, 0);

	gtk_box_pack_end(GTK_BOX(bar->playback_section), GTK_WIDGET(bar->playback_details_section), FALSE, FALSE, 0);
}

void koto_playerbar_create_primary_controls(KotoPlayerBar* bar) {
	bar->back_button = koto_button_new_with_icon("", "media-skip-backward-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->play_pause_button = koto_button_new_with_icon("", "media-playback-start-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_LARGE); // TODO: Have this take in a state and switch to a different icon if necessary
	bar->forward_button = koto_button_new_with_icon("", "media-skip-forward-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);

	if (bar->back_button != NULL) {
		gtk_box_pack_start(GTK_BOX(bar->primary_controls_section), GTK_WIDGET(bar->back_button), FALSE, FALSE, 0);
	}

	if (bar->play_pause_button != NULL) {
		gtk_box_pack_start(GTK_BOX(bar->primary_controls_section), GTK_WIDGET(bar->play_pause_button), FALSE, FALSE, 0);
	}

	if (bar->forward_button != NULL) {
		gtk_box_pack_start(GTK_BOX(bar->primary_controls_section), GTK_WIDGET(bar->forward_button), FALSE, FALSE, 0);
	}
}

void koto_playerbar_create_secondary_controls(KotoPlayerBar* bar) {
	bar->repeat_button = koto_button_new_with_icon("", "media-playlist-repeat-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->shuffle_button = koto_button_new_with_icon("", "media-playlist-shuffle-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->playlist_button = koto_button_new_with_icon("", "playlist-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->eq_button = koto_button_new_with_icon("", "multimedia-equalizer-symbolic", NULL, KOTO_BUTTON_PIXBUF_SIZE_NORMAL);
	bar->volume_button = gtk_volume_button_new(); // Have this take in a state and switch to a different icon if necessary
	gtk_scale_button_set_value(GTK_SCALE_BUTTON(bar->volume_button), 0.5);

	if (bar->repeat_button != NULL) {
		gtk_box_pack_start(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->repeat_button), FALSE, FALSE, 0);
	}

	if (bar->shuffle_button != NULL) {
		gtk_box_pack_start(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->shuffle_button), FALSE, FALSE, 0);
	}

	if (bar->playlist_button != NULL) {
		gtk_box_pack_start(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->playlist_button), FALSE, FALSE, 0);
	}

	if (bar->eq_button != NULL) {
		gtk_box_pack_start(GTK_BOX(bar->secondary_controls_section), GTK_WIDGET(bar->eq_button), FALSE, FALSE, 0);
	}

	if (bar->volume_button != NULL) {
		gtk_box_pack_start(GTK_BOX(bar->secondary_controls_section), bar->volume_button, FALSE, FALSE, 0);
	}
}
