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

static void		xslt_add_file_cb (	GtkWidget 			*button,
										xsltTransformer	*ttt);



void xslt_transform ( GtkWidget * button, xsltTransformer * ttt);

static guint xslt_transformer_signals[XSLT_LAST_SIGNAL] = { 0 };


static GtkFileFilter *make_xml_file_filter(){
	GtkFileFilter *new_filter;
	new_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(new_filter, _("XML Files"));
	gtk_file_filter_add_mime_type(new_filter, "text/xml");
	gtk_file_filter_add_pattern(new_filter, "*.xml");
	gtk_file_filter_add_pattern(new_filter, "*.sgml");
	gtk_file_filter_add_pattern(new_filter, "*.xsl");
	gtk_file_filter_add_pattern(new_filter, "*.xslt");
	gtk_file_filter_add_pattern(new_filter, "*.xsd");
	gtk_file_filter_add_pattern(new_filter, "*.xhtml");
	return new_filter;
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

static gchar *
show_save_dialog()
{
	GtkWidget *dialog;
	gchar *filename = NULL;
	dialog = gtk_file_chooser_dialog_new(_("Choose a destination"),
						  NULL,
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						  NULL);
						  
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), make_xml_file_filter());

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
	}
	gtk_widget_destroy(dialog);
	return filename;
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
						  
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), make_xml_file_filter());

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

void xslt_save_file (GtkWidget *source, xsltTransformer * ttt)
{
	gchar *filepath = NULL;
	filepath = show_save_dialog();

	g_return_if_fail(filepath != NULL);
	
	gtk_entry_set_text(ttt->results_entry, filepath);
	ttt->results_path = g_strdup(filepath);

	g_free(filepath);

}

void xslt_add_file_menu_action(GtkWidget *source, xsltTransformer * ttt){
	gchar 			* file_path = NULL;
	GtkListStore	* parameters;
	
	file_path = show_open_dialog();
	
	if(file_path == NULL)
		return;

	xml_tree_model_add_file(ttt->stylesheet,file_path);
	
	parameters = xml_tree_model_get_stylesheet_params (ttt->stylesheet);
	
	gtk_tree_view_set_model(ttt->parameters, parameters); 

	gtk_entry_set_text(ttt->entry, file_path);

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
xslt_set_label (xsltTransformer	*ttt,
				xmlTreeModel	 	*model,
				GtkLabel	 		*label)
{
	if(xml_tree_model_is_stylesheet(model)) {
		gtk_label_set_text(label, _("Source:"));
	}
}

void
xslt_transformer_set_model(xsltTransformer *ttt, xmlTreeModel * xmltreemodel)
{
	ttt->model = xmltreemodel;
	g_signal_emit(ttt, xslt_transformer_signals[XSLT_MODEL_CHANGED],0,xmltreemodel);
}

GtkWidget *
make_toolbar(xsltTransformer *ttt)
{
	GtkToolbar	*toolbar;
	GtkWidget	*button;
	
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

	button = gtk_menu_tool_button_new_from_stock("gtk-open");
	g_signal_connect(button, "clicked", G_CALLBACK(xslt_add_file_menu_action), ttt);

	gtk_toolbar_insert(toolbar, button,-1);
	/* Sub menu for file button */
	GtkWidget *file_menu, *open_item;
	file_menu = gtk_menu_new();
	open_item = gtk_menu_item_new_with_label(_("Add File"));
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
	g_signal_connect(open_item, "activate", G_CALLBACK(xslt_add_file_menu_action), ttt);
	gtk_widget_show (open_item);

	open_item = gtk_menu_item_new_with_label(_("Open Files..."));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(open_item),gtk_menu_new());
	gtk_widget_show (open_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
	g_signal_connect (open_item, "activate", G_CALLBACK (xslt_add_file_cb), ttt);

	gtk_menu_tool_button_set_menu(button, file_menu);

	button = gtk_tool_button_new_from_stock("gtk-save-as");
	gtk_toolbar_insert(toolbar, button,-1);
	g_signal_connect(button, "clicked", G_CALLBACK(xslt_save_file), ttt);

	button = gtk_tool_button_new_from_stock("gtk-media-play");
	gtk_toolbar_insert(toolbar, button,-1);
	g_signal_connect(button, "clicked", G_CALLBACK(xslt_transform), ttt);

	return toolbar;
}

static void
xslt_transformer_init (xsltTransformer *ttt)
{

	GtkWidget	*scrolled_window;
	GtkWidget 	*paramvbox;
	GtkWidget	*entryhbox;
	GtkLabel	*label;
	GtkFrame	*frame;
	GtkWidget	*panel;
	GtkToolbar	*toolbar;
	
	toolbar = make_toolbar(ttt);
	
	/* Entry bar and label (and buttons? */
	label = gtk_label_new(_("Template:"));
	g_signal_connect(ttt, "xslt-transformer-model-changed", G_CALLBACK(xslt_set_label), label);
	
	ttt->entry = make_xslt_entry(ttt);
	ttt->results_nav = xml_navigator_new();
	ttt->stylesheet = xml_tree_model_new();
	ttt->results_entry = gtk_entry_new();
	ttt->results_path = NULL;
	
    paramvbox = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(paramvbox), GTK_WIDGET(toolbar), FALSE, FALSE, 0);
	
	entryhbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(entryhbox), GTK_WIDGET(label), FALSE, FALSE, 0);
 	gtk_box_pack_start(GTK_BOX(entryhbox), ttt->entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(paramvbox), GTK_WIDGET(entryhbox), FALSE, FALSE, 0);

	entryhbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(entryhbox), GTK_WIDGET(gtk_label_new(_("Output:"))), FALSE, FALSE, 0);
 	gtk_box_pack_start(GTK_BOX(entryhbox), ttt->results_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(paramvbox), GTK_WIDGET(entryhbox), FALSE, FALSE, 0);

    ttt->parameters = make_xslt_param_view(ttt);
    scrolled_window = make_scrolled_window();
	gtk_container_add(GTK_CONTAINER(scrolled_window), ttt->parameters);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 6);
	frame = gtk_frame_new(_("Parameters"));
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
	gtk_box_pack_start(GTK_BOX(paramvbox), GTK_WIDGET(frame), TRUE, TRUE, 0);

	frame = gtk_frame_new(_("Results"));
    gtk_container_add(GTK_CONTAINER(frame), ttt->results_nav);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

	panel = gtk_hbox_new(FALSE,6);
	gtk_box_pack_start(GTK_BOX(panel), GTK_WIDGET(paramvbox), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(panel), GTK_WIDGET(frame), TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(ttt), GTK_WIDGET(panel), TRUE, TRUE, 0);
	
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

void xslt_add_file_items(GtkMenuItem *item, xsltTransformer * ttt)
{
 GtkWidget * files_menu = gtk_menu_item_get_submenu(item);
}

static void
xslt_add_file_cb (GtkWidget 		*button,
                  xsltTransformer	*ttt)
{
	GtkWidget *items;
	items = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(button),items);

	g_signal_emit(ttt, xslt_transformer_signals[XSLT_MENU_ACTIVATED],0,items);
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

	xml_navigator_set_model(ttt->results_nav, ttt->result);

	g_return_if_fail(ttt->results_path != NULL);

	xml_tree_model_save_xsl(ttt->result, ttt->results_path);

	g_signal_emit(ttt, xslt_transformer_signals[XSLT_MODEL_TRANSFORMED],0,ttt->results_path);
}
