/* main.c
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

#include <glib/gi18n.h>

#include "koto-config.h"
#include "koto-window.h"

static void on_activate (GtkApplication *app) {
	g_assert(GTK_IS_APPLICATION (app));

	GtkWindow *window;

	window = gtk_application_get_active_window (app);
	if (window == NULL) {
		window = g_object_new(KOTO_TYPE_WINDOW,  "application", app,  "default-width", 600,  "default-height", 300, NULL);
	}

	gtk_window_present(window);
}

int main (int argc, char *argv[]) {
	g_autoptr(GtkApplication) app = NULL;
	int ret;

	/* Set up gettext translations */
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gtk_init();
	app = gtk_application_new ("com.github.joshstrobl.koto", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
	ret = g_application_run (G_APPLICATION (app), argc, argv);

	return ret;
}
