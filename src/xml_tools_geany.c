/*
 *      xslteditor.c - an XSLT rendering plugin for Geany
 *
 *      Copyright 2012 Chris Daley <chebizarro(at)gmail(dot)com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 * $Id$
 */

//#include <config.h>
#include <gtk/gtk.h>

#include <glib/gi18n.h>

#include "xmltools.h"
#include "xmltreemodel.h"
#include "xmlnavigator.h"
#include "xpathexplorer.h"

#include "geanyplugin.h"


/* Check plugin version */
PLUGIN_VERSION_CHECK(GEANY_API_VERSION)

/* Set plugin info */
PLUGIN_SET_INFO(_("XML/XSLT Tools"), _("Adds XML/XSLT Tools to Geany."),
	"0.1.alpha",_("Chris Daley"))	

/* Geany plugin variables */
GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;

static gint			page_number = 0;
//static xmlTreeModel	* xmllist;
static XmlNavigator	* navigator;
static XmlNavigator	* navigator_blank;

static GtkWidget		* navbox;

static GHashTable		* views;

static gboolean change_focus_to_editor(GeanyDocument *doc);

void plugin_help(void) {
 //TODO
}

static void
on_xsl_menu_activated (	XmlNavigator 	* ttt,
						GtkWidget	 	* menu)
{
	GtkWidget 		*file_menu, *open_item;
	GeanyDocument	*doc;
	gint i;

	doc = document_get_current();
	file_menu = gtk_menu_new();
	for (i = 0; i < GEANY(documents_array)->len; i++) {
		if(documents[i]->is_valid && documents[i] != doc){
			if(documents[i]->file_type->id == GEANY_FILETYPES_XML) {
				open_item = gtk_menu_item_new_with_label(document_get_basename_for_display(documents[i],-1));
				//g_signal_connect(open_item, "activate", G_CALLBACK(xui_add_file), documents[i]->real_path);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), open_item);
				gtk_widget_show (open_item);
			}
		}
	}
	
}

static gboolean change_focus_to_editor(GeanyDocument *doc)
{
	if (DOC_VALID(doc))
	{
		gtk_window_get_focus(GTK_WINDOW(geany->main_widgets->window));
		GtkWidget *sci = GTK_WIDGET(doc->editor->sci);
		gtk_widget_grab_focus(sci);
	}
	return FALSE;
}

static gboolean
on_navigator_activated(	GtkWidget *widget,
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
			GeanyDocument *doc;
			doc = document_get_current();
			navqueue_goto_line(doc, doc, line);
			change_focus_to_editor(doc);
		}
	}
	return FALSE;
}

static void
on_document_filetype_set (	GObject *obj,
							GeanyDocument *doc,
							GeanyFiletype *filetype_old,
							gpointer user_data )
{
	XmlNavigator	* nav;
	xmlTreeModel	* model;

	if(doc == NULL)
		return;

	if(doc->file_type->id == GEANY_FILETYPES_XML) {
		model = xml_tree_model_new();
		xml_tree_model_add_file(model, doc->real_path);
		nav = g_hash_table_lookup(views, doc);
		xml_navigator_set_model(nav, model);
	} else {
		nav = navigator_blank;
	}
	
	if(doc == document_get_current()) {
		g_object_ref(navigator);
		gtk_container_remove(navbox, navigator);
		navigator = nav;
		gtk_box_pack_start(navbox, navigator, TRUE, TRUE, 0);
		gtk_widget_show_all(GTK_WIDGET(navigator));
	}

}

static void
on_document_save (	G_GNUC_UNUSED GObject *object,
					GeanyDocument *doc,
					gpointer data)
{
	XmlNavigator	* nav;
	xmlTreeModel	* model;

	if(doc->file_type->id == GEANY_FILETYPES_XML) {
		nav = g_hash_table_lookup(views, doc);
	
		if(nav == NULL) {
			nav = xml_navigator_new();
			model = xml_tree_model_new();
			xml_tree_model_add_file(model, doc->real_path);
			xml_navigator_set_model(nav, model);
			g_signal_connect(nav, "xml-row-activated", G_CALLBACK(on_navigator_activated), NULL);
			ui_widget_modify_font_from_string(GTK_WIDGET(nav->navigator), geany->interface_prefs->tagbar_font);
			ui_widget_modify_font_from_string(GTK_WIDGET(nav->transformer.parameters), geany->interface_prefs->tagbar_font);
			ui_widget_modify_font_from_string(GTK_WIDGET(nav->xpathExplorer.results), geany->interface_prefs->tagbar_font);
			//g_object_unref(model);
			g_hash_table_insert(views, doc, nav);
			//g_object_ref(nav);
		}
	} else {
		nav = navigator_blank;
	}
	
	if(doc == document_get_current()) {
		g_object_ref(navigator);
		gtk_container_remove(navbox, navigator);
		navigator = nav;
		gtk_box_pack_start(navbox, navigator, TRUE, TRUE, 0);
		gtk_widget_show_all(GTK_WIDGET(navigator));
	}
}

