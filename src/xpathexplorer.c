/*
 *	Custom Widget Template
 *
 */

#include "xpathexplorer.h"

enum {
	XPATH_EXPLORER_SIGNAL,
	LAST_SIGNAL
};

static void xpath_explorer_class_init		(XpathExplorerClass *klass);
static void xpath_explorer_init				(XpathExplorer			*ttt);

static GtkWidget * make_xpath_entry(XpathExplorer *ttt, xmlTreeModel * xmltreemodel);

static guint xpath_explorer_signals[LAST_SIGNAL] = { 0 };

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
	xpath_explorer_signals[XPATH_EXPLORER_SIGNAL] = g_signal_new ("xpath_explorer",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
												G_STRUCT_OFFSET (XpathExplorerClass, xpath_explorer),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__VOID,
												G_TYPE_NONE, 0);
}

static void
xpath_explorer_init (XpathExplorer *ttt)
{
	ttt->xpath_view = make_xpath_entry(ttt, NULL);
    gtk_box_pack_start(GTK_BOX(ttt), ttt->xpath_view, FALSE, FALSE, 0);
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

static void
run_xpath(GtkButton * runxpath, XpathExplorer *ttt ) {
	GtkListStore * xpath_result;
	const gchar * xpath = gtk_entry_get_text(GTK_ENTRY(ttt->xpath_entry));	
	//xpath_result = xml_get_xpath_results(ttt->model, xpath);
	//gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->xpath_results), GTK_TREE_MODEL(xpath_result));
}

static GtkWidget *
make_xpath_entry (XpathExplorer *ttt, xmlTreeModel *xmltreemodel) {
	
	GtkWidget * xpath_frame;
	xpath_frame = gtk_vbox_new(TRUE, 3);

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
	GtkToolButton * runxpath;
	runxpath = gtk_tool_button_new_from_stock("gtk-find");
	g_signal_connect(runxpath, "clicked", G_CALLBACK(run_xpath), ttt);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(runxpath), FALSE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(xpath_frame), hbox);

	/* The results expander */
	GtkWidget *results_expander; 
	results_expander = gtk_expander_new("Results");
	gtk_expander_set_expanded(GTK_EXPANDER(results_expander), TRUE);
	//gtk_container_add(GTK_CONTAINER(xpath_frame), results_expander);

	GtkWidget *scrolled_window;
	scrolled_window = make_scrolled_window();
    gtk_box_pack_start (GTK_BOX(xpath_frame), GTK_WIDGET(scrolled_window), TRUE, TRUE, 0);
	//gtk_container_add(GTK_CONTAINER(results_expander), scrolled_window);

	GtkWidget * view_vbox = gtk_vbox_new(TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), view_vbox);

	//ttt->xpath_results = make_navigator_view(ttt);
	//gtk_container_add(GTK_CONTAINER(view_vbox), ttt->xpath_results);	

	return xpath_frame;
}
