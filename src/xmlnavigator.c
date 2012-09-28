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
	XPATH_MODEL_CHANGED,
	LAST_SIGNAL
};

static void xml_navigator_class_init		(XmlNavigatorClass 	*klass);
static void xml_navigator_init				(XmlNavigator		*ttt);

static GtkWidget * make_navigator_view(XmlNavigator *ttt);
static GtkWidget * make_scrolled_window(void);
static void make_toolbar(XmlNavigator * ttt);
static GtkTreeModelFilter * create_filter(XmlNavigator * ttt);

static guint xml_navigator_signals[LAST_SIGNAL] = { 0 };

/* Filter */
static gboolean
show_visible(GtkTreeModel	*model,
             GtkTreeIter	*iter,
             XmlNavigator	*ttt)
{
	gboolean visible;
	
	gint row = 0;
	gtk_tree_model_get (model, iter, XML_TREE_MODEL_COL_TYPE, &row, -1);

	if (row > 0 && row < XML_N_NODE_TYPES)
		visible = ttt->filter.row_visible[row];
		
	return visible;

}

static void
modify_columns (GtkTreeModel *model,
                GtkTreeIter *iter,
                GValue *value,
                gint column,
                XmlNavigator *ttt)
{
    GtkTreeModel	*child;
    GtkTreeIter   	iter_c;

    /* Get child model and iter */
    child = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ) );
    gtk_tree_model_filter_convert_iter_to_child_iter( GTK_TREE_MODEL_FILTER( model ),
                                                      &iter_c, iter );

    switch( column )
    {
		case XML_TREE_MODEL_COL_NAME:
		{
			if(ttt->filter.show_ns == TRUE) {
				gchar	*ns;  /* This is source string from child model */
				gchar	*name;
				glong	ns_length;
				/* Get source string */
				gtk_tree_model_get( child, &iter_c, XML_TREE_MODEL_COL_NS, &ns, -1 );
				gtk_tree_model_get( child, &iter_c, XML_TREE_MODEL_COL_NAME, &name, -1 );
				ns_length = g_utf8_strlen(ns,-1);
				if(ns_length < 1) {
					g_value_set_string( value, name );
				} else {
					GString * ns_name;
					ns_name = g_string_new("");
					g_string_printf(ns_name, "%s:%s",ns,name);
					g_value_set_string( value, ns_name->str );
					g_string_free(ns_name, FALSE);
					g_free(ns);
					g_free(name);
				}
				break;
			}
		}
		case XML_TREE_MODEL_COL_NS:
		case XML_TREE_MODEL_COL_TYPE:
		case XML_TREE_MODEL_COL_CONTENT:
		case XML_TREE_MODEL_COL_LINE:
		case XML_TREE_MODEL_COL_POS:
		case XML_TREE_MODEL_COL_XPATH:
			gtk_tree_model_get_value( child, &iter_c, column, value);
			break;
    }
}

static GtkTreeModelFilter * create_filter(XmlNavigator *ttt) {
	/* Create filter with virtual root set to second branch*/
	GtkTreePath *virtual_root;
	virtual_root = gtk_tree_path_new_from_string("0");		
	/* Create filter and set visible column */
	
	GtkTreeModelFilter *filter;
	filter = gtk_tree_model_filter_new( GTK_TREE_MODEL( ttt->model ), virtual_root );

	xmlTreeModel * model;
	model = ttt->model;
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter), show_visible, ttt, NULL);

	gtk_tree_model_filter_set_modify_func (	filter,
                                          	XML_TREE_MODEL_N_COLUMNS,
                                            model->column_types,
                                            modify_columns,
                                            ttt,
                                            NULL);

	return filter;
}

void
make_filter (XmlNavigator * ttt)
{
	ttt->filter.row_visible[XML_ELEMENT_NODE] = TRUE;
	ttt->filter.row_visible[XML_DOCUMENT_NODE] = TRUE;
	ttt->filter.row_visible[XML_ATTRIBUTE_NODE] = TRUE;
	ttt->filter.row_visible[XML_TEXT_NODE] = FALSE;
	/*
	ttt->filter.row_visible[XML_DTD_NODE] = TRUE;
	ttt->filter.row_visible[XML_ENTITY_DECL] = TRUE;
	ttt->filter.row_visible[XML_ATTRIBUTE_DECL] = TRUE;
	ttt->filter.row_visible[XML_ELEMENT_DECL] = TRUE;
	*/
	ttt->filter.row_visible[XML_HTML_DOCUMENT_NODE] = TRUE;

	ttt->filter.show_ns = TRUE;
}

