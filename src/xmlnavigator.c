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
static GtkTreeModelFilter * create_filter(xmlTreeModel * xmltreemodel);

static guint xml_navigator_signals[LAST_SIGNAL] = { 0 };


static void
run_xpath(GtkEntry * xpath_entry, XmlNavigator *ttt ) {
	GtkListStore * xpath_result;
	const gchar * xpath = gtk_entry_get_text(GTK_ENTRY(xpath_entry));	
	xpath_result = xml_get_xpath_results(ttt->model, xpath);
	if(xpath_result != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->navigator_view), GTK_TREE_MODEL(xpath_result));
		g_signal_emit(ttt, xml_navigator_signals[MODEL_CHANGED],0,GTK_TREE_MODEL(xpath_result));
	}
}

static gboolean
toggle_xpath(XmlNavigator *ttt,
             xmlTreeModel * xmltreemodel,
             GtkEntry *entry)
{
	gtk_widget_set_sensitive (GTK_WIDGET(ttt->xpath_entry), TRUE);
	return TRUE;
}


static void
activate_cb (GtkEntry  *entry,
             XmlNavigator *ttt)
{
	run_xpath(entry, ttt);
}

static void
icon_press_cb (GtkEntry       *entry,
               gint            position,
               GdkEventButton *event,
               XmlNavigator *ttt)
{
	if (position == GTK_ENTRY_ICON_PRIMARY) {
		activate_cb(entry, ttt);
	} else {
		gtk_entry_set_text (entry, "");
		gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->navigator_view), GTK_TREE_MODEL(ttt->filter));
		g_signal_emit(ttt, xml_navigator_signals[MODEL_CHANGED],0,XML_TREE_MODEL(ttt->model));
	}
 }

static GtkWidget *
make_xpath_entry (XmlNavigator *ttt)
{
	GtkWidget *entry;
	entry = gtk_entry_new ();


	gtk_entry_set_icon_from_stock (	entry,
									GTK_ENTRY_ICON_PRIMARY,
									GTK_STOCK_FIND);	

	gtk_entry_set_icon_from_stock (GTK_ENTRY (entry),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   GTK_STOCK_CLEAR);

	g_signal_connect (entry, "icon-press",
                      G_CALLBACK (icon_press_cb), ttt);

    g_signal_connect (entry, "activate",
                      G_CALLBACK (activate_cb), ttt);

	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(toggle_xpath), entry);

	//gtk_widget_show_all(entry);

	//gtk_widget_set_sensitive(entry, FALSE);
	
	return entry;
}

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
	ttt->toolbar = make_toolbar(ttt);
	gtk_box_pack_start(GTK_BOX(ttt), ttt->toolbar, FALSE, FALSE, 0);

	ttt->xpath_entry = make_xpath_entry(ttt);
	gtk_box_pack_start(GTK_BOX(ttt), ttt->xpath_entry, FALSE, FALSE, 0);

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
xml_navigator_set_model(XmlNavigator *ttt, xmlTreeModel * xmltreemodel)
{
	ttt->model = xmltreemodel;
	ttt->filter = create_filter(xmltreemodel);
	gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->navigator_view), GTK_TREE_MODEL(ttt->filter));	
	g_signal_emit(ttt, xml_navigator_signals[MODEL_CHANGED],0,xmltreemodel);

}

static GtkTreeModelFilter * create_filter(xmlTreeModel * xmltreemodel) {
	/* Create filter with virtual root set to second branch*/
	GtkTreePath *virtual_root;
	virtual_root = gtk_tree_path_new_from_string("0");		
	/* Create filter and set visible column */
	GtkTreeModelFilter *filter;
	filter = gtk_tree_model_filter_new( GTK_TREE_MODEL( xmltreemodel ), virtual_root );
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER( filter ), XML_TREE_MODEL_COL_VISIBLE );
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
	
	xml_tree_model_set_visible (ttt->model, ttt->toolbar_buttons[i].type, toggle);

	if(ttt->toolbar_buttons[i].type == XML_DTD_NODE) {
		xml_tree_model_set_visible (ttt->model, XML_ATTRIBUTE_DECL, toggle);
		xml_tree_model_set_visible (ttt->model, XML_ENTITY_DECL, toggle);
		xml_tree_model_set_visible (ttt->model, XML_ELEMENT_DECL, toggle);
	}
	
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(ttt->filter));
	return TRUE;
}

static gboolean
xml_toggle_visible(XmlNavigator *ttt,
                   xmlTreeModel * xmltreemodel,
                   XmlToolBarButton *button)
{
	button->model = ttt->model;	
	gtk_widget_set_sensitive (GTK_WIDGET(button->button), TRUE);
	gtk_toggle_tool_button_set_active(button->button, xml_tree_model_get_visible (xmltreemodel, button->type));
	g_signal_connect(button->button, "toggled", G_CALLBACK(toolbar_button_toggle), ttt);
	return TRUE;
}

static void
make_toolbar_button(XmlNavigator	 *ttt,
					xmlElementType type,
    	            XmlToolBarButton * button)
{
	button->button = gtk_toggle_tool_button_new_from_stock(XmlNodes[type].stock_id);
	gtk_widget_set_sensitive (GTK_WIDGET(button->button), FALSE);
	button->type = type;
	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(xml_toggle_visible), button);
}

static void validate_clicked(GtkToolButton * button, XmlNavigator *ttt){
	xml_tree_model_validate(ttt->model);
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

	GtkWidget *button = gtk_tool_button_new_from_stock("gtk-apply");
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(button));
	g_signal_connect(button, "clicked", G_CALLBACK(validate_clicked), ttt);

	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(gtk_separator_tool_item_new()));

	for(int i=0;i<6;++i)
	{
		gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(ttt->toolbar_buttons[i].button));
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
	g_return_val_if_fail(ttt != NULL, NULL);
	
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
                                        "stock-id", XML_TREE_MODEL_COL_TYPE,
                                        NULL);
/*
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_NS);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);
*/

 	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_NAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);
 
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer,"ellipsize", PANGO_ELLIPSIZE_END, NULL);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_CONTENT);
	gtk_tree_view_column_set_title (col, "Value");
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_XPATH);
	gtk_tree_view_column_set_title (col, "Xpath");
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);


	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(view), TRUE);


	return view;
}


