/*
 *	Custom Widget Template
 *
 */

#include "xmltools.h"
#include "xpathexplorer.h"

enum {
	XPATH_MODEL_CHANGED,
	XPATH_ROW_ACTIVATED_SIGNAL,
	XPATH_BUTTON_PRESS_EVENT,
	XPATH_SHOW_IN_NAV_SIGNAL,
	XPATH_LAST_SIGNAL
};

static guint xpath_explorer_signals[XPATH_LAST_SIGNAL] = { 0 };

static void xpath_explorer_class_init		(XpathExplorerClass *klass);
static void xpath_explorer_init			(XpathExplorer		*ttt);

static GtkWidget	*make_xpath_entry		(XpathExplorer 	*ttt);
static gboolean	xpath_update_completion	(XpathExplorer 	*ttt,
											 xmlTreeModel	*xmltreemodel,
											 GtkEntry		*xpath_entry);
static gboolean	xpath_match_select		(GtkEntryCompletion	*widget,
											 GtkTreeModel		*model,
											 GtkTreeIter		*iter,
											 XpathExplorer		*ttt);

static gboolean	xpath_icon_press_cb		(GtkEntry       *entry,
											 gint            position,
											 GdkEventButton *event,
											 XpathExplorer *ttt);

static void		xpath_activate_cb 		(GtkEntry  *entry,
											 XpathExplorer *ttt);

static GtkWidget *	make_scrolled_window	(void);

static GtkWidget *	make_results_view		(XpathExplorer * ttt);

void	xpath_insert_text_handler					(GtkEditable *editable,
											gchar       *new_text,
											gint         new_text_length,
											gpointer     position,
											XpathExplorer *ttt);

void	xpath_delete_text_handler					(GtkEditable *editable,
											 gint         start_pos,
											 gint         end_pos,
											 XpathExplorer *ttt);

static void		make_xpath_completion	(XpathExplorer *ttt);

static gboolean	xpath_explorer_button_press_event(	GtkWidget *widget,
									GdkEventButton *event,
									XpathExplorer *ttt);

static gboolean	xpath_explorer_row_activated(GtkTreeView	*tree_view,
							GtkTreePath	*path,
                            GtkTreeViewColumn *column,
                            XpathExplorer *ttt);

											
GType
xpath_explorer_get_type (void)
{
	static GType ttt_type = 0;
	if (!ttt_type)
		{
			const GTypeInfo ttt_info =
			{
				sizeof (XpathExplorerClass),
				NULL, /* base_init */
				NULL, /* base_finalize */
				(GClassInitFunc) xpath_explorer_class_init,
				NULL, /* class_finalize */
				NULL, /* class_data */
				sizeof (XpathExplorer),
				0,
				(GInstanceInitFunc) xpath_explorer_init,
			};
			ttt_type = g_type_register_static (GTK_TYPE_BOX, "XpathExplorer", &ttt_info, 0);
		}
	return ttt_type;
}

static void
xpath_explorer_class_init (XpathExplorerClass *klass)
{
	xpath_explorer_signals[XPATH_MODEL_CHANGED] = g_signal_new ("xpath-model-changed",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XpathExplorerClass, xpath_model_changed),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

	xpath_explorer_signals[XPATH_ROW_ACTIVATED_SIGNAL] = g_signal_new ("xpath-row-activated",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XpathExplorerClass, xpath_row_activated),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);


	xpath_explorer_signals[XPATH_BUTTON_PRESS_EVENT] = g_signal_new ("xpath-button-pressed",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XpathExplorerClass, xpath_button_press),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__VOID,
												G_TYPE_NONE,0 );


	xpath_explorer_signals[XPATH_SHOW_IN_NAV_SIGNAL] = g_signal_new ("xpath-show-in-navigator",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XpathExplorerClass, xpath_show_in_nav),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__VOID,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

											
}

