/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) 2012 Chris Daley <chris@chris-Latitude>
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

#include "xmlmodel.h"

static GtkWidget * create_view (XmlList *list);

/* vbox widget for file vies */
static GtkWidget	*tools_view_box,
					*scrolled_window,
					*files_view_vbox,
					*view,
					*xpath_entry;
static XmlList		*xmllist;

/* Create the xpath text entry */
static GtkWidget *make_xpath_entry() {
	GtkWidget *hbox, *xlabel;
	hbox = gtk_hbox_new(FALSE, 3);
	xlabel = gtk_label_new("Xpath");
	gtk_box_pack_start(GTK_BOX(hbox), xlabel,FALSE , TRUE, 0);
	xpath_entry = gtk_entry_new();
	//g_signal_connect(xpath_entry, "activate", G_CALLBACK(xui_on_xpath_activate),xfile);
    gtk_box_pack_start(GTK_BOX(hbox), xpath_entry, TRUE, TRUE, 0);
			gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);


	
	return hbox;
}

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
	
	/* Create and set up the XmlList */
	xmllist = xml_list_new();
	xml_list_add_file(xmllist,file_path);

	/* Set an xpath */
	//xml_list_set_xpath (xmllist,"/*");	

	/* Set DTD Nodes to visible -
	 only applicable where there is a filter applied*/
	xml_list_set_visible (xmllist, XML_DTD_NODE, TRUE);

	view = create_view(xmllist);

	/* Create filter with virtual root set to second branch*/
	GtkTreePath *virtual_root;
	virtual_root = gtk_tree_path_new_from_string("0");
	
	/* Create filter and set visible column */
	GtkWidget *filter;
	filter = gtk_tree_model_filter_new(xmllist, virtual_root );
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER( filter ), XML_LIST_COL_VISIBLE );
	g_object_unref( G_OBJECT( xmllist ) );

	
	//view = create_view(filter);
	
	/* Set searchable */
	//gtk_tree_view_set_enable_search(view, TRUE);

	GtkEntryCompletion *completion;
	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 5);
	gtk_entry_completion_set_inline_completion(completion, TRUE);
	//g_object_set (completion, "text-column", XML_LIST_COL_XPATH, NULL);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(xmllist));
	g_object_unref(xmllist);
	gtk_entry_set_completion(GTK_ENTRY(xpath_entry), completion);

	GtkEntryCompletion *comp;
	g_object_set (xpath_entry, "truncate-multiline", TRUE, NULL);

	comp = gtk_entry_completion_new ();
	gtk_entry_completion_set_popup_single_match (comp, FALSE);
	gtk_entry_completion_set_minimum_key_length (comp, 0);

	/* see docs for gtk_entry_completion_set_text_column() */
	g_object_set (comp, "text-column", XML_LIST_COL_XPATH, NULL);

	/* Need a match func here or entry completion uses a wrong one.
	 * We do our own filtering after all. */
	gtk_entry_completion_set_match_func (comp,
										(GtkEntryCompletionMatchFunc) gtk_true,
				   						xpath_entry,
				   						NULL);
	GtkCellRenderer *cell;
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (comp),
                          		cell, TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (comp),
                            		cell,
                             	   "text", XML_LIST_COL_XPATH);

	//g_signal_connect (comp, "match-selected",
	//	    G_CALLBACK (match_selected_callback), xpath_entry);

	//gtk_entry_set_completion (GTK_ENTRY (xpath_entry), comp);
	g_object_unref (comp);
	/* NB: This needs to happen after the completion is set, so this handler
	 * runs before the handler installed by entrycompletion */
	//g_signal_connect (chooser_entry, "key-press-event",
    //              G_CALLBACK (xml_xpathentry_tab_handler), NULL);

	
	gtk_container_add(GTK_CONTAINER(files_view_vbox), view);	
	gtk_widget_show_all(view);
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

static GtkWidget *
create_view (XmlList *list)
{
  GtkTreeViewColumn		*col;
  GtkCellRenderer		*renderer;
  GtkWidget				*view;
 
  view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
 
  g_object_unref(list); /* destroy store automatically with view */
 
  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();
 
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text", XML_LIST_COL_NAME);
  gtk_tree_view_column_set_title (col, "Name");
	gtk_tree_view_column_set_resizable(col, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);
 
  renderer = gtk_cell_renderer_text_new();
  g_object_set(renderer,"ellipsize", PANGO_ELLIPSIZE_END, NULL);
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text", XML_LIST_COL_CONTENT);
  gtk_tree_view_column_set_title (col, "Value");
	gtk_tree_view_column_set_resizable(col, TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

  renderer = gtk_cell_renderer_text_new();
	
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text", XML_LIST_COL_XPATH);
  gtk_tree_view_column_set_title (col, "Path");
  gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);	

	return view;
}

static GtkWidget*
create_window (void)
{
	GtkWidget *window;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "xml-list");

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

	gtk_window_set_keep_above(window, TRUE);


	/* Create the Main Toolbar*/
	GtkWidget *toolbar;
	tools_view_box = gtk_vbox_new(FALSE, 3);
	toolbar = make_toolbar();
	gtk_box_pack_start(GTK_BOX(tools_view_box), toolbar, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(tools_view_box), make_xpath_entry(), FALSE, FALSE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (tools_view_box), 2);
	/* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (tools_view_box), scrolled_window, TRUE, TRUE, 0);
	files_view_vbox = gtk_vbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), files_view_vbox);
	
	view = gtk_label_new ("Add a file");
	
	gtk_container_add(GTK_CONTAINER(files_view_vbox), view);	
	gtk_container_add(GTK_CONTAINER(window), tools_view_box);

	gtk_widget_show_all(window);

	//xml_list_add_file(xmllist,"/home/chris/Documents/Dev/xslt-editor/Samples/bible/BasicEnglish.xml");
	gtk_main();
	 
	return 0;
}
