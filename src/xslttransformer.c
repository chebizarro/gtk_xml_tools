/*
 *	Custom Widget Template
 *
 */

#include "xslttransformer.h"

enum {
	XSLT_TRANSFORMER_SIGNAL,
	LAST_SIGNAL
};

static void xslt_transformer_class_init		(xsltTransformerClass	*klass);
static void xslt_transformer_init				(xsltTransformer			*ttt);

static GtkWidget * make_view (xsltTransformer *ttt);
static GtkWidget * make_param_expander (xsltTransformer * ttt);
static GtkWidget * make_param_view (xsltTransformer * ttt);

static guint xslt_transformer_signals[LAST_SIGNAL] = { 0 };

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
	xslt_transformer_signals[XSLT_TRANSFORMER_SIGNAL] = g_signal_new ("xslt_transformer",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
												G_STRUCT_OFFSET (xsltTransformerClass, xslt_transformer),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__VOID,
												G_TYPE_NONE, 0);
}

static void
xslt_transformer_init (xsltTransformer *ttt)
{

	gtk_box_pack_start(GTK_BOX(ttt), make_view(ttt), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ttt), make_param_expander(ttt), FALSE, FALSE, 0);
	
}

GtkWidget*
xslt_transformer_new ()
{
	return GTK_WIDGET (g_object_new (xslt_transformer_get_type (), "orientation", GTK_ORIENTATION_VERTICAL, NULL));
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

static void add_file(GtkWidget *source, xsltTransformer * ttt) {
	gchar *file_path = NULL;

	file_path = show_open_dialog();
	
	if(file_path == NULL)
		return;

	xsltStylesheetPtr xslCur;
	xslCur = xsltParseStylesheetFile((xmlChar *)file_path);

	gtk_entry_set_text (ttt->file_entry, file_path);

	if(xslCur != NULL) {
		GtkListStore *list_store;
		GtkTreeIter iter;
		gint i, size, column;
		xsltStackElemPtr stack;
		
		list_store = gtk_list_store_new (2,G_TYPE_STRING, G_TYPE_STRING);
		stack = xslCur->variables; 
		
		while(stack != NULL) {

			if(stack->comp->type == XSLT_FUNC_PARAM) {
				gtk_list_store_append (list_store, &iter);

				gtk_list_store_set(GTK_LIST_STORE(list_store), &iter,
									0, stack->name,
									1, "",
									-1);
			}
			stack = stack->next;
		}

		gtk_tree_view_set_model(GTK_TREE_VIEW(ttt->param_list_box), GTK_TREE_MODEL(list_store));	
		gtk_expander_set_expanded(gtk_widget_get_parent(ttt->param_list_box), TRUE);
		xsltFreeStylesheet(xslCur);
	}
	
	xmlErrorPtr err = xmlGetLastError();
	return NULL;
}

static GtkWidget *
make_param_view (xsltTransformer * ttt)
{
	g_return_val_if_fail(ttt != NULL, NULL);
	
	GtkTreeViewColumn	*col;
	GtkCellRenderer		*renderer, *icon_renderer;
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

static GtkWidget *
make_param_expander (xsltTransformer * ttt)
{
	/* The paramters expander */
	GtkWidget *param_expander; 
	param_expander = gtk_expander_new("Parameters");
	gtk_expander_set_expanded(GTK_EXPANDER(param_expander), FALSE);
	//gtk_box_pack_start(GTK_BOX(ttt), param_expander, TRUE, TRUE, 0);
	//gtk_container_add(GTK_CONTAINER(ttt), param_expander);
	ttt->param_list_box = make_param_view(ttt);
	gtk_container_add(GTK_CONTAINER(param_expander), ttt->param_list_box);
	return param_expander;
}
	
static GtkWidget *
make_view (xsltTransformer *ttt) {
	
	GtkWidget * xframe;
	xframe = gtk_vbox_new(TRUE, 3);

	GtkWidget *hbox;
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
		
	/* Label and entry */
	GtkWidget *xlabel;
	xlabel = gtk_label_new("XSLT File:");
	gtk_box_pack_start(GTK_BOX(hbox), xlabel,FALSE, TRUE, 0);
	ttt->file_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), ttt->file_entry, TRUE, TRUE, 0);

	/* Create the toolbar object and initialise it*/
	GtkWidget *toolbar;
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	
	/* File Select button */
	GtkToolButton * selectfile;
	selectfile = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_OPEN));
	g_signal_connect(selectfile, "clicked", G_CALLBACK(add_file), ttt);
	//gtk_container_add(GTK_CONTAINER(toolbar), selectfile);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(selectfile), FALSE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(xframe), hbox);



	return xframe;
}