void
tree_model_filter_set_visible (XmlNavigator *ttt, xmlElementType nodetype, gboolean visible)
{
	ttt->filter.row_visible[nodetype] = visible;
}

gboolean
tree_model_filter_get_visible (XmlNavigator *ttt, xmlElementType nodetype)
{
	return ttt->filter.row_visible[nodetype];
}


/* Callbacks */

/* Toolbar */

static gboolean
toolbar_button_toggle(GtkToggleToolButton *button,
                      XmlNavigator * ttt)
{
	gboolean toggle;
	int i = 0;
	
	toggle = (gtk_toggle_tool_button_get_active(button)) ? TRUE : FALSE;
	
	while(ttt->toolbar.toolbar_buttons[i].button != button) {
		++i;
	}
	
	tree_model_filter_set_visible (ttt, ttt->toolbar.toolbar_buttons[i].type, toggle);

	if(ttt->toolbar.toolbar_buttons[i].type == XML_DTD_NODE) {
		tree_model_filter_set_visible (ttt, XML_ATTRIBUTE_DECL, toggle);
		tree_model_filter_set_visible (ttt, XML_ENTITY_DECL, toggle);
		tree_model_filter_set_visible (ttt, XML_ELEMENT_DECL, toggle);
	}
	
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(ttt->filter.filter));
	return TRUE;
}

static gboolean
xml_toggle_visible(XmlNavigator *ttt,
                   xmlTreeModel * xmltreemodel,
                   xmlToolBarButton *button)
{
	button->model = ttt->model;	
	gtk_widget_set_sensitive (GTK_WIDGET(button->button), TRUE);
	gtk_toggle_tool_button_set_active(button->button, tree_model_filter_get_visible (ttt, button->type));
	g_signal_connect(button->button, "toggled", G_CALLBACK(toolbar_button_toggle), ttt);
	return TRUE;
}

static gboolean
ns_button_toggle (	GtkToggleToolButton *button,
                     XmlNavigator * ttt)
{
	gboolean toggle;
	toggle = gtk_toggle_tool_button_get_active(button);
	ttt->filter.show_ns = toggle;

	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(ttt->filter.filter));
	return TRUE;
}

static void
update_breadcrumbs (XmlNavigator *ttt,
					xmlTreeModel *xmltreemodel,
					XmlBreadcrumbs *breadcrumbs)
{
	xml_breadcrumbs_set_model(ttt->breadcrumbs, xmltreemodel);
}

static gboolean
toggle_ns (XmlNavigator *ttt,
           xmlTreeModel * xmltreemodel,
           GtkToggleToolButton * button)
{
	gtk_widget_set_sensitive (GTK_WIDGET(button), TRUE);
	gtk_toggle_tool_button_set_active(button, ttt->filter.show_ns);	

	xmlTreeModel * model = ttt->model;

	if(ttt->filter.show_ns)
		gtk_tree_model_filter_set_modify_func (	ttt->filter.filter,
                                          	XML_TREE_MODEL_N_COLUMNS,
                                            model->column_types,
                                            modify_columns,
                                            ttt,
                                            NULL);
	
	g_signal_connect(button, "toggled", G_CALLBACK(ns_button_toggle), ttt);
	return TRUE;

}

/* Navigator */
void
xml_navigator_copy(GtkWidget *menuitem, XmlNavigator *ttt)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gchar		*xpath;
	gchar		*menu_name = gtk_menu_item_get_label(menuitem);
	gint		column = 0;
	GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ttt->navigator));

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
xml_view_popup_menu (GtkWidget *treeview, GdkEventButton *event, XmlNavigator *ttt)
{

	GtkWidget *results_menu, *results_item;
	results_menu = gtk_menu_new();

	results_item = gtk_menu_item_new_with_label(_("Copy XPath"));
	g_signal_connect(results_item, "activate", G_CALLBACK(xml_navigator_copy), ttt);
	gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_item);

	results_item = gtk_menu_item_new_with_label(_("Copy Name"));
	g_signal_connect(results_item, "activate", G_CALLBACK(xml_navigator_copy), ttt);
	gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_item);

	results_item = gtk_menu_item_new_with_label(_("Copy Value"));
	g_signal_connect(results_item, "activate", G_CALLBACK(xml_navigator_copy), ttt);
	gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_item);

	//results_item = gtk_menu_item_new_with_label(_("Show in XPath Explorer"));
	//g_signal_connect(results_item, "activate", G_CALLBACK(xpath_show_in_navigator), ttt);
	//gtk_menu_shell_append(GTK_MENU_SHELL(results_menu), results_item);

	gtk_widget_show_all(results_menu);

    gtk_menu_popup(GTK_MENU(results_menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));

}

