/*
 *	Custom Widget Template
 *
 */

#include "xmltreeview.h"

enum {
	XML_TREE_VIEW_SIGNAL,
	LAST_SIGNAL
};

static void xml_tree_view_class_init		(xmlTreeViewClass *klass);
static void xml_tree_view_init				(xmlTreeView			*ttt);


static guint xml_tree_view_signals[LAST_SIGNAL] = { 0 };

GType
xml_tree_view_get_type (void)
{
	static GType ttt_type = 0;
	if (!ttt_type)
		{
			const GTypeInfo ttt_info =
			{
				sizeof (xmlTreeViewClass),
				NULL, /* base_init */
				NULL, /* base_finalize */
				(GClassInitFunc) xml_tree_view_class_init,
				NULL, /* class_finalize */
				NULL, /* class_data */
				sizeof (xmlTreeView),
				0,
				(GInstanceInitFunc) xml_tree_view_init,
			};
			ttt_type = g_type_register_static (GTK_TYPE_TREE_VIEW, "xmlTreeView", &ttt_info, 0);
		}
	return ttt_type;
}

static void
xml_tree_view_class_init (xmlTreeViewClass *klass)
{
	xml_tree_view_signals[XML_TREE_VIEW_SIGNAL] = g_signal_new ("xml_tree_view",
												G_TYPE_FROM_CLASS (klass),
												G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
												G_STRUCT_OFFSET (xmlTreeViewClass, xml_tree_view),
												NULL, 
												NULL,								
												g_cclosure_marshal_VOID__VOID,
												G_TYPE_NONE, 0);
}

static void
xml_tree_view_init (xmlTreeView *ttt)
{
}

GtkWidget*
xml_tree_view_new ()
{
	return GTK_WIDGET (g_object_new (xml_tree_view_get_type (), NULL));
}


