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
#include "xslttransformer.h"

#include "geanyplugin.h"

typedef struct _xmlTools			xmlTools;

/*
struct _xmlTools
{
	xmlTreeModel	*model;
	XmlNavigator	*navigator;
	XpathExplorer	*explorer;
	xsltTransformer	*transformer;
};
*/

struct _xmlTools
{
	XmlNavigator	*navigator;
	XpathExplorer	*explorer;
	xsltTransformer	*transformer;
	/* Widgets after this point will not be displayed */
	xmlTreeModel	*model;
};


enum {
	XTOOLS_NAVIGATOR,
	XTOOLS_EXPLORER,
	XTOOLS_TRANSFORMER,
	XTOOLS_COUNT
};

static GHashTable	* xtools;
static xmlTools	* tools_blank;
static gint		xtools_pages[XTOOLS_COUNT];
static GtkWidget	*xtools_geany_pages[XTOOLS_COUNT];
static GtkWidget	*xtools_views_blank[XTOOLS_COUNT];
static GtkWidget	*xtools_views[XTOOLS_COUNT];

/* Check plugin version */
PLUGIN_VERSION_CHECK(GEANY_API_VERSION)

/* Set plugin info */
PLUGIN_SET_INFO(_("XML/XSLT Tools"), _("Adds XML/XSLT Tools to Geany."),
	"0.1.alpha","Chris Daley")	

/* Geany plugin variables */
GeanyPlugin		*geany_plugin;
GeanyData		*geany_data;
GeanyFunctions	*geany_functions;


static gboolean change_focus_to_editor(GeanyDocument *doc);

static gboolean on_document_new (	G_GNUC_UNUSED GObject *object,
									GeanyDocument 	* doc,
									xmlTreeModel 	* xmlmodel);
static on_xml_error (	xmlTreeModel * xmltreemodel,
						xmlErrorPtr error,
						XmlNavigator * ttt);

static	on_xsl_error (	xmlTreeModel * xmltreemodel,
				xslErrorMessage * error,
				XmlNavigator * ttt);

static xmlTools * xtools_new( GeanyDocument *doc);


void plugin_help(void) {
 //TODO
}

static void
on_xslt_add_file(GtkWidget * menuitem, xsltTransformer * ttt)
{
	XmlNavigator	* model;
	GeanyDocument	* doc;
	
	doc = document_find_by_filename(gtk_menu_item_get_label(menuitem));
	
	//model = g_hash_table_lookup(views, doc);
	
	if(model != NULL)
		xslt_transformer_set_stylesheet(ttt, model->model);
}

static void
on_xsl_menu_activated (	xsltTransformer 	* ttt,
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
				//open_item = gtk_menu_item_new_with_label(document_get_basename_for_display(documents[i],-1));
				open_item = gtk_menu_item_new_with_label(documents[i]->real_path);
				g_signal_connect(open_item, "activate", G_CALLBACK(on_xslt_add_file), ttt);
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
on_xpath_show_in_navigator (XpathExplorer *widget,
							gchar		  *xpath,
							XmlNavigator  *navigator)
{
	xml_navigator_goto_xpath(navigator, xpath);
	return TRUE;
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
		//nav = g_hash_table_lookup(views, doc);
		xml_navigator_set_model(nav, model);
	} else {
		//nav = navigator_blank;
	}
	
	if(doc == document_get_current()) {
		//g_object_ref(navigator);
		//gtk_container_remove(navbox, navigator);
		//navigator = nav;
		//gtk_box_pack_start(navbox, navigator, TRUE, TRUE, 0);
		//gtk_widget_show_all(GTK_WIDGET(navigator));
	}

}

static void
on_xsl_transform (	xsltTransformer 	* ttt,
					GtkWidget	 	* button)
{
	GeanyDocument* gresult;
	gint byteswritten;
	
	if(ttt->result == NULL)
		return;
	
	gchar template[] = "/tmp/xml_transformation_XXXXXX";
	gint file;
	file = g_mkstemp(template);
	
	if(file < 0)
		return;

	byteswritten = xml_tree_model_write_to_file(ttt->result, file, 1);

	if( byteswritten > 0) {
		gresult = document_open_file(template, TRUE, NULL, NULL);
	}

	close(file);
}

static gboolean
on_document_new(G_GNUC_UNUSED GObject *object,
				GeanyDocument 	* doc,
				xmlTreeModel 	* xmlmodel)
{
	
	return TRUE;
}


static void
on_document_save (	G_GNUC_UNUSED GObject *object,
					GeanyDocument *doc,
					gpointer data)
{

	xmlTools *tools;

	if(doc->file_type->id == GEANY_FILETYPES_XML) {
		tools = g_hash_table_lookup(xtools, doc);
	
		if(tools == NULL) {
			tools = xtools_new(doc);
			g_hash_table_insert(xtools, doc, tools);
		} else {
			xml_tree_model_reload(tools->model);
		}
	} else {
		tools = tools_blank;
	}
	
}

