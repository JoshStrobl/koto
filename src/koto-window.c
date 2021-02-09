/* koto-window.c
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

#include "indexer/file-indexer.h"
#include "koto-config.h"
#include "koto-headerbar.h"
#include "koto-nav.h"
#include "koto-playerbar.h"
#include "koto-window.h"

struct _KotoWindow {
	GtkApplicationWindow  parent_instance;
	KotoIndexedLibrary *library;
	KotoHeaderBar        *header_bar;
	GtkWidget *primary_layout;
	GtkWidget *content_layout;

	KotoNav *nav;
	KotoPlayerBar *player_bar;
};

G_DEFINE_TYPE (KotoWindow, koto_window, GTK_TYPE_APPLICATION_WINDOW)

static void koto_window_class_init (KotoWindowClass *klass) {
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	gtk_widget_class_set_template_from_resource (widget_class, "/com/github/joshstrobl/koto/koto-window.ui");
}

static void koto_window_init (KotoWindow *self) {
	gtk_widget_init_template (GTK_WIDGET (self));
	GtkCssProvider* provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource(provider, "/com/github/joshstrobl/koto/style.css");
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	//KotoHeaderBar *header = koto_headerbar_new();
	self->header_bar = koto_headerbar_new();

	if (self->header_bar != NULL) {
		gtk_window_set_titlebar(GTK_WINDOW(self), GTK_WIDGET(self->header_bar));
	}

	self->primary_layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	self->content_layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	self->nav = koto_nav_new();

	if (self->nav != NULL) {
		gtk_box_pack_start(GTK_BOX(self->content_layout), GTK_WIDGET(self->nav), FALSE, TRUE, 10);

		GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
		gtk_box_pack_end(GTK_BOX(self->content_layout), sep, FALSE, TRUE, 0);
	}

	gtk_box_pack_start(GTK_BOX(self->primary_layout), self->content_layout, TRUE, TRUE, 0);

	self->player_bar = koto_playerbar_new();

	if (self->player_bar != NULL) {
		gtk_box_pack_start(GTK_BOX(self->primary_layout), GTK_WIDGET(self->player_bar), FALSE, FALSE, 0);
	}

	gtk_container_add(GTK_CONTAINER(self), self->primary_layout);
	gtk_widget_show_all(GTK_WIDGET(self));

	g_thread_new("load-library", load_library, self);
}

void load_library(KotoWindow *self) {
	KotoIndexedLibrary *lib = koto_indexed_library_new(g_get_user_special_dir(G_USER_DIRECTORY_MUSIC));

	if (lib != NULL) {
		self->library = lib;
	}
}
