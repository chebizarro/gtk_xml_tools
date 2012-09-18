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
	XSL_MENU_ACTIVATED,
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
	ttt->filter.row_visible[XML_DTD_NODE] = TRUE;
	ttt->filter.row_visible[XML_ENTITY_DECL] = TRUE;
	ttt->filter.row_visible[XML_ATTRIBUTE_DECL] = TRUE;
	ttt->filter.row_visible[XML_ELEMENT_DECL] = TRUE;
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

/* Xpath */

static gboolean
xpath_update_completion(XmlNavigator *ttt,
	                   	xmlTreeModel * xmltreemodel,
						GtkEntry * xpath_entry)
{
	GtkListStore * xpath_result;
	GtkEntryCompletion * completion;
	gchar * xpath;

	xpath = gtk_entry_get_text(GTK_ENTRY(xpath_entry));	
	completion = gtk_entry_get_completion(xpath_entry);

	GString * xpath_full;
	xpath_full = g_string_new("");
	g_string_printf(xpath_full, "%s/*",xpath);

	xpath_result = xml_get_xpath_results(ttt->model, xpath_full->str);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(xpath_result));
	
	g_string_free(xpath_full, FALSE);
	

	return TRUE;
}

static gboolean
xpath_match_select(	GtkEntryCompletion	* widget,
					GtkTreeModel		* model,
					GtkTreeIter			* iter,
					XmlNavigator		* ttt)
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
run_xpath(GtkEntry * xpath_entry, XmlNavigator *ttt ) {
	GtkListStore * xpath_result;
	gchar * xpath = gtk_entry_get_text(GTK_ENTRY(xpath_entry));	
	xpath_result = xml_get_xpath_results(ttt->model, xpath);
	if(xpath_result != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->xpathExplorer.results), GTK_TREE_MODEL(xpath_result));
		g_signal_emit(ttt, xml_navigator_signals[XPATH_MODEL_CHANGED],0,GTK_TREE_MODEL(xpath_result));
	}
}

static void
xpath_activate_cb (GtkEntry  *entry,
             XmlNavigator *ttt)
{
	run_xpath(entry, ttt);
}

static void
xpath_icon_press_cb (GtkEntry       *entry,
               gint            position,
               GdkEventButton *event,
               XmlNavigator *ttt)
{
	if (position == GTK_ENTRY_ICON_PRIMARY) {
		xpath_activate_cb(entry, ttt);
	} else {
		gtk_entry_set_text (entry, "");
		gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->xpathExplorer.results), NULL);
	}
 }

/* validate */
static void
validator_entry_icon_press_cb (	GtkEntry		* entry,
								gint            position,
								GdkEventButton	* event,
								XmlNavigator	* ttt)
{
	/* Sub menu for file button */
	GtkWidget *file_menu, *open_item;
	file_menu = gtk_menu_new();

	open_item = gtk_menu_item_new_with_label("Add File");
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
	//g_signal_connect(open_item, "activate", G_CALLBACK(xslt_add_file), ttt);
	gtk_widget_show (open_item);
	
	open_item = gtk_menu_item_new_with_label("Open Files...");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(open_item),gtk_menu_new());
	gtk_widget_show (open_item);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
	
	gtk_menu_popup(file_menu, NULL, NULL, NULL, NULL, event->button, event->time);
}

/* Toolbar */
static void validate_clicked(GtkToolButton * button, XmlNavigator *ttt){
	xml_tree_model_validate(ttt->model);
}

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

void
xml_navigator_set_model(XmlNavigator *ttt, xmlTreeModel * xmltreemodel)
{
	ttt->model = xmltreemodel;
	ttt->filter.filter = create_filter(ttt);
	gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->navigator), GTK_TREE_MODEL(ttt->filter.filter));
	//g_object_unref(ttt->filter.filter);	
	g_signal_emit(ttt, xml_navigator_signals[MODEL_CHANGED],0,xmltreemodel);

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

void xslt_add_file_items(GtkMenuItem *item, XmlNavigator * ttt)
{
	GtkWidget * files_menu = gtk_menu_item_get_submenu(item);
	
}

void xslt_add_file(XmlNavigator * ttt){
	gchar *file_path = NULL;

	file_path = show_open_dialog();
	
	if(file_path == NULL)
		return;

	ttt->stylesheet = xml_tree_model_new();
	xml_tree_model_add_file(ttt->stylesheet,file_path);
	gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->transformer.parameters), GTK_TREE_MODEL(xml_tree_model_get_stylesheet_params (ttt->stylesheet))); 
	g_object_unref(ttt->stylesheet);
}

static gboolean
xslt_add_params(XmlNavigator 	* ttt,
                xmlTreeModel 	* xmltreemodel,
	            GtkTreeView 	* treeview)
{
	if(xmltreemodel->xsldoc != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(treeview),
								GTK_TREE_MODEL(xml_tree_model_get_stylesheet_params (ttt->model))); 
		g_object_unref(ttt->model);
	}
}