static
on_xsl_error (	xmlTreeModel * xmltreemodel,
				xslErrorMessage * error,
				XmlNavigator * ttt)
{
	int error_colour;
	GeanyDocument * doc;
   
	doc = document_find_by_real_path(error->file);

	//msgwin_switch_tab(MSG_MESSAGE, TRUE);
	
	if(error->file == NULL && error->line < 1 && error->element == NULL) {
		msgwin_msg_add(	COLOR_DARK_RED, NULL, doc,	"XSL: %s", error->error );
	} else {
		msgwin_msg_add(	COLOR_DARK_RED, error->line, doc,
					"XSL: %s: %s - %s line: %i",
					error->error, error->file, error->element, error->line );
	}
}

static
on_xml_error (	xmlTreeModel * xmltreemodel,
				xmlErrorPtr error,
				XmlNavigator * ttt)
{
	int error_colour;
	GeanyDocument * doc;
	gchar * errorlevel;
	
	switch(error->level)
	{
		case XML_ERR_NONE:
			error_colour = COLOR_BLACK;
			errorlevel = _("no error");
			break;
		case XML_ERR_WARNING:
			error_colour = COLOR_BLUE;
			errorlevel = _("warning");
			break;
		case XML_ERR_ERROR:
			error_colour = COLOR_DARK_RED;
			errorlevel = _("error");
			break;
		case XML_ERR_FATAL:
			error_colour = COLOR_RED;
			errorlevel = _("fatal error");
			break;
		default:
			error_colour = COLOR_BLACK;
			errorlevel = _("message");
			break;
    }
    
    
	doc = document_find_by_real_path(error->file);

	//msgwin_switch_tab(MSG_MESSAGE, TRUE);
	msgwin_msg_add(	error_colour, error->line, doc,
					"XML: %s: %s - %s Line: %i, Column : %i",
					errorlevel, error->file, error->message, error->line, error->int2 );
}



static xmlTools *
xtools_new( GeanyDocument *doc) {

	xmlTools * tools = g_malloc(sizeof(* tools));

	tools->model = xml_tree_model_new();
	g_signal_connect(tools->model, "xml-tree-model-error", G_CALLBACK(on_xml_error), NULL);
	g_signal_connect(tools->model, "xml-tree-model-xsl-error", G_CALLBACK(on_xsl_error), NULL);

	if(doc->real_path != NULL) {
		xml_tree_model_add_file(tools->model, doc->real_path);
	} 

	tools->navigator = xml_navigator_new();
	
	xml_navigator_set_model(tools->navigator, tools->model);
	g_signal_connect(tools->navigator, "xml-row-activated", G_CALLBACK(on_navigator_activated), NULL);
	
	tools->explorer = xpath_explorer_new();
	xpath_explorer_set_model(tools->explorer, tools->model);
	g_signal_connect(tools->explorer, "xpath-row-activated", G_CALLBACK(on_navigator_activated), NULL);
	g_signal_connect(tools->explorer, "xpath-show-in-navigator", G_CALLBACK(on_xpath_show_in_navigator), tools->navigator);
	ui_widget_modify_font_from_string(GTK_WIDGET(tools->explorer->results), geany->interface_prefs->tagbar_font);

	tools->transformer = xslt_transformer_new();
	xslt_transformer_set_model(tools->transformer, tools->model);
	g_signal_connect(tools->navigator, "xslt-transformer-menu-activated", G_CALLBACK(on_xsl_menu_activated), NULL);
	g_signal_connect(tools->navigator, "xslt-transformer-model-transformed", G_CALLBACK(on_xsl_transform), NULL);
	ui_widget_modify_font_from_string(GTK_WIDGET(tools->navigator->navigator), geany->interface_prefs->tagbar_font);

	return tools;
}

xmlTools *
xtools_new_blank()
{
	xmlTools * tools = g_malloc(sizeof(* tools));

	tools->navigator = xml_navigator_new();
	gtk_widget_set_name(tools->navigator, "XML Navigator");
	gtk_widget_set_sensitive(tools->navigator, FALSE);
	
	tools->explorer = xpath_explorer_new();
	//xpath_explorer_set_model(tools->explorer, NULL);
	gtk_widget_set_name(tools->explorer, "XPath Explorer");
	gtk_widget_set_sensitive(tools->explorer, FALSE);
	
	tools->transformer = xslt_transformer_new();
	gtk_widget_set_name(tools->transformer, "XSLT Transformer");
	gtk_widget_set_sensitive(tools->transformer, FALSE);

	tools->model = NULL;

	return tools;

}

static void
xtools_remove_child( GtkWidget * child, GtkWidget * parent)
{
	gtk_object_ref(child);
	gtk_container_remove(parent, child);
}

