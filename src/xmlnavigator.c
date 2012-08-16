/*
 *	Custom Widget Template
 *
 */
 
#include "xmlnavigator.h"

enum {
	NAVIGATOR_DBLCLK_SIGNAL,
	LAST_SIGNAL
};

static void xml_navigator_class_init		(XmlNavigatorClass *klass);
static void xml_navigator_init				(XmlNavigator			*ttt);

static GtkWidget * make_navigator_view(XmlList * xmllist);
static GtkWidget * make_scrolled_window(void);
static GtkWidget * make_toolbar(void);
static GtkWidget * make_xpath_entry(XmlNavigator *ttt, XmlList * xmllist);

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
	xml_navigator_signals[NAVIGATOR_DBLCLK_SIGNAL] = g_signal_new ("navigator-selected",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
												G_STRUCT_OFFSET (XmlNavigatorClass, navigator_selected),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__VOID,
												G_TYPE_NONE, 0);
}



static void
xml_navigator_init (XmlNavigator *ttt)
{

	ttt->xpath_entry = make_xpath_entry(ttt, NULL);
    gtk_box_pack_start(GTK_BOX(ttt), ttt->xpath_entry, FALSE, FALSE, 0);

	ttt->toolbar = make_toolbar();
	gtk_box_pack_start(GTK_BOX(ttt), ttt->toolbar, FALSE, FALSE, 0);

    ttt->scrolled_window = make_scrolled_window();
    gtk_box_pack_start (GTK_BOX(ttt), ttt->scrolled_window, TRUE, TRUE, 0);

	ttt->navigator_view_vbox = gtk_vbox_new(FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ttt->scrolled_window), ttt->navigator_view_vbox);
	
	ttt->navigator_view = make_navigator_view(NULL);
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
	
	gtk_widget_destroy(ttt->navigator_view);
	
	ttt->navigator_view = make_navigator_view(xmllist);
	
	gtk_container_add(GTK_CONTAINER(ttt->navigator_view_vbox), ttt->navigator_view);	
	gtk_widget_show_all(ttt->navigator_view);

	
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

void xml_toggle_visible(GtkToggleToolButton *toggle_tool_button, xmlElementType node) {
	
}

static GtkWidget *
make_toolbar(void)
{
	GtkWidget *wid, *toolbar;

	/* Create the toolbar object and initialise it*/
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	/* attributes visible item */
	wid = GTK_WIDGET(gtk_toggle_tool_button_new_from_stock("xml_attribute_node"));
	//g_signal_connect(wid, "toggled", G_CALLBACK(xml_toggle_visible), XML_ATTRIBUTE_NODE);
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	/* DTD */
	wid = GTK_WIDGET(gtk_toggle_tool_button_new_from_stock("xml_dtd_node"));
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_toggle_tool_button_new_from_stock("xml_comment_node"));
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_toggle_tool_button_new_from_stock("xml_pi_node"));
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_toggle_tool_button_new_from_stock("xml_text_node"));
	gtk_container_add(GTK_CONTAINER(toolbar), wid);

	wid = GTK_WIDGET(gtk_toggle_tool_button_new_from_stock("xml_cdata_section_node"));
	gtk_container_add(GTK_CONTAINER(toolbar), wid);


	return toolbar;
}

static gboolean
on_tree_view_button_press_event(GtkWidget *widget,
								GdkEventButton *event,
								gpointer       user_data)
{
	//g_signal_emit_by_name ((gpointer) widget, "navigator-selected");
	return FALSE;
}


static GtkWidget *
make_navigator_view (XmlList *xmllist)
{
	if(xmllist == NULL)
		return gtk_label_new ("Add a file");

	GtkTreeViewColumn		*col;
	GtkCellRenderer		*renderer, *icon_renderer;
	GtkWidget				*view;
 
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(xmllist));
 	g_object_unref(xmllist); /* destroy store automatically with view */

	g_signal_connect(view, "button-press-event",
			G_CALLBACK(on_tree_view_button_press_event), NULL);
 
 
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

static GtkWidget *
make_xpath_entry (XmlNavigator *ttt, XmlList *xmllist) {
	
	/* The attributes expander */
	GtkWidget *file_expander; 
	file_expander = gtk_expander_new("XPath");
	gtk_expander_set_expanded(GTK_EXPANDER(file_expander), TRUE);

	GtkWidget * file_frame;
	file_frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(file_frame), GTK_SHADOW_IN);
	//gtk_frame_set_label_widget(GTK_FRAME(file_frame),frame_label_widget);
		
	GtkWidget *hbox, *xlabel;
	hbox = gtk_hbox_new(FALSE, 3);
	xlabel = gtk_label_new("XPath:");
	gtk_box_pack_start(GTK_BOX(hbox), xlabel,FALSE , TRUE, 0);
	ttt->xpath_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), ttt->xpath_entry, TRUE, TRUE, 0);
			gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);

	gtk_container_add(GTK_CONTAINER(file_expander), hbox);
	gtk_container_add(GTK_CONTAINER(file_frame), file_expander);

	return file_frame;
}