static void
xslt_entry_icon_press_cb (	GtkEntry		* entry,
							gint            position,
							GdkEventButton	*event,
							XmlNavigator	*ttt)
{

	/* Sub menu for file button */
	GtkWidget *file_menu, *open_item, *items;
	file_menu = gtk_menu_new();

	open_item = gtk_menu_item_new_with_label("Add File");
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
	g_signal_connect(open_item, "activate", G_CALLBACK(xslt_add_file), ttt);
	gtk_widget_show (open_item);
	
	items = gtk_menu_new();

	open_item = gtk_menu_item_new_with_label("Open Files...");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(open_item),items);
	gtk_widget_show (open_item);

	g_signal_emit(ttt, xml_navigator_signals[XSL_MENU_ACTIVATED],0,items);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);

	g_signal_connect(open_item, "activate", G_CALLBACK(xslt_add_file_items), ttt);
	
	gtk_menu_popup(file_menu, NULL, NULL, NULL, NULL, event->button, event->time);

}

/* Ui definitions */

static void
make_toolbar_button(XmlNavigator	 *ttt,
					xmlElementType type,
    	            xmlToolBarButton * button)
{
	button->button = gtk_toggle_tool_button_new_from_stock(XmlNodes[type].stock_id);

	GString * ns_name = g_string_new("");
	g_string_printf(ns_name, "Show/Hide %s", XmlNodes[type].label);
	gtk_widget_set_tooltip_text(button->button, ns_name->str);
	g_string_free(ns_name, FALSE);

	gtk_widget_set_sensitive (GTK_WIDGET(button->button), FALSE);
	button->type = type;
	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(xml_toggle_visible), button);
}

static void
make_toolbar(XmlNavigator * ttt)
{
	GtkToolbar * toolbar; 

	toolbar = gtk_toolbar_new();

	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	make_toolbar_button(ttt,XML_ATTRIBUTE_NODE, &ttt->toolbar.toolbar_buttons[0]);
	make_toolbar_button(ttt,XML_DTD_NODE, &ttt->toolbar.toolbar_buttons[1]);
	make_toolbar_button(ttt,XML_COMMENT_NODE, &ttt->toolbar.toolbar_buttons[2]);
	make_toolbar_button(ttt,XML_PI_NODE, &ttt->toolbar.toolbar_buttons[3]);
	make_toolbar_button(ttt,XML_TEXT_NODE, &ttt->toolbar.toolbar_buttons[4]);
	make_toolbar_button(ttt,XML_CDATA_SECTION_NODE, &ttt->toolbar.toolbar_buttons[5]);

	GtkWidget *button = gtk_tool_button_new_from_stock("gtk-apply");
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(button));
	g_signal_connect(button, "clicked", G_CALLBACK(validate_clicked), ttt);
	gtk_widget_set_tooltip_text(button, "Validate");
	

	button = gtk_toggle_tool_button_new_from_stock("gtk-zoom-100");
	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(button));
	gtk_widget_set_sensitive (GTK_WIDGET(button), FALSE);
	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(toggle_ns), button);
	gtk_widget_set_tooltip_text(button, "Show/Hide Namespace");


	gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(gtk_separator_tool_item_new()));

	for(int i=0;i<6;++i)
	{
		gtk_container_add(GTK_CONTAINER(toolbar), GTK_WIDGET(ttt->toolbar.toolbar_buttons[i].button));
	}

	ttt->toolbar.toolbar = toolbar;
	gtk_box_pack_start(GTK_BOX(ttt), ttt->toolbar.toolbar, FALSE, FALSE, 0);
}

static GtkWidget *
make_xpath_entry (XmlNavigator *ttt)
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

	return entry;
}

static void
make_xpath_completion (XmlNavigator *ttt)
{
	GtkEntryCompletion *completion;

    completion = gtk_entry_completion_new();

	gtk_entry_completion_set_text_column(completion, XML_TREE_MODEL_COL_XPATH);

	gtk_entry_set_completion(GTK_ENTRY(ttt->xpathExplorer.entry), completion);
    //g_signal_connect(G_OBJECT (completion), "match-selected", G_CALLBACK (xpath_match_select), ttt);

	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(xpath_update_completion), ttt->xpathExplorer.entry);
	g_signal_connect(ttt, "xpath-model-changed", G_CALLBACK(xpath_update_completion), ttt->xpathExplorer.entry);

}