static gboolean
on_document_activate(	G_GNUC_UNUSED GObject *object,
						GeanyDocument *doc,
						gpointer data)
{
	XmlNavigator	* nav;
	xmlTreeModel	* model;

	if(doc->file_type->id == GEANY_FILETYPES_XML) {
		
		nav = g_hash_table_lookup(views, doc);
		
		if(nav == NULL) {
			nav = xml_navigator_new();
			model = xml_tree_model_new();
			xml_tree_model_add_file(model, doc->real_path);
			xml_navigator_set_model(nav, model);
			g_signal_connect(nav, "xml-row-activated", G_CALLBACK(on_navigator_activated), NULL);
			g_signal_connect(nav, "xsl-menu-activated", G_CALLBACK(on_xsl_menu_activated), NULL);
			ui_widget_modify_font_from_string(GTK_WIDGET(nav->navigator), geany->interface_prefs->tagbar_font);
			ui_widget_modify_font_from_string(GTK_WIDGET(nav->transformer.parameters), geany->interface_prefs->tagbar_font);
			ui_widget_modify_font_from_string(GTK_WIDGET(nav->xpathExplorer.results), geany->interface_prefs->tagbar_font);
			//g_object_unref(model);
			g_hash_table_insert(views, doc, nav);
			//g_object_ref(nav);
		}
	
	} else {
		nav = navigator_blank;
	}
		
	g_object_ref(navigator);
	
	gtk_container_remove(navbox, navigator);
		
	navigator = nav;
		
	gtk_box_pack_start(navbox, nav, TRUE, TRUE, 0);
		
	gtk_widget_show_all(GTK_WIDGET(navigator));
	
	return TRUE;
}

static void on_document_close(G_GNUC_UNUSED GObject *object,
								GeanyDocument *doc)
{
	XmlNavigator	* nav;

	if(doc == NULL)
		return FALSE;
		
	if(doc == document_get_current()) {
		g_object_ref(navigator);
		gtk_container_remove(navbox, navigator);
		navigator = navigator_blank;
		gtk_box_pack_start(navbox, navigator, TRUE, TRUE, 0);
		gtk_widget_show_all(GTK_WIDGET(navigator));
	}
	
	if(doc->file_type->id == GEANY_FILETYPES_XML) {
		nav = g_hash_table_lookup(views, doc);
		gtk_widget_destroy(nav);
		g_hash_table_remove(views, doc);
	}
}



static void
message_logger (const gchar *log_domain,
                GLogLevelFlags log_level,
                const gchar *message,
                gpointer user_data)
{
	int error_colour;

	msgwin_clear_tab(MSG_MESSAGE);
	msgwin_switch_tab(MSG_MESSAGE, TRUE);
		
	switch(log_level)
	{
		case G_LOG_LEVEL_WARNING:
			error_colour = COLOR_RED;
			break;
		case G_LOG_LEVEL_MESSAGE:
			error_colour = COLOR_BLUE;
			break;
		default:
			error_colour = COLOR_BLUE;
			break;
	}
	
	msgwin_msg_add(error_colour,0,NULL,message);

}

/* Plugin initialisation */
void
plugin_init(GeanyData *data)
{
	LIBXML_TEST_VERSION

	xml_icon_factory_new();
	
	navigator = xml_navigator_new();
	navigator_blank = navigator;
	
	navbox = gtk_vbox_new(FALSE,0);
		
	views = g_hash_table_new(NULL, NULL);
	
	gtk_box_pack_start(navbox, navigator, TRUE, TRUE, 0);
	
	gtk_widget_set_sensitive(navigator, FALSE);

	gtk_widget_show_all(GTK_WIDGET(navbox));
	
	page_number = gtk_notebook_append_page(GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook), GTK_WIDGET(navbox), gtk_label_new(_("XML Tools")));

	plugin_signal_connect(geany_plugin, NULL, "document-activate", FALSE,
		(GCallback)&on_document_activate, NULL);

	plugin_signal_connect(geany_plugin, NULL, "document-open", TRUE,
		(GCallback)&on_document_activate, NULL);

	plugin_signal_connect(geany_plugin, NULL, "document-close", TRUE,
		(GCallback)&on_document_close, NULL);

	plugin_signal_connect(geany_plugin, NULL, "document-save", TRUE,
		(GCallback)&on_document_save, NULL);

/*
	plugin_signal_connect(geany_plugin, NULL, "document-filetype-set", TRUE,
		(GCallback)&on_document_filetype_set, NULL);

	plugin_signal_connect(geany_plugin, NULL, "document-new", TRUE,
		(GCallback)&on_document_close, NULL);

	plugin_signal_connect(geany_plugin, NULL, "document-reload", TRUE,
		(GCallback)&on_document_close, NULL);

*/



	g_log_set_handler(XML_TREE_MESSAGE, G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE, message_logger, NULL);

	plugin_module_make_resident(geany_plugin);
}

/* Plugin cleanup */
void plugin_cleanup(void)
{
	msgwin_clear_tab(MSG_MESSAGE);

	gtk_notebook_remove_page (GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook), page_number);
}

GtkWidget *plugin_configure(GtkDialog *dialog)
{
	GtkWidget *vbox, *show_toolbar;
	vbox = gtk_vbox_new(FALSE, 6);
	
	show_toolbar = gtk_check_button_new_with_label(
		_("Show toolbar"));


	gtk_box_pack_start(GTK_BOX(vbox), show_toolbar, FALSE, FALSE, 3);
	
	return vbox;
}