static void
xtools_show(xmlTools * tools) {
	
	GtkWidget * view;
	GtkWidget * panel[XTOOLS_COUNT];
	gint i;

	panel[XTOOLS_NAVIGATOR] = tools->navigator;
	panel[XTOOLS_EXPLORER] = tools->explorer;
	panel[XTOOLS_TRANSFORMER] = tools->transformer;
	
	for( i = 0; i < XTOOLS_COUNT; ++i) {
		view = xtools_views[i];
		gtk_container_foreach(view, xtools_remove_child, view);
		gtk_widget_show_all(panel[i]);
		gtk_box_pack_start(view, panel[i], TRUE, TRUE, 0);
	}
	
}

static gboolean
on_document_activate(	G_GNUC_UNUSED GObject *object,
						GeanyDocument *doc,
						gpointer data)
{
	xmlTools *tools;

	if(doc->file_type->id == GEANY_FILETYPES_XML) {
		tools = g_hash_table_lookup(xtools, doc);
		if(tools == NULL) {
			tools = xtools_new(doc);
			g_hash_table_insert(xtools, doc, tools);
		}
	} else {
		tools = tools_blank;
	}
		
	xtools_show(tools);	
	
	return TRUE;
}

static void on_document_close(G_GNUC_UNUSED GObject *object,
								GeanyDocument *doc)
{
	xmlTools	*tools;

	if(doc == NULL)
		return FALSE;
		
	if(doc == document_get_current()) {
		xtools_show(tools_blank);
	}
	
	if(doc->file_type->id == GEANY_FILETYPES_XML) {
		tools = g_hash_table_lookup(xtools, doc);
		if(tools != NULL) {
			g_hash_table_remove(xtools, doc);
		}
	}
}



static void
message_logger (const gchar *log_domain,
                GLogLevelFlags log_level,
                const gchar *message,
                gpointer user_data)
{
	int error_colour;

	//msgwin_clear_tab(MSG_MESSAGE);
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

void xtools_destroy (xmlTools * tool) {
	gtk_widget_destroy(tool->navigator);
	gtk_widget_destroy(tool->explorer);
	gtk_widget_destroy(tool->transformer);
	gtk_widget_destroy(tool->model);
	g_free(tool);
}

/* Plugin initialisation */
void
plugin_init(GeanyData *data)
{
	gint i;
	GtkWidget *panel[XTOOLS_COUNT];

	LIBXML_TEST_VERSION

	xml_icon_factory_new();

	xtools = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)xtools_destroy);

	tools_blank = xtools_new_blank();

	xtools_geany_pages[XTOOLS_NAVIGATOR] = geany->main_widgets->sidebar_notebook;
	xtools_geany_pages[XTOOLS_EXPLORER] = geany->main_widgets->message_window_notebook;
	xtools_geany_pages[XTOOLS_TRANSFORMER] = geany->main_widgets->message_window_notebook;

	panel[XTOOLS_NAVIGATOR] = tools_blank->navigator;
	panel[XTOOLS_EXPLORER] = tools_blank->explorer;
	panel[XTOOLS_TRANSFORMER] = tools_blank->transformer;


	for(i = 0; i < XTOOLS_COUNT; ++i) {
		//panel = *tools_blank;
		xtools_views[i] = gtk_vbox_new(FALSE,0);
		gtk_box_pack_start(xtools_views[i], panel[i], TRUE, TRUE, 0);
		gtk_widget_show_all(GTK_WIDGET(xtools_views[i]));
		xtools_pages[i] = gtk_notebook_append_page(	GTK_NOTEBOOK(xtools_geany_pages[i]),
											GTK_WIDGET(xtools_views[i]),
											gtk_label_new(_(gtk_widget_get_name(panel[i]))));
	}

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
		(GCallback)&on_document_new, NULL);

 
	plugin_signal_connect(geany_plugin, NULL, "document-reload", TRUE,
		(GCallback)&on_document_close, NULL);

*/
	plugin_module_make_resident(geany_plugin);
}

/* Plugin cleanup */
void plugin_cleanup(void)
{
	gint i;
	
	msgwin_clear_tab(MSG_MESSAGE);

	for(i = 0; i < XTOOLS_COUNT; ++i) {
		gtk_notebook_remove_page(GTK_NOTEBOOK(xtools_geany_pages[i]),xtools_pages[i]);
	}

	xtools_destroy(tools_blank);
	g_hash_table_destroy(xtools);
	
}

GtkWidget *plugin_configure(GtkDialog *dialog)
{
	GtkWidget *vbox, *show_toolbar;
	vbox = gtk_vbox_new(FALSE, 6);
	
	gtk_box_pack_start(GTK_BOX(vbox), show_toolbar, FALSE, FALSE, 3);
	
	return vbox;
}
