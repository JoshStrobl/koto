/* koto-headerbar.c
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

#include "koto-config.h"
#include "koto-headerbar.h"

struct _KotoHeaderBar {
	GtkHeaderBar parent_instance;
	GtkButton *menu_button;
	GtkWidget *search;
};

struct _KotoHeaderBarClass {
	GtkHeaderBarClass parent_class;
};

G_DEFINE_TYPE(KotoHeaderBar, koto_headerbar, GTK_TYPE_HEADER_BAR)

static void koto_headerbar_class_init(KotoHeaderBarClass *c) {
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(c);
	gtk_widget_class_set_template_from_resource(widget_class, "/com/github/joshstrobl/koto/koto-headerbar.ui");
}

static void koto_headerbar_init(KotoHeaderBar *self) {
	gtk_widget_init_template(GTK_WIDGET(self));
	gtk_widget_show_all(GTK_WIDGET(self));
}

KotoHeaderBar* koto_headerbar_new(void) {
	return g_object_new(KOTO_TYPE_HEADERBAR, NULL);
}
