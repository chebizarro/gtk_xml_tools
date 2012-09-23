/*
 *	Custom Widget Template
 *
 */

#include "xslttransformer.h"

enum {
	XSLT_MODEL_CHANGED,
	XSLT_MODEL_TRANSFORMED,
	XSLT_MENU_ACTIVATED,
	XSLT_LAST_SIGNAL
};

static void xslt_transformer_class_init		(xsltTransformerClass	*klass);
static void xslt_transformer_init				(xsltTransformer		*ttt);

static GtkWidget * make_view (xsltTransformer *ttt);
static GtkWidget * make_param_expander (xsltTransformer * ttt);
static GtkWidget * make_param_view (xsltTransformer * ttt);

static void		xslt_entry_icon_press_cb (	GtkEntry		* entry,
							gint            position,
							GdkEventButton	*event,
							xsltTransformer	*ttt);


static guint xslt_transformer_signals[XSLT_LAST_SIGNAL] = { 0 };


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

GType
xslt_transformer_get_type (void)
{
	static GType ttt_type = 0;
	if (!ttt_type)
		{
			const GTypeInfo ttt_info =
			{
				sizeof (xsltTransformerClass),
				NULL, /* base_init */
				NULL, /* base_finalize */
				(GClassInitFunc) xslt_transformer_class_init,
				NULL, /* class_finalize */
				NULL, /* class_data */
				sizeof (xsltTransformer),
				0,
				(GInstanceInitFunc) xslt_transformer_init,
			};
			ttt_type = g_type_register_static (GTK_TYPE_BOX, "xsltTransformer", &ttt_info, 0);
		}
	return ttt_type;
}

static void
xslt_transformer_class_init (xsltTransformerClass *klass)
{
	
											
	xslt_transformer_signals[XSLT_MODEL_CHANGED] = g_signal_new ("xslt-transformer-model-changed",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (xsltTransformerClass, xslt_model_changed),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

	xslt_transformer_signals[XSLT_MENU_ACTIVATED] = g_signal_new ("xslt-transformer-menu-activated",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (xsltTransformerClass, xsl_menu_activated),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);


	xslt_transformer_signals[XSLT_MODEL_TRANSFORMED] = g_signal_new ("xslt-transformer-transformed",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST,
												G_STRUCT_OFFSET (xsltTransformerClass, xslt_model_transformed),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__POINTER,
												G_TYPE_NONE, 1, G_TYPE_POINTER);

}


GtkWidget*
xslt_transformer_new ()
{
	return GTK_WIDGET (g_object_new (xslt_transformer_get_type (), "orientation", GTK_ORIENTATION_VERTICAL, NULL));
}


static GtkEntry *
make_xslt_entry (xsltTransformer *ttt)
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

void
on_xslt_params_cell_edited (GtkCellRendererText *cell,
                            gchar               *path_string,
                            gchar               *new_text,
                            xsltTransformer        *ttt)
{

	GtkTreeModel * model = (GtkTreeModel *)gtk_tree_view_get_model(ttt->parameters);
	GtkTreePath	 * path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter  iter;
	
	//gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));
	
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, new_text, -1);

	gtk_tree_path_free (path);
	
}

static GtkWidget *
make_xslt_param_view (xsltTransformer * ttt)
{
	//g_return_val_if_fail(ttt != NULL, NULL);
	
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
	g_signal_connect(renderer, "edited", (GCallback) on_xslt_params_cell_edited, ttt);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_add_attribute (col, renderer, "text", 1);
	gtk_tree_view_column_set_title (col, "Value");
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view),col);

	return view;
}


void xslt_add_file_menu_action(GtkWidget *source, xsltTransformer * ttt){
	gchar 			* file_path = NULL;
	GtkListStore	* parameters;
	
	file_path = show_open_dialog();
	
	if(file_path == NULL)
		return;

	ttt->stylesheet = xml_tree_model_new();
	xml_tree_model_add_file(ttt->stylesheet,file_path);
	
	parameters = xml_tree_model_get_stylesheet_params (ttt->stylesheet);
	
	gtk_tree_view_set_model(ttt->parameters, parameters); 

	gtk_entry_set_text(ttt->entry, file_path);

	//g_object_unref(ttt->stylesheet);
}

static gboolean
xslt_add_params(xsltTransformer 	* ttt,
                xmlTreeModel 	* xmltreemodel,
	            GtkTreeView 	* treeview)
{
	if(xmltreemodel->xsldoc != NULL) {
		gtk_tree_view_set_model(GTK_TREE_VIEW(treeview),
								GTK_TREE_MODEL(xml_tree_model_get_stylesheet_params (ttt->model))); 
		g_object_unref(ttt->model);
	} else {
		gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), NULL);
	}
}