/* Shows the open file dialog and returns the selectd filepath */ 
static gchar *show_open_dialog() {
	GtkWidget *dialog;
	gchar *filename = NULL;
	dialog = gtk_file_chooser_dialog_new(_("Choose File"),
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

static void
xpath_open(GtkWidget *button, XpathExplorer *ttt)
{
	gchar			 *filename = NULL;
    gchar            *status;
    gchar            *text;
	gboolean		 results;
	
	filename = show_open_dialog();
	
	g_return_if_fail(filename != NULL);
	
	results = g_file_get_contents(filename, &text, NULL, NULL);
	
	if(results) {
        gtk_entry_set_text(ttt->entry, text);
	}

    g_free (text);
    g_free (filename);
	
}

static void
xpath_save(GtkWidget *button, XpathExplorer *ttt)
{
	
}


static GtkWidget *
make_xpath_toolbar(XpathExplorer *ttt)
{
	GtkWidget	*toolbar;
	GtkWidget	*button;
	
	toolbar = gtk_toolbar_new();
	
	button = gtk_tool_button_new_from_stock("gtk-open");
	gtk_toolbar_insert(toolbar, button, -1);
	g_signal_connect(button, "clicked", G_CALLBACK(xpath_open), ttt);

	button = gtk_tool_button_new_from_stock("gtk-save");
	gtk_toolbar_insert(toolbar, button, -1);
	g_signal_connect(button, "clicked", G_CALLBACK(xpath_save), ttt);

	return toolbar;
}

static void
xpath_explorer_init (XpathExplorer *ttt)
{
	GtkWidget	*scrolled_window;
	GtkWidget	*hbox;
	GtkLabel	*label;
	GtkFrame	*frame;
	GtkWidget	*toolbar;

	label = gtk_label_new(_("XPath:"));

	ttt->entry = make_xpath_entry(ttt);
	make_xpath_completion(ttt);
 
	toolbar = make_xpath_toolbar(ttt);
 
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), FALSE, FALSE, 6);
 	gtk_box_pack_start(GTK_BOX(hbox), ttt->entry, TRUE, TRUE, 0);
 	//gtk_box_pack_start(GTK_BOX(hbox), toolbar, TRUE, TRUE, 0);

    scrolled_window = make_scrolled_window();
    ttt->results = make_results_view(ttt);
	gtk_container_add(GTK_CONTAINER(scrolled_window), ttt->results);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 6);

	frame = gtk_frame_new(_("Results"));
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

	gtk_box_pack_start(GTK_BOX(ttt), GTK_WIDGET(hbox), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ttt), GTK_WIDGET(frame), TRUE, TRUE, 0);
	
	gtk_container_set_border_width(GTK_CONTAINER(ttt), 6);
	
}

GtkWidget*
xpath_explorer_new ()
{
	return GTK_WIDGET (g_object_new (xpath_explorer_get_type (), "orientation", GTK_ORIENTATION_VERTICAL, NULL));
}


static GtkWidget *
make_scrolled_window(void)
{
	GtkWidget *scroll_win;
	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (scroll_win), 2);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_win),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	return scroll_win;
}

static gboolean
xpath_update_completion(XpathExplorer *ttt,
	                   	xmlTreeModel * xmltreemodel,
						GtkEntry * xpath_entry)
{
	GtkListStore * xpath_result;
	GtkEntryCompletion * completion;
	gchar * xpath;

	if(ttt->model == NULL)
		return FALSE;

	xpath = gtk_entry_get_text(GTK_ENTRY(xpath_entry));	
	completion = gtk_entry_get_completion(xpath_entry);

	GString * xpath_full;
	xpath_full = g_string_new("");
	g_string_printf(xpath_full, "%s/*",xpath);

	xpath_result = xml_tree_model_get_xpath_results(ttt->model, xpath_full->str);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(xpath_result));
	
	g_string_free(xpath_full, FALSE);
	
	return TRUE;
}

static gboolean
xpath_match_select(	GtkEntryCompletion	* widget,
					GtkTreeModel		* model,
					GtkTreeIter			* iter,
					XpathExplorer		* ttt)
{ 
	GtkEntry	* entry;
	gchar	 	* xpath;

	gtk_tree_model_get(model, iter, XML_TREE_MODEL_COL_XPATH, xpath, -1);
	entry = gtk_entry_completion_get_entry (widget);
	
	gtk_entry_set_text(entry, xpath);

	xpath_update_completion(ttt, ttt->model, entry);
	
	g_free(xpath);
	return FALSE;
}

static void
run_xpath(GtkEntry * xpath_entry, XpathExplorer *ttt ) {
	GtkListStore * xpath_result;
	gchar * xpath = gtk_entry_get_text(GTK_ENTRY(xpath_entry));	
	xpath_result = xml_tree_model_get_xpath_results(ttt->model, xpath);
	if(xpath_result != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->results), GTK_TREE_MODEL(xpath_result));
		g_signal_emit(ttt, xpath_explorer_signals[XPATH_MODEL_CHANGED],0,GTK_TREE_MODEL(xpath_result));
	}
}

static void
xpath_activate_cb (GtkEntry  *entry,
             XpathExplorer *ttt)
{
	run_xpath(entry, ttt);
}

