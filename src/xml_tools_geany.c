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

static gint		page_number = 0;
static XmlList	*xmllist;
static GtkWidget	*navigator;


/* Display plugin help */
void plugin_help(void) {
}



static gboolean change_focus_to_editor(GeanyDocument *doc)
{
	/* idx might not be valid e.g. if user closed a tab whilst Geany is opening files */
	if (DOC_VALID(doc))
	{
		gtk_window_get_focus(GTK_WINDOW(geany->main_widgets->window));
		GtkWidget *sci = GTK_WIDGET(doc->editor->sci);
		gtk_widget_grab_focus(sci);
	}
	return FALSE;
}


static void on_document_activate(G_GNUC_UNUSED GObject *object,
								GeanyDocument *doc, XmlList * data)
{
	GeanyDocument *odoc;
	odoc = document_get_current();
	if(odoc != NULL) {
		xml_list_add_file(xmllist, odoc->real_path);
		xml_list_set_visible (xmllist, XML_DTD_NODE, TRUE);
		xml_list_set_visible (xmllist, XML_ATTRIBUTE_NODE, TRUE);
		
		/* Create filter with virtual root set to second branch*/
		GtkTreePath *virtual_root;
		virtual_root = gtk_tree_path_new_from_string("0");		
		/* Create filter and set visible column */
		GtkTreeModel *filter;
		filter = gtk_tree_model_filter_new( GTK_TREE_MODEL( xmllist ), virtual_root );
		gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER( filter ), XML_LIST_COL_VISIBLE );
		
		xml_navigator_set_model(XML_NAVIGATOR(navigator), XML_LIST(filter));
	}
}

static void on_document_close(G_GNUC_UNUSED GObject *object,
								GeanyDocument *doc)
{
	msgwin_msg_add(COLOR_RED,-1, NULL,"on_document_close");
	GeanyDocument *odoc;
	odoc = document_get_current();
	if(odoc != NULL) {
		xml_list_add_file(xmllist, odoc->real_path);
	} else {
		xml_list_add_file(xmllist, NULL);
	}
	xml_navigator_set_model(XML_NAVIGATOR(navigator),XML_LIST(xmllist));
}

void	on_tree_view_refresh(	GtkTreeModel *tree_model,
								GtkTreePath  *path,
								GtkTreeIter  *iter,
								gpointer      user_data)
{
	//gtk_tree_view_expand_row();
}

/* Plugin initialisation */
void plugin_init(GeanyData *data)
{
	LIBXML_TEST_VERSION

	xml_icon_factory_new();
	
	xmllist = xml_list_new();

	g_signal_connect(xmllist, "row-has-child-toggled",
			G_CALLBACK(on_tree_view_refresh), NULL);

	navigator = xml_navigator_new();

	/*
	g_signal_connect(navigator, "navigator-selected",
			G_CALLBACK(on_tree_view_refresh), NULL);
	*/
	
	gtk_widget_show_all(navigator);

	page_number = gtk_notebook_append_page(GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook), navigator, gtk_label_new(_("XML/XSLT Tools")));

	plugin_signal_connect(geany_plugin, NULL, "document-activate", FALSE,
		(GCallback)&on_document_activate, NULL);

	plugin_signal_connect(geany_plugin, NULL, "document-save", TRUE,
		(GCallback)&on_document_activate, NULL);

	plugin_signal_connect(geany_plugin, NULL, "document-open", TRUE,
		(GCallback)&on_document_activate, NULL);
/*
	plugin_signal_connect(geany_plugin, NULL, "document-close", TRUE,
		(GCallback)&on_document_close, NULL);

	plugin_signal_connect(geany_plugin, NULL, "document-filetype-set", TRUE,
		(GCallback)&on_document_activate, NULL);
*/
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
/*
	case XML_ELEMENT_NODE:
	case XML_ATTRIBUTE_NODE:
	case XML_TEXT_NODE:
	case XML_CDATA_SECTION_NODE:
	case XML_ENTITY_REF_NODE:
	case XML_ENTITY_NODE:
	case XML_PI_NODE:
	case XML_COMMENT_NODE:
	case XML_DOCUMENT_NODE:
	case XML_DOCUMENT_TYPE_NODE:
	case XML_DOCUMENT_FRAG_NODE:
	case XML_NOTATION_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_DTD_NODE:
	case XML_ELEMENT_DECL:
	case XML_ATTRIBUTE_DECL:
	case XML_ENTITY_DECL:
	case XML_NAMESPACE_DECL:
	case XML_XINCLUDE_START:
	case XML_XINCLUDE_END:
	case XML_DOCB_DOCUMENT_NODE:
*/

	gtk_box_pack_start(GTK_BOX(vbox), show_toolbar, FALSE, FALSE, 3);
	
	return vbox;
}
