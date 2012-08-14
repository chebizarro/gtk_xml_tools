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

#include "xmlmodel.h"

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
static GtkWidget	*tools_view_box, *scrolled_window, *files_view_vbox;
static XmlList	*xmllist;


typedef struct
{
	/* general settings */
	gchar *config_file;
	gboolean enable_toolbar;

} XfilesInfo;
static XfilesInfo *xinfo = NULL;

/* Display plugin help */
void plugin_help(void) {
}

static GdkPixbuf *
utils_pixbuf_from_stock(const gchar *stock_id)
{
	GtkIconSet *icon_set;

	icon_set = gtk_icon_factory_lookup_default(stock_id);

	if (icon_set)
		return gtk_icon_set_render_icon(icon_set, gtk_widget_get_default_style(),
										gtk_widget_get_default_direction(),
										GTK_STATE_NORMAL, GTK_ICON_SIZE_MENU, NULL, NULL);
	return NULL;
}


static gboolean change_focus_to_editor(GeanyDocument *doc)
{
	/* idx might not be valid e.g. if user closed a tab whilst Geany is opening files */
	if (DOC_VALID(doc))
	{
		GtkWidget *focusw = gtk_window_get_focus(
				GTK_WINDOW(geany->main_widgets->window));
		GtkWidget *sci = GTK_WIDGET(doc->editor->sci);
		gtk_widget_grab_focus(sci);
	}
	return FALSE;
}

static gboolean on_tree_list_selection_changed(
		GtkTreeSelection *selection)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint line = 0;

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, 3, &line, -1);
		if (line > 0)
		{
			GeanyDocument *doc = document_get_current();
			if(doc) {
				navqueue_goto_line(doc, doc, line);
				change_focus_to_editor(doc);
			}
		}
	}
	return FALSE;
}

static gboolean on_tree_view_button_press_event(GtkWidget *widget,
		GdkEventButton *event, gpointer data)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));

	if (event->type == GDK_2BUTTON_PRESS)
	{	/* double click on parent node(section) expands/collapses it */
		GtkTreeModel *model;
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected(selection, &model, &iter))
		{
			on_tree_list_selection_changed(selection);
			/*
			if (gtk_tree_model_iter_has_child(model, &iter))
			{
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(widget), path))
					gtk_tree_view_collapse_row(GTK_TREE_VIEW(widget), path);
				else
					gtk_tree_view_expand_row(GTK_TREE_VIEW(widget), path, FALSE);
				
				gtk_tree_path_free(path);
				return TRUE;
			}
			*/
		}
	}
	else if (event->button == 1)
	{	/* allow reclicking of treeview item */
		/* delay the query of selection state because this callback is executed before GTK
		 * changes the selection (g_signal_connect_after would be better but it doesn't work) */
		/*g_idle_add((GSourceFunc) xui_on_tree_list_selection_changed, selection);*/
	}
	else if (event->button == 3)
	{
		/*gtk_menu_popup(GTK_MENU(tv.popup_taglist), NULL, NULL, NULL, NULL,
										event->button, event->time);*/
		return TRUE;
	}
	return FALSE;
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
	xml_list_add_file(xmllist,file_path);
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
	GtkTreeViewColumn	*col;
	GtkWidget			*view;
	GtkCellRenderer		*renderer;
 
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
 
	g_object_unref(list); /* destroy store automatically with view */

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
                                 
	renderer = gtk_cell_renderer_text_new();
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

	g_signal_connect(view, "button-press-event",
			G_CALLBACK(on_tree_view_button_press_event), NULL);

	return view;
}

static void on_document_activate(G_GNUC_UNUSED GObject *object,
								GeanyDocument *doc, XmlList * data)
{
	msgwin_msg_add(COLOR_RED,-1, NULL,"on_document_activate");
	GeanyDocument *odoc;
	odoc = document_get_current();
	if(odoc != NULL)
		xml_list_add_file(xmllist, odoc->real_path);
	
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

	/* Configuration 
	CONFIG_FILE = g_strconcat(geany->app->configdir, G_DIR_SEPARATOR_S, "plugins", G_DIR_SEPARATOR_S,
		"xmltools", G_DIR_SEPARATOR_S, "xmltools.conf", NULL);
*/

	/* Create the Main Toolbar*/
	GtkWidget *toolbar, *view;
	tools_view_box = gtk_vbox_new(FALSE, 0);
	toolbar = make_toolbar();
	gtk_box_pack_start(GTK_BOX(tools_view_box), toolbar, FALSE, FALSE, 0);

	/* create a new scrolled window. */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
									GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (tools_view_box), scrolled_window, TRUE, TRUE, 0);
	files_view_vbox = gtk_vbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), files_view_vbox);

	xmllist = xml_list_new();

	g_signal_connect(xmllist, "row-has-child-toggled",
			G_CALLBACK(on_tree_view_refresh), NULL);

	
	/* Create filter and set visible column */
	GtkTreeModel *filter;
	filter = gtk_tree_model_filter_new( GTK_TREE_MODEL( xmllist ), NULL );
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER( filter ), 4 );
	g_object_unref( G_OBJECT( xmllist ) );

	view = create_view(filter); 
	//view = create_view(xmllist);
	
	gtk_container_add(GTK_CONTAINER(files_view_vbox), view);

	gtk_widget_show_all(tools_view_box);

	page_number = gtk_notebook_append_page(GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook), tools_view_box, gtk_label_new(_("XML/XSLT Tools")));


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

/* Todo: plugin configuration */

static void x_configure_response_cb(GtkDialog *dialog, gint response, gpointer user_data)
{
	if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
	{
		GKeyFile *config = g_key_file_new();
		gchar *data;
		gchar *config_dir = g_path_get_dirname(xinfo->config_file);

		xinfo->enable_toolbar = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
			g_object_get_data(G_OBJECT(dialog), "check_doclist"))));

		g_key_file_load_from_file(config, xinfo->config_file, G_KEY_FILE_NONE, NULL);
		g_key_file_set_boolean(config, "addons", "show_toolbar", xinfo->enable_toolbar);

		if (! g_file_test(config_dir, G_FILE_TEST_IS_DIR) && utils_mkdir(config_dir, TRUE) != 0)
		{
			dialogs_show_msgbox(GTK_MESSAGE_ERROR,
				_("Plugin configuration directory could not be created."));
		}
		else
		{
			/* write config to file */
			data = g_key_file_to_data(config, NULL, NULL);
			utils_write_file(xinfo->config_file, data);
			g_free(data);
		}
		g_free(config_dir);
		g_key_file_free(config);
	}
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
	
	g_signal_connect(dialog, "response", G_CALLBACK(x_configure_response_cb), NULL);
	
	return vbox;
}