void
xslt_transformer_set_model(xsltTransformer *ttt, xmlTreeModel * xmltreemodel)
{
	ttt->model = xmltreemodel;
	g_signal_emit(ttt, xslt_transformer_signals[XSLT_MODEL_CHANGED],0,xmltreemodel);
}


static void
xslt_transformer_init (xsltTransformer *ttt)
{

	GtkWidget	*scrolled_window;
	GtkWidget 	*vbox;
	GtkWidget	*hbox;
	GtkLabel	*label;
	GtkFrame	*frame;
	
	label = gtk_label_new(_("Template"));

	ttt->entry = make_xslt_entry(ttt);
 
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label), FALSE, FALSE, 5);
 	gtk_box_pack_start(GTK_BOX(hbox), ttt->entry, TRUE, TRUE, 0);

    scrolled_window = make_scrolled_window();

    ttt->parameters = make_xslt_param_view(ttt);

    vbox = gtk_vbox_new(TRUE, 3);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), vbox);
	gtk_container_add(GTK_CONTAINER(vbox), ttt->parameters);

	frame = gtk_frame_new(_("Parameters"));
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

	gtk_box_pack_start(GTK_BOX(ttt), GTK_WIDGET(hbox), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ttt), GTK_WIDGET(frame), TRUE, TRUE, 0);
	
	gtk_container_set_border_width(GTK_CONTAINER(ttt), 6);

	g_signal_connect(ttt, "xslt-transformer-model-changed", G_CALLBACK(xslt_add_params), ttt->parameters);

}


void xslt_transformer_set_stylesheet(xsltTransformer *ttt, xmlTreeModel * xmltreemodel)
{
	gchar 			* file_path = NULL;
	GtkListStore	* parameters;

	ttt->stylesheet = xmltreemodel;

	if(xml_tree_model_is_stylesheet(ttt->stylesheet)) {
		parameters = xml_tree_model_get_stylesheet_params (ttt->stylesheet);
		gtk_tree_view_set_model(ttt->parameters, parameters); 
	} else {
		gtk_tree_view_set_model(ttt->parameters, NULL);
	}

	gtk_entry_set_text(ttt->entry, ttt->stylesheet->filename);

}


static void
xslt_entry_icon_press_cb (	GtkEntry		* entry,
							gint            position,
							GdkEventButton	*event,
							xsltTransformer	*ttt)
{

	/* Sub menu for file button */
	GtkWidget *file_menu, *open_item, *items;
	file_menu = gtk_menu_new();

	open_item = gtk_menu_item_new_with_label("Add File");
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
	g_signal_connect(open_item, "activate", G_CALLBACK(xslt_add_file_menu_action), ttt);
	gtk_widget_show (open_item);
	
	items = gtk_menu_new();

	open_item = gtk_menu_item_new_with_label("Open Files...");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(open_item),items);
	gtk_widget_show (open_item);

	g_signal_emit(ttt, xslt_transformer_signals[XSLT_MENU_ACTIVATED],0,items);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);

	//g_signal_connect(open_item, "activate", G_CALLBACK(xslt_add_file_items), ttt);
	
	gtk_menu_popup(file_menu, NULL, NULL, NULL, NULL, event->button, event->time);

}

gboolean
xslt_params_to_hash_table(	GtkTreeModel *model,
                            GtkTreePath *path,
                            GtkTreeIter *iter,
                            GHashTable * params)
{
	gchar * value;
	gchar * key;
	
	gtk_tree_model_get(model, iter, 1, &value, -1);

	if(strlen(value) > 0) {
		gtk_tree_model_get(model, iter, 0, &key, -1);
		g_hash_table_insert(params, key, value);
	}
	
	return FALSE;
}

void 
xslt_transform ( GtkWidget * button, xsltTransformer * ttt)
{
	GHashTable	* params = g_hash_table_new(NULL, NULL);

	GtkTreeModel * model = (GtkTreeModel *)gtk_tree_view_get_model(ttt->parameters);

	gtk_tree_model_foreach(model, xslt_params_to_hash_table, params);

	if(!xml_tree_model_is_stylesheet(ttt->stylesheet) && !xml_tree_model_is_stylesheet(ttt->model))
		return;

	if(xml_tree_model_is_stylesheet(ttt->stylesheet)) {
		ttt->result = xml_tree_model_transform(ttt->model, ttt->stylesheet, params);
	} else {
		ttt->result = xml_tree_model_transform(ttt->stylesheet, ttt->model, params);
	}

	g_signal_emit(ttt, xslt_transformer_signals[XSLT_MODEL_TRANSFORMED],0,ttt->result);
}