static gboolean
xpath_icon_press_cb (GtkEntry       *entry,
               gint            position,
               GdkEventButton *event,
               XpathExplorer *ttt)
{
	if (position == GTK_ENTRY_ICON_PRIMARY) {
		xpath_activate_cb(entry, ttt);
	} else {
		gtk_entry_set_text (entry, "");
		gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->results), NULL);
	}
 }


void
xpath_insert_text_handler(GtkEditable *editable,
					gchar       *new_text,
                    gint         new_text_length,
                    gpointer     position,
                    XpathExplorer *ttt)
{
	if(g_strcmp0(new_text, "/") == 0) {
		xpath_update_completion(ttt, ttt->model, ttt->entry);
	}
}

void
xpath_delete_text_handler (GtkEditable *editable,
                     gint         start_pos,
                     gint         end_pos,
                     XpathExplorer *ttt)
{
	xpath_update_completion(ttt, ttt->model, ttt->entry);
}
 
static GtkWidget *
make_xpath_entry (XpathExplorer *ttt)
{
	GtkEntry *entry;
	
	entry = gtk_entry_new ();

	gtk_entry_set_icon_from_stock (GTK_ENTRY (entry),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   GTK_STOCK_CLEAR);

	gtk_widget_set_tooltip_text(entry, _("Enter an Xpath"));

	g_signal_connect (entry, "icon-press",
                      G_CALLBACK (xpath_icon_press_cb), ttt);
    g_signal_connect (entry, "activate",
                      G_CALLBACK (xpath_activate_cb), ttt);

    g_signal_connect (entry, "delete-text",
                      G_CALLBACK (xpath_delete_text_handler), ttt);

	g_signal_connect(G_OBJECT(entry), "insert-text",
					 G_CALLBACK(xpath_insert_text_handler), ttt);

	return entry;
}

static void
make_xpath_completion (XpathExplorer *ttt)
{
	GtkEntryCompletion *completion;
    completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, XML_TREE_MODEL_COL_XPATH);
	gtk_entry_set_completion(GTK_ENTRY(ttt->entry), completion);
    //g_signal_connect(G_OBJECT (completion), "match-selected", G_CALLBACK (xpath_match_select), ttt);
	g_signal_connect(ttt, "xpath-model-changed", G_CALLBACK(xpath_update_completion), ttt->entry);
}

void
xpath_show_in_navigator (GtkWidget *menuitem, XpathExplorer *ttt)
{
	GtkTreeIter			iter;
	GtkTreeModel		*model = NULL;
	gchar				*xpath = NULL;
	GtkTreeSelection	*selection = NULL;

	selection = gtk_tree_view_get_selection(ttt->results);
	
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, XML_TREE_MODEL_COL_XPATH, &xpath, -1);
		g_signal_emit(ttt, xpath_explorer_signals[XPATH_SHOW_IN_NAV_SIGNAL],0,xpath);
		g_signal_emit(ttt, xpath_explorer_signals[XPATH_ROW_ACTIVATED_SIGNAL],0,selection);

		g_object_unref(model);
		g_free(xpath);
		//gtk_tree_iter_free(&iter);
	}
	
}

void
xpath_results_copy(GtkWidget *menuitem, XpathExplorer *ttt)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar		*xpath;
	gchar		*menu_name = gtk_menu_item_get_label(menuitem);
	gint		column = 0;
	GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ttt->results));


	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		if(g_strcmp0(menu_name, _("Copy XPath"))  == 0)
			column = XML_TREE_MODEL_COL_XPATH;
			
		if(g_strcmp0(menu_name, _("Copy Name"))  == 0)
			column = XML_TREE_MODEL_COL_NAME;

		if(g_strcmp0(menu_name, _("Copy Value"))  == 0)
			column = XML_TREE_MODEL_COL_CONTENT;
			
		gtk_tree_model_get(model, &iter, column, &xpath, -1);

		gtk_clipboard_set_text(clipboard, xpath, -1);
	}
}