static gboolean
xml_navigator_button_press_event(	GtkWidget *widget,
									GdkEventButton *event,
									XmlNavigator *ttt)
{
	/* single click with the right mouse button */
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ttt->navigator));
		if (gtk_tree_selection_count_selected_rows(selection)  == 1) {
			xml_view_popup_menu(ttt->navigator, event, ttt);
			g_signal_emit(ttt, xml_navigator_signals[BUTTON_PRESS_EVENT],0,event);
			return TRUE; 
		}
	}
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

void
xml_navigator_refresh_model(xmlTreeModel * xmltreemodel,
							gpointer	data,
							XmlNavigator * ttt)
{
	gtk_tree_model_filter_refilter(ttt->filter.filter);
	g_signal_emit(ttt, xml_navigator_signals[MODEL_CHANGED],0,xmltreemodel);
}

void
xml_navigator_set_model(XmlNavigator *ttt, xmlTreeModel * xmltreemodel)
{
	ttt->model = xmltreemodel;
	ttt->filter.filter = create_filter(ttt);
	gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->navigator), GTK_TREE_MODEL(ttt->filter.filter));
	gtk_tree_view_expand_row(GTK_TREE_VIEW(ttt->navigator),gtk_tree_path_new_first(), FALSE), 
	//g_object_unref(ttt->filter.filter);	
	g_signal_emit(ttt, xml_navigator_signals[MODEL_CHANGED],0,xmltreemodel);
	g_signal_connect(ttt->model, "xml-tree-model-changed", G_CALLBACK(xml_navigator_refresh_model), ttt);
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


/* Ui definitions */

static void
make_toolbar_button(XmlNavigator	 *ttt,
					xmlElementType type,
    	            xmlToolBarButton * button)
{
	button->button = gtk_toggle_tool_button_new_from_stock(XmlNodes[type].stock_id);

	GString * ns_name = g_string_new("");
	g_string_printf(ns_name, _("Show/Hide %s"), XmlNodes[type].label);
	gtk_widget_set_tooltip_text(button->button, ns_name->str);
	g_string_free(ns_name, FALSE);

	gtk_widget_set_sensitive (GTK_WIDGET(button->button), FALSE);
	button->type = type;
	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(xml_toggle_visible), button);
}

static void
make_toolbar(XmlNavigator * ttt)
{
	GtkToolbar			*toolbar; 
	GtkToggleToolButton	*button;
	
	toolbar = gtk_toolbar_new();

	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	make_toolbar_button(ttt,XML_ATTRIBUTE_NODE, &ttt->toolbar.toolbar_buttons[0]);
	make_toolbar_button(ttt,XML_DTD_NODE, &ttt->toolbar.toolbar_buttons[1]);
	make_toolbar_button(ttt,XML_COMMENT_NODE, &ttt->toolbar.toolbar_buttons[2]);
	make_toolbar_button(ttt,XML_PI_NODE, &ttt->toolbar.toolbar_buttons[3]);
	make_toolbar_button(ttt,XML_TEXT_NODE, &ttt->toolbar.toolbar_buttons[4]);
	make_toolbar_button(ttt,XML_CDATA_SECTION_NODE, &ttt->toolbar.toolbar_buttons[5]);

	button = gtk_toggle_tool_button_new_from_stock("gtk-zoom-100");
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(button));
	gtk_widget_set_sensitive (GTK_WIDGET(button), FALSE);
	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(toggle_ns), button);
	gtk_widget_set_tooltip_text(button, _("Show/Hide Namespace"));


	for(int i=0;i<6;++i)
	{
		gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(ttt->toolbar.toolbar_buttons[i].button));
	}

	ttt->toolbar.toolbar = toolbar;
	gtk_box_pack_start(GTK_BOX(ttt), ttt->toolbar.toolbar, FALSE, FALSE, 0);
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

static GtkWidget *
make_navigator_view (XmlNavigator * ttt)
{
	g_return_val_if_fail(ttt != NULL, NULL);
	
	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer, *icon_renderer;
	GtkWidget			*view;
 
	view = gtk_tree_view_new_with_model (xml_tree_model_new());
	
	g_signal_connect(view, "button-press-event",
			G_CALLBACK(xml_navigator_button_press_event), ttt);

	g_signal_connect(view, "row-activated",
			G_CALLBACK(xml_navigator_row_activated), ttt);

	g_signal_connect(view, "row-expanded",
			G_CALLBACK(xml_navigator_row_expanded), ttt);

	g_signal_connect(view, "row-collapsed",
			G_CALLBACK(xml_navigator_row_collapsed), ttt);

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
	g_object_set(renderer,"ellipsize", PANGO_ELLIPSIZE_END, NULL);
	g_object_set(renderer,"single-paragraph-mode", TRUE, NULL);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", XML_TREE_MODEL_COL_CONTENT);
	gtk_tree_view_column_set_title (col, _("Value"));
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(view), TRUE);

	return view;
}

