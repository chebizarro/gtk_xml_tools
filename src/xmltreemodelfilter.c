/*
 *	Custom Widget Template
 *
 */

#include "xmltreemodelfilter.h"

enum {
	XML_TREE_MODEL_FILTER_SIGNAL,
	LAST_SIGNAL
};

static void xml_tree_model_filter_class_init		(xmlTreeModelFilterClass *klass);
static void xml_tree_model_filter_init				(xmlTreeModelFilter			*ttt);

static guint xml_tree_model_filter_signals[LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;  /* GObject stuff - nothing to worry about */

GType
xml_tree_model_filter_get_type (void)
{
	static GType ttt_type = 0;
	if (!ttt_type)
		{
			const GTypeInfo ttt_info =
			{
				sizeof (xmlTreeModelFilterClass),
				NULL, /* base_init */
				NULL, /* base_finalize */
				(GClassInitFunc) xml_tree_model_filter_class_init,
				NULL, /* class_finalize */
				NULL, /* class_data */
				sizeof (xmlTreeModelFilter),
				0,
				(GInstanceInitFunc) xml_tree_model_filter_init,
			};
			ttt_type = g_type_register_static (G_TYPE_OBJECT, "xmlTreeModelFilter", &ttt_info, 0);
		}
	return ttt_type;
}

static void
xml_tree_model_filter_finalize (GObject *object)
{
	/* must chain up - finalize parent */
	(* parent_class->finalize) (object);
}
 

static void
xml_tree_model_filter_class_init (xmlTreeModelFilterClass *klass)
{
	GObjectClass *object_class;
 	parent_class = (GObjectClass*) g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
 	object_class->finalize = xml_tree_model_filter_finalize;
}

static void
show_visible(GtkTreeModel 		*model,
             GtkTreeIter  		*iter,
             xmlTreeModelFilter *filter)
{
	gint row = 0;
	gboolean visible = FALSE;

	gtk_tree_model_get (model, iter, XML_TREE_MODEL_COL_TYPE, &row, -1);

	if (row > 0 && row < XML_N_NODE_TYPES)
		visible = filter->row_visible[row];

	return visible;
}

static void
xml_tree_model_filter_init (xmlTreeModelFilter *ttt)
{



	ttt->row_visible[XML_ELEMENT_NODE] = TRUE;
	ttt->row_visible[XML_DOCUMENT_NODE] = TRUE;
	ttt->row_visible[XML_ATTRIBUTE_NODE] = TRUE;
	ttt->row_visible[XML_TEXT_NODE] = FALSE;

	ttt = gtk_tree_model_filter_new(ttt->model,ttt->root);	
}

GtkWidget*
xml_tree_model_filter_new (GtkTreeModel *child_model,
                           GtkTreePath *root)
{
	xmlTreeModelFilter * ttt;

	ttt->model = child_model;
	ttt->root = root;	
	
	ttt = (xmlTreeModelFilter *)g_object_new (xml_tree_model_filter_get_type (), ttt);

	
	gtk_tree_model_filter_set_visible_func(ttt, show_visible, ttt, NULL);
	
	return ttt;
}

void
xml_tree_model_filter_set_visible (xmlTreeModelFilter *ttt, xmlElementType nodetype, gboolean visible)
{
	g_return_if_fail (IS_XML_TREE_MODEL_FILTER(ttt));
	ttt->row_visible[nodetype] = visible;
}

gboolean
xml_tree_model_filter_get_visible (xmlTreeModelFilter *ttt, xmlElementType nodetype)
{
	g_return_val_if_fail(IS_XML_TREE_MODEL_FILTER(ttt), FALSE);
	return ttt->row_visible[nodetype];
}