void
xpath_view_popup_menu (GtkWidget *treeview, GdkEventButton *event, XpathExplorer *ttt)
{

	GtkWidget *results_menu, *results_item;
	results_menu = gtk_menu_new();

	results_item = gtk_menu_item_new_with_label(_("Copy XPath"));
	g_signal_connect(results_item, "activate", G_CALLBACK(xpath_results_copy), ttt);
	gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_item);

	results_item = gtk_menu_item_new_with_label(_("Copy Name"));
	g_signal_connect(results_item, "activate", G_CALLBACK(xpath_results_copy), ttt);
	gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_item);

	results_item = gtk_menu_item_new_with_label(_("Copy Value"));
	g_signal_connect(results_item, "activate", G_CALLBACK(xpath_results_copy), ttt);
	gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_item);

	results_item = gtk_menu_item_new_with_label(_("Show in XML Navigator"));
	g_signal_connect(results_item, "activate", G_CALLBACK(xpath_show_in_navigator), ttt);
	gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_item);

	gtk_widget_show_all(results_menu);

    gtk_menu_popup(GTK_MENU(results_menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
	
}

gboolean
xpath_view_onPopupMenu (GtkWidget *treeview, XpathExplorer *ttt)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	if (gtk_tree_selection_count_selected_rows(selection)  == 1) {
		xpath_view_popup_menu(treeview, NULL, ttt);
		return TRUE;
	}
    return FALSE;
}

GtkListStore * 
make_results_blank() {
	xmlTreeModel	*dummy = xml_tree_model_new();
	GtkListStore	*blank = gtk_list_store_newv (dummy->n_columns,dummy->column_types);
	//g_free(dummy);
	return blank;
}

static GtkWidget *
make_results_view (XpathExplorer * ttt)
{
	g_return_val_if_fail(ttt != NULL, NULL);
	
	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer, *icon_renderer;
	GtkWidget			*view;

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(make_results_blank()));
	//view = gtk_tree_view_new();
	
	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(view), TRUE);
	
	g_signal_connect(view, "button-press-event",
			G_CALLBACK(xpath_explorer_button_press_event), ttt);
/*
    g_signal_connect(view, "popup-menu",
			G_CALLBACK(xpath_view_onPopupMenu), ttt);*/

	g_signal_connect(view, "row-activated",
			G_CALLBACK(xpath_explorer_row_activated), ttt);

 	col = gtk_tree_view_column_new();
 	gtk_tree_view_column_set_title (col, _("Name"));
	gtk_tree_view_column_set_resizable(col, TRUE);
	 
    icon_renderer = xml_cell_renderer_new();
    gtk_tree_view_column_pack_start(col, icon_renderer, FALSE);
    gtk_tree_view_column_set_attributes(col, icon_renderer,
                                        "node-id", XML_TREE_MODEL_COL_TYPE,
                                        NULL);

 	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_NAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);
 
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_XPATH);
	gtk_tree_view_column_set_title (col, _("XPath"));
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_LINE);
	gtk_tree_view_column_set_title (col, _("Line"));
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer,"ellipsize", PANGO_ELLIPSIZE_END, NULL);
	g_object_set(renderer,"single-paragraph-mode", TRUE, NULL);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_CONTENT);
	gtk_tree_view_column_set_title (col, _("Value"));
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	return view;
}

void
xpath_explorer_refresh_model(xmlTreeModel * xmltreemodel,
							gpointer	data,
							XpathExplorer * ttt)
{
	g_signal_emit(ttt, xpath_explorer_signals[XPATH_MODEL_CHANGED],0,xmltreemodel);
}

void
xpath_explorer_set_model(XpathExplorer *ttt, xmlTreeModel * xmltreemodel)
{
	ttt->model = xmltreemodel;

	g_signal_emit(ttt, xpath_explorer_signals[XPATH_MODEL_CHANGED],0,xmltreemodel);
	g_signal_connect(ttt->model, "xml-tree-model-changed", G_CALLBACK(xpath_explorer_refresh_model), ttt);
}

static gboolean
xpath_explorer_row_activated(GtkTreeView	*tree_view,
							GtkTreePath	*path,
                            GtkTreeViewColumn *column,
                            XpathExplorer *ttt)
{
	GtkTreeSelection *selection;
	gboolean return_value = FALSE;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));

	g_signal_emit(ttt, xpath_explorer_signals[XPATH_ROW_ACTIVATED_SIGNAL],0,selection, &return_value);
	return return_value;
}


static gboolean
xpath_explorer_button_press_event(	GtkWidget *widget,
									GdkEventButton *event,
									XpathExplorer *ttt)
{
	/* single click with the right mouse button? */
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ttt->results));
		if (gtk_tree_selection_count_selected_rows(selection)  == 1) {
			xpath_view_popup_menu(ttt->results, event, ttt);
			//g_signal_emit(ttt, xpath_explorer_signals[XPATH_BUTTON_PRESS_EVENT],0,selection);
			return TRUE; 
		}
	}
	return FALSE;
}