static void
make_navigator(XmlNavigator * ttt)
{
	GtkWidget * scrolled_window;
	
    scrolled_window = make_scrolled_window();
	ttt->navigator = make_navigator_view(ttt);
   
	gtk_container_add(GTK_CONTAINER(scrolled_window), ttt->navigator);

    gtk_box_pack_start (GTK_BOX(ttt), scrolled_window, TRUE, TRUE, 0);
	
}


/* Boiler plate code for class definitions */

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
	xml_navigator_signals[BUTTON_PRESS_EVENT] = g_signal_new ("xml-button-press",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XmlNavigatorClass, xml_button_press),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);


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

GtkWidget*
xml_navigator_new ()
{
	xml_icon_factory_new();
	return GTK_WIDGET (g_object_new (xml_navigator_get_type (), "orientation", GTK_ORIENTATION_VERTICAL, NULL));
}

gboolean
xml_navigator_goto_path(XmlBreadcrumbs *breadcrumbs, GtkTreePath *path, XmlNavigator *ttt) {
	g_return_if_fail(ttt != NULL);
	g_return_if_fail(path != NULL);
	g_return_if_fail(breadcrumbs != NULL);

	GtkTreePath * treepath;
	GtkTreeViewColumn * col;
	
	col = gtk_tree_view_get_column(ttt->navigator, 0);

	treepath = gtk_tree_model_filter_convert_child_path_to_path(ttt->filter.filter, path);

	g_return_if_fail(treepath != NULL);

	gtk_tree_view_expand_to_path(ttt->navigator, treepath);
	gtk_tree_view_set_cursor(ttt->navigator, treepath, col, FALSE);

	gtk_tree_path_free(treepath);

}

static void
make_breadcrumbs(XmlNavigator *ttt)
{
	ttt->breadcrumbs = xml_breadcrumbs_new();
    gtk_box_pack_end (GTK_BOX(ttt), ttt->breadcrumbs, FALSE, FALSE, 0);
   	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(update_breadcrumbs), ttt->breadcrumbs);
   	g_signal_connect(ttt->breadcrumbs, "xml-breadcrumb-path-clicked", G_CALLBACK(xml_navigator_goto_path), ttt);

}

static void
xml_navigator_init (XmlNavigator *ttt)
{
	make_toolbar(ttt);
	make_filter(ttt);
	make_navigator(ttt);
	make_breadcrumbs(ttt);
}

void
xml_navigator_goto_file_location (XmlNavigator *ttt, int column)
{
	g_return_if_fail(ttt != NULL);
	g_return_if_fail(ttt->model != NULL);

	GtkTreePath * treepath, *childpath;
	GtkTreeViewColumn * col;
	
	col = gtk_tree_view_get_column(ttt->navigator, 0);

	childpath = xml_tree_model_get_path_from_position(ttt->model, column);
	
	treepath = gtk_tree_model_filter_convert_child_path_to_path(ttt->filter.filter, childpath);

	g_return_if_fail(treepath != NULL);

	gtk_tree_view_expand_to_path(ttt->navigator, treepath);
	gtk_tree_view_set_cursor(ttt->navigator, treepath, col, FALSE);
	//gtk_tree_view_row_activated(ttt->navigator, treepath, col);

	gtk_tree_path_free(treepath);
	gtk_tree_path_free(childpath);
}


void
xml_navigator_goto_xpath (XmlNavigator *ttt, gchar *xpath)
{
	g_return_if_fail(ttt != NULL);
	g_return_if_fail(ttt->model != NULL);
	g_return_if_fail(xpath != NULL);

	GtkTreePath * treepath, *childpath;
	GtkTreeViewColumn * column;
	
	column = gtk_tree_view_get_column(ttt->navigator, 0);

	childpath = xml_tree_model_get_path_from_xpath(ttt->model, xpath);
	
	treepath = gtk_tree_model_filter_convert_child_path_to_path(ttt->filter.filter, childpath);

	g_return_if_fail(treepath != NULL);

	gtk_tree_view_expand_to_path(ttt->navigator, treepath);
	gtk_tree_view_set_cursor(ttt->navigator, treepath, column, FALSE);
	//gtk_tree_view_row_activated(ttt->navigator, treepath, column);

	gtk_tree_path_free(treepath);
	gtk_tree_path_free(childpath);
}
