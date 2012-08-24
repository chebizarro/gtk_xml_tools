/*
 *	Custom Widget Template
 *
 */

#include "xmltools.h"
#include "xmlnavigator.h"

enum {
	ROW_ACTIVATED_SIGNAL,
	ROW_COLLAPSED_SIGNAL,
	ROW_EXPANDED_SIGNAL,
	BUTTON_PRESS_EVENT,
	MODEL_CHANGED,
	LAST_SIGNAL
};

static void xml_navigator_class_init		(XmlNavigatorClass 	*klass);
static void xml_navigator_init				(XmlNavigator		*ttt);

static GtkWidget * make_navigator_view(XmlNavigator *ttt);
static GtkWidget * make_scrolled_window(void);
static GtkWidget * make_toolbar(XmlNavigator * ttt);
static GtkWidget * make_xpath_entry(XmlNavigator *ttt, XmlList * xmllist);
static GtkTreeModel * create_filter(XmlList * xmllist);

static guint xml_navigator_signals[LAST_SIGNAL] = { 0 };

GType
xml_navigator_get_type (void)
{
	static GType ttt_type = 0;
	if (!ttt_type)
		{
			const GTypeInfo ttt_info =
			{
				sizeof (XmlNavigatorClass),
				NULL, /* base_init */
				NULL, /* base_finalize */
				(GClassInitFunc) xml_navigator_class_init,
				NULL, /* class_finalize */
				NULL, /* class_data */
				sizeof (XmlNavigator),
				0,
				(GInstanceInitFunc) xml_navigator_init,
			};
			ttt_type = g_type_register_static (GTK_TYPE_BOX, "XmlNavigator", &ttt_info, 0);
		}
	return ttt_type;
}

static void
xml_navigator_class_init (XmlNavigatorClass *klass)
{
	xml_navigator_signals[ROW_ACTIVATED_SIGNAL] = g_signal_new ("xml-row-activated",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XmlNavigatorClass, xml_row_activated),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

	xml_navigator_signals[ROW_COLLAPSED_SIGNAL] = g_signal_new ("xml-row-collapsed",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XmlNavigatorClass, xml_row_collapsed),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

	xml_navigator_signals[ROW_EXPANDED_SIGNAL] = g_signal_new ("xml-row-expanded",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XmlNavigatorClass, xml_row_expanded),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

	xml_navigator_signals[MODEL_CHANGED] = g_signal_new ("xml-model-changed",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XmlNavigatorClass, xml_model_changed),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);


}



static void
xml_navigator_init (XmlNavigator *ttt)
{
	ttt->xpath_view = make_xpath_entry(ttt, NULL);
    gtk_box_pack_start(GTK_BOX(ttt), ttt->xpath_view, TRUE, FALSE, 0);


	ttt->toolbar = make_toolbar(ttt);
	gtk_box_pack_start(GTK_BOX(ttt), ttt->toolbar, FALSE, FALSE, 0);

    ttt->scrolled_window = make_scrolled_window();
    gtk_box_pack_start (GTK_BOX(ttt), ttt->scrolled_window, TRUE, TRUE, 0);

	ttt->navigator_view_vbox = gtk_vbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ttt->scrolled_window), ttt->navigator_view_vbox);
	
	ttt->navigator_view = make_navigator_view(ttt);
	gtk_container_add(GTK_CONTAINER(ttt->navigator_view_vbox), ttt->navigator_view);	

	
}

GtkWidget*
xml_navigator_new ()
{
	return GTK_WIDGET (g_object_new (xml_navigator_get_type (), "orientation", GTK_ORIENTATION_VERTICAL, NULL));
}

void
xml_navigator_set_model(XmlNavigator *ttt, XmlList * xmllist)
{
	ttt->model = xmllist;
	ttt->filter = create_filter(xmllist);
	gtk_tree_view_set_model(ttt->navigator_view, GTK_TREE_MODEL(ttt->filter));	
	g_signal_emit(ttt, xml_navigator_signals[MODEL_CHANGED],0,xmllist);

}

