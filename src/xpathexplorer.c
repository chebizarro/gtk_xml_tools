/*
 *	Custom Widget Template
 *
 */

#include "xmltools.h"
#include "xpathexplorer.h"

enum {
	XPATH_MODEL_CHANGED,
	XPATH_ROW_ACTIVATED_SIGNAL,
	XPATH_ROW_COLLAPSED_SIGNAL,
	XPATH_ROW_EXPANDED_SIGNAL,
	XPATH_BUTTON_PRESS_EVENT,
	LAST_SIGNAL
};

static guint xpath_explorer_signals[LAST_SIGNAL] = { 0 };

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

static gboolean	xpath_explorer_row_collapsed(GtkTreeView *tree_view,
							GtkTreeIter *iter,
							GtkTreePath *path,
							XpathExplorer *ttt);

static gboolean	xpath_explorer_row_expanded(	GtkTreeView *tree_view,
							GtkTreeIter *iter,
							GtkTreePath *path,
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
	xpath_explorer_signals[XPATH_ROW_ACTIVATED_SIGNAL] = g_signal_new ("xpath-row-activated",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XpathExplorerClass, xpath_row_activated),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

	xpath_explorer_signals[XPATH_ROW_COLLAPSED_SIGNAL] = g_signal_new ("xpath-row-collapsed",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XpathExplorerClass, xpath_row_collapsed),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

	xpath_explorer_signals[XPATH_ROW_EXPANDED_SIGNAL] = g_signal_new ("xpath-row-expanded",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XpathExplorerClass, xpath_row_expanded),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);


	xpath_explorer_signals[XPATH_MODEL_CHANGED] = g_signal_new ("xpath-model-changed",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XpathExplorerClass, xpath_model_changed),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);
											
}



static void
xpath_explorer_init (XpathExplorer *ttt)
{
	GtkWidget	*scrolled_window;
	GtkWidget 	*vbox;
	GtkWidget	*hbox;
	GtkLabel	*label;
	GtkFrame	*frame;
	

	label = gtk_label_new(_("Xpath:"));

	ttt->entry = make_xpath_entry(ttt);
	make_xpath_completion(ttt);
 
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), FALSE, FALSE, 5);
 	gtk_box_pack_start(GTK_BOX(hbox), ttt->entry, TRUE, TRUE, 0);

    scrolled_window = make_scrolled_window();

    ttt->results = make_results_view(ttt);

    vbox = gtk_vbox_new(TRUE, 3);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vbox);
	gtk_container_add(GTK_CONTAINER(vbox), ttt->results);

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
	gtk_entry_set_icon_from_stock (	entry,
									GTK_ENTRY_ICON_PRIMARY,
									GTK_STOCK_FIND);	
	gtk_entry_set_icon_from_stock (GTK_ENTRY (entry),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   GTK_STOCK_CLEAR);

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

	//g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(xpath_update_completion), ttt->entry);
	g_signal_connect(ttt, "xpath-model-changed", G_CALLBACK(xpath_update_completion), ttt->entry);

}

static GtkWidget *
make_results_view (XpathExplorer * ttt)
{
	g_return_val_if_fail(ttt != NULL, NULL);
	
	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer, *icon_renderer;
	GtkWidget			*view;

	xmlTreeModel			*dummymodel;
	
	dummymodel = xml_tree_model_new();

	view = gtk_tree_view_new_with_model(dummymodel);
	
	//view = gtk_tree_view_new();
	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(view), TRUE);
	
	//g_signal_connect(view, "button-press-event",
	//		G_CALLBACK(xpath_explorer_button_press_event), ttt);

	g_signal_connect(view, "row-activated",
			G_CALLBACK(xpath_explorer_row_activated), ttt);

	g_signal_connect(view, "row-expanded",
			G_CALLBACK(xpath_explorer_row_expanded), ttt);

	g_signal_connect(view, "row-collapsed",
			G_CALLBACK(xpath_explorer_row_collapsed), ttt);

 	col = gtk_tree_view_column_new();
 	gtk_tree_view_column_set_title (col, "Name");
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
	gtk_tree_view_column_set_title (col, "Xpath");
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_LINE);
	gtk_tree_view_column_set_title (col, "Line");
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer,"ellipsize", PANGO_ELLIPSIZE_END, NULL);
	g_object_set(renderer,"single-paragraph-mode", TRUE, NULL);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_CONTENT);
	gtk_tree_view_column_set_title (col, "Value");
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
xpath_explorer_button_press_event(	GtkWidget *widget,
									GdkEventButton *event,
									XpathExplorer *ttt)
{
	//g_signal_emit(ttt, xpath_explorer_signals[XPATH_BUTTON_PRESS_EVENT],0,event);
	return FALSE;
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
xpath_explorer_row_collapsed(GtkTreeView *tree_view,
							GtkTreeIter *iter,
							GtkTreePath *path,
							XpathExplorer *ttt)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	g_signal_emit(ttt, xpath_explorer_signals[XPATH_ROW_COLLAPSED_SIGNAL],0,selection);
	return FALSE;
}

static gboolean
xpath_explorer_row_expanded(	GtkTreeView *tree_view,
							GtkTreeIter *iter,
							GtkTreePath *path,
							XpathExplorer *ttt)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));	
	g_signal_emit(ttt, xpath_explorer_signals[XPATH_ROW_EXPANDED_SIGNAL],0,selection);
	return FALSE;
}