static void
make_xpath_explorer(XmlNavigator * ttt)
{
	ttt->xpathExplorer.widget = gtk_vbox_new(FALSE,3);

	ttt->xpathExplorer.expander = gtk_expander_new("Xpath");
	
	ttt->xpathExplorer.entry = make_xpath_entry(ttt);

	ttt->xpathExplorer.results = make_navigator_view(ttt);
	
	make_xpath_completion(ttt);

	GtkWidget * vbox;
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), ttt->xpathExplorer.entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), ttt->xpathExplorer.results, TRUE, FALSE, 0);

	gtk_container_add(ttt->xpathExplorer.expander, vbox);

	gtk_expander_set_expanded(ttt->xpathExplorer.expander, TRUE);
	
	gtk_box_pack_start(GTK_BOX(ttt->xpathExplorer.widget), ttt->xpathExplorer.expander, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(ttt), ttt->xpathExplorer.widget, FALSE, FALSE, 0);

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
 
	//view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ttt->filter.filter));
 	//g_object_unref(ttt->filter.filter); /* destroy store automatically with view */

	view = gtk_tree_view_new ();
	
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
	gtk_tree_view_column_set_title (col, "Value");
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(view), TRUE);

	return view;
}

static void
make_navigator(XmlNavigator * ttt)
{
	GtkWidget * scrolled_window;
	GtkWidget * vbox;
	
    scrolled_window = make_scrolled_window();
    vbox = gtk_vbox_new(FALSE, 0);
	ttt->navigator = make_navigator_view(ttt);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vbox);
    
	gtk_container_add(GTK_CONTAINER(vbox), ttt->navigator);

    gtk_box_pack_start (GTK_BOX(ttt), scrolled_window, TRUE, TRUE, 0);
	
}

static GtkEntry *
make_xslt_entry (XmlNavigator *ttt)
{
	GtkEntry *entry;
	
	entry = gtk_entry_new();
	
	gtk_entry_set_icon_from_stock (GTK_ENTRY (entry),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   GTK_STOCK_OPEN);
	
	g_signal_connect (entry, "icon-press",
                      G_CALLBACK (xslt_entry_icon_press_cb), ttt);

	return entry;
}

static GtkWidget *
make_xslt_param_view (XmlNavigator * ttt)
{
	g_return_val_if_fail(ttt != NULL, NULL);
	
	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer;
	GtkWidget			*view;
 
	view = gtk_tree_view_new();

 	col = gtk_tree_view_column_new();
 
 	gtk_tree_view_column_set_title (col, "Parameter");
	gtk_tree_view_column_set_resizable(col, TRUE);
 
 	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);
 
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer,"editable", TRUE, NULL);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 1);
	gtk_tree_view_column_set_title (col, "Value");
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	return view;
}

static void
make_xslt_transformer(XmlNavigator * ttt)
{
	ttt->transformer.widget = gtk_vbox_new(FALSE,3);
	ttt->transformer.expander = gtk_expander_new("Transform");

	ttt->transformer.entry = make_xslt_entry(ttt);
	ttt->transformer.parameters = make_xslt_param_view(ttt);

	GtkWidget * vbox;
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), ttt->transformer.entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), ttt->transformer.parameters, TRUE, FALSE, 0);

	gtk_container_add(ttt->transformer.expander, vbox);
	
	gtk_box_pack_start(GTK_BOX(ttt->transformer.widget), ttt->transformer.expander, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ttt), ttt->transformer.widget, FALSE, FALSE, 0);

	g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(xslt_add_params), ttt->transformer.parameters);

}

static GtkEntry *
make_validator_entry (XmlNavigator *ttt)
{
	GtkEntry *entry;
	
	entry = gtk_entry_new();
	
	gtk_entry_set_icon_from_stock (GTK_ENTRY (entry),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   GTK_STOCK_OPEN);
	
	g_signal_connect (entry, "icon-press",
                      G_CALLBACK (validator_entry_icon_press_cb), ttt);

	return entry;
}


static void
make_validator( XmlNavigator *ttt)
{
	GtkWidget * vbox;
	vbox = gtk_vbox_new(FALSE, 3);

	ttt->validator.widget = gtk_vbox_new(FALSE,3);
	ttt->validator.expander = gtk_expander_new("Validate");
	ttt->validator.entry = make_validator_entry(ttt);

	gtk_widget_set_tooltip_text(ttt->validator.entry, "Select a schema to validate against");
	

	gtk_box_pack_start(GTK_BOX(vbox), ttt->validator.entry, TRUE, TRUE, 0);
	gtk_container_add(ttt->validator.expander, vbox);
	gtk_box_pack_start(GTK_BOX(ttt->validator.widget), ttt->validator.expander, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ttt), ttt->validator.widget, FALSE, FALSE, 0);

	//g_signal_connect(ttt, "xml-model-changed", G_CALLBACK(xslt_add_params), ttt->transformer.parameters);
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

	xml_navigator_signals[XSL_MENU_ACTIVATED] = g_signal_new ("xsl-menu-activated",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XmlNavigatorClass, xsl_menu_activated),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

	xml_navigator_signals[XPATH_MODEL_CHANGED] = g_signal_new ("xpath-model-changed",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (XmlNavigatorClass, xpath_model_changed),
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

static void
xml_navigator_init (XmlNavigator *ttt)
{

	make_toolbar(ttt);

	make_xpath_explorer(ttt);

	make_validator(ttt);
	
	make_xslt_transformer(ttt);

	make_filter(ttt);
	
	make_navigator(ttt);

}