static GtkTreeModel * create_filter(XmlList * xmllist) {
	/* Create filter with virtual root set to second branch*/
	GtkTreePath *virtual_root;
	virtual_root = gtk_tree_path_new_from_string("0");		
	/* Create filter and set visible column */
	GtkTreeModel *filter;
	filter = gtk_tree_model_filter_new( GTK_TREE_MODEL( xmllist ), virtual_root );
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER( filter ), XML_LIST_COL_VISIBLE );
	return filter;
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
toolbar_button_toggle(GtkToggleToolButton *button,
                      XmlNavigator * ttt)
{
	gboolean toggle;
	int i = 0;
	
	toggle = (gtk_toggle_tool_button_get_active(button)) ? TRUE : FALSE;
	
	while(ttt->toolbar_buttons[i].button != button) {
		++i;
	}
	
	xml_list_set_visible (ttt->model, ttt->toolbar_buttons[i].type, toggle);
	gtk_tree_model_filter_refilter(ttt->filter);
	return TRUE;
}

static gboolean
xml_toggle_visible(XmlNavigator *ttt,
                   XmlList * xmllist,
                   XmlToolBarButton *button)
{
	button->model = ttt->model;	
	gtk_widget_set_sensitive (button->button, TRUE);
	gtk_toggle_tool_button_set_active(button->button, xml_list_get_visible (xmllist, button->type));
	g_signal_connect(button->button, "toggled", G_CALLBACK(toolbar_button_toggle), ttt);
	return TRUE;
}

static
make_toolbar_button(XmlNavigator	 *ttt,
					xmlElementType	 type,
    	            XmlToolBarButton * button)
{
	button->button = gtk_toggle_tool_button_new_from_stock(XmlNodes[type].stock_id);
	gtk_widget_set_sensitive (button->button, FALSE);
	button->type = type;
	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(xml_toggle_visible), button);
}

static GtkWidget *
make_toolbar(XmlNavigator * ttt)
{
	GtkWidget 	*toolbar;

	make_toolbar_button(ttt,XML_ATTRIBUTE_NODE, &ttt->toolbar_buttons[0]);
	make_toolbar_button(ttt,XML_DTD_NODE, &ttt->toolbar_buttons[1]);
	make_toolbar_button(ttt,XML_COMMENT_NODE, &ttt->toolbar_buttons[2]);
	make_toolbar_button(ttt,XML_PI_NODE, &ttt->toolbar_buttons[3]);
	make_toolbar_button(ttt,XML_TEXT_NODE, &ttt->toolbar_buttons[4]);
	make_toolbar_button(ttt,XML_CDATA_SECTION_NODE, &ttt->toolbar_buttons[5]);

	/* Create the toolbar object and initialise it*/
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	for(int i=0;i<6;++i)
	{
		gtk_container_add(GTK_CONTAINER(toolbar), ttt->toolbar_buttons[i].button);
	}


	return toolbar;
}

static gboolean
xml_navigator_button_press_event(	GtkWidget *widget,
									GdkEventButton *event,
									XmlNavigator *ttt)
{
	g_signal_emit(ttt, xml_navigator_signals[BUTTON_PRESS_EVENT],0,event);
	return FALSE;
}

static gboolean
xml_navigator_row_activated(GtkTreeView	*tree_view,
							GtkTreePath	*path,
                            GtkTreeViewColumn *column,
                            XmlNavigator *ttt)
{
	GtkTreeSelection *selection;
	gboolean return_value = FALSE;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));

	g_signal_emit(ttt, xml_navigator_signals[ROW_ACTIVATED_SIGNAL],0,selection, &return_value);
	return return_value;
}

static gboolean
xml_navigator_row_collapsed(GtkTreeView *tree_view,
							GtkTreeIter *iter,
							GtkTreePath *path,
							XmlNavigator *ttt)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	g_signal_emit(ttt, xml_navigator_signals[ROW_COLLAPSED_SIGNAL],0,selection);
	return FALSE;
}

