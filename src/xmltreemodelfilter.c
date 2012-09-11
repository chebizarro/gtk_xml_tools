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
			ttt_type = g_type_register_static (GTK_TYPE_TREE_MODEL_FILTER, "xmlTreeModelFilter", &ttt_info, 0);
		}
	return ttt_type;
}

static void
xml_tree_model_filter_class_init (xmlTreeModelFilterClass *klass)
{
	xml_tree_model_filter_signals[XML_TREE_MODEL_FILTER_SIGNAL] = g_signal_new ("xml_tree_model_filter",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
												G_STRUCT_OFFSET (xmlTreeModelFilterClass, xml_tree_model_filter),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__VOID,
												G_TYPE_NONE, 0);
}

static void
xml_tree_model_filter_init (xmlTreeModelFilter *ttt)
{
}

GtkWidget*
xml_tree_model_filter_new ()
{
	return GTK_WIDGET (g_object_new (xml_tree_model_filter_get_type (), NULL));
}

