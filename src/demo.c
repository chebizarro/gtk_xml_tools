/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * demo.c
 * Copyright (C) 2012 Chris Daley <chebizarro@gmail.com>
 * 
 * xml-list is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * xml-list is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gtk/gtk.h>

#include <glib/gi18n.h>

#include "xmltools.h"
#include "xmltreemodel.h"
#include "xmlnavigator.h"


static xmlTreeModel		*xmllist;
static GtkWidget	*navigator;
static GtkWidget	*toolbar;
static GtkWidget	*view,*tools_view_vbox;

/* Shows the open file dialog and returns the selectd filepath */ 
static gchar *show_open_dialog() {
	GtkWidget *dialog;
	gchar *filename = NULL;
	dialog = gtk_file_chooser_dialog_new("Choose File",
						  NULL,
						  GTK_FILE_CHOOSER_ACTION_OPEN,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						  NULL);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
	}
	gtk_widget_destroy(dialog);
	return filename;
}

/* Opens the file selection dialog and adds the selected
 *  file to the project window */
static void add_file(GtkWidget *source, gchar *file_path) {
	if(!file_path) {
		file_path = show_open_dialog();
		if(!file_path) {
			return;
		}
	}

	gtk_widget_destroy(view);
	
	/* Create and set up the xmlTreeModel */
	xmllist = xml_tree_model_new();
	xml_tree_model_add_file(xmllist,file_path);

	xml_navigator_set_model(XML_NAVIGATOR(navigator), XML_TREE_MODEL(xmllist));	

}


/* Creates the main toolbar */
static GtkWidget *make_toolbar(void)
{
	GtkWidget *wid, *toolbar;

	/* Create the toolbar object and initialise it*/
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	/* Project open menu item */
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_OPEN));
	g_signal_connect(wid, "clicked", G_CALLBACK(add_file), NULL);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	/* Preferences */
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_PREFERENCES));
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	/* Help */
	wid = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_HELP));
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	return toolbar;
}

static on_tree_view_refresh(GtkWidget *widget, GdkEventButton * data) {

}

static gboolean
on_navigator_activated(	XmlNavigator *widget,
						GtkTreeSelection *selection)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint line = 0;
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, XML_TREE_MODEL_COL_LINE, &line, -1);
		if (line > 0)
		{
		}
	}
	return TRUE;
}

static GtkWidget*
create_window (void)
{
	GtkWidget *window;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "XML Tools");

	/* Exit when the window is closed */
	g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	
	return window;
}


int
main (int argc, char *argv[])
{
	GtkWidget *window;
	
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
 
	gtk_init(&argc,&argv);
 
	window = create_window ();

	gtk_window_set_default_size (GTK_WINDOW(window), 400, 400);

	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

	navigator = xml_navigator_new();

	g_signal_connect(navigator, "xml-row-activated",
			G_CALLBACK(on_navigator_activated), NULL);

	
	/* Create the Main Toolbar*/
	GtkWidget *toolbar;
	tools_view_vbox = gtk_vbox_new(FALSE, 3);
	toolbar = make_toolbar();
	gtk_box_pack_start(GTK_BOX(tools_view_vbox), toolbar, FALSE, FALSE, 0);

	
	gtk_container_add(GTK_CONTAINER(tools_view_vbox), navigator);
	
	gtk_container_add(GTK_CONTAINER(window), tools_view_vbox);

	gtk_widget_show_all(window);

	gtk_main();
	 
	return 0;
}