static gboolean
xml_navigator_row_expanded(	GtkTreeView *tree_view,
							GtkTreeIter *iter,
							GtkTreePath *path,
							XmlNavigator *ttt)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));	
	g_signal_emit(ttt, xml_navigator_signals[ROW_EXPANDED_SIGNAL],0,selection);
	return FALSE;
}


static GtkWidget *
make_navigator_view (XmlNavigator * ttt)
{
	g_return_if_fail(ttt != NULL);
	
	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer, *icon_renderer;
	GtkWidget			*view;
 
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ttt->filter));
 	g_object_unref(ttt->filter); /* destroy store automatically with view */

	//g_signal_connect(view, "button-press-event",
	//		G_CALLBACK(xml_navigator_button_press_event), ttt);

	g_signal_connect(view, "row-activated",
			G_CALLBACK(xml_navigator_row_activated), ttt);

	g_signal_connect(view, "row-expanded",
			G_CALLBACK(xml_navigator_row_expanded), ttt);

	g_signal_connect(view, "row-collapsed",
			G_CALLBACK(xml_navigator_row_collapsed), ttt);

 	col = gtk_tree_view_column_new();
 
 	gtk_tree_view_column_set_title (col, "Name");
	gtk_tree_view_column_set_resizable(col, TRUE);
 
    icon_renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(col, icon_renderer, FALSE);
    gtk_tree_view_column_set_attributes(col, icon_renderer,
                                        "stock-id", XML_LIST_COL_TYPE,
                                        NULL);

 	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_LIST_COL_NAME);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);
 
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer,"ellipsize", PANGO_ELLIPSIZE_END, NULL);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_LIST_COL_CONTENT);
	gtk_tree_view_column_set_title (col, "Value");
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(view), TRUE);


	return view;
}

static void
run_xpath(GtkButton * runxpath, XmlNavigator *ttt ) {
	GtkListStore * xpath_result;
	gchar * xpath = gtk_entry_get_text(GTK_ENTRY(ttt->xpath_entry));	
	xpath_result = xml_get_xpath_results(ttt->model, xpath);
	gtk_tree_view_set_model(ttt->xpath_results, xpath_result);
}

static GtkWidget *
make_xpath_entry (XmlNavigator *ttt, XmlList *xmllist) {
	
	GtkWidget * xpath_frame;
	xpath_frame = gtk_vbox_new(FALSE, 3);
	//gtk_frame_set_shadow_type(GTK_FRAME(xpath_frame), GTK_SHADOW_IN);
	//gtk_frame_set_label_widget(GTK_FRAME(xpath_frame),frame_label_widget);

	GtkWidget *hbox;
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
		
	/* Label and entry */
	GtkWidget *xlabel;
	xlabel = gtk_label_new("XPath:");
	gtk_box_pack_start(GTK_BOX(hbox), xlabel,FALSE, TRUE, 0);
	ttt->xpath_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), ttt->xpath_entry, TRUE, TRUE, 0);
	
	/* Run button */
	GtkWidget * runxpath;
	runxpath = gtk_tool_button_new_from_stock("gtk-find");
	g_signal_connect(runxpath, "clicked", G_CALLBACK(run_xpath), ttt);
    gtk_box_pack_start(GTK_BOX(hbox), runxpath, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(xpath_frame), hbox);

	/* The results expander */
	GtkWidget *results_expander; 
	results_expander = gtk_expander_new("Results");
	gtk_expander_set_expanded(GTK_EXPANDER(results_expander), TRUE);
	//gtk_container_add(GTK_CONTAINER(xpath_frame), results_expander);

	GtkWidget *scrolled_window;
	scrolled_window = make_scrolled_window();
    gtk_box_pack_start (xpath_frame, scrolled_window, TRUE, TRUE, 0);
	//gtk_container_add(GTK_CONTAINER(results_expander), scrolled_window);

	GtkWidget * view_vbox = gtk_vbox_new(TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), view_vbox);

	ttt->xpath_results = make_navigator_view(ttt);
	gtk_container_add(GTK_CONTAINER(view_vbox), ttt->xpath_results);	

	return xpath_frame;
}
