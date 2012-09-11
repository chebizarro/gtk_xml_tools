/*
 *	Custom Widget Template
 *
 */

#ifndef __XML_TREE_VIEW_H__
#define __XML_TREE_VIEW_H__

#include <gtk/gtk.h>
#include "xmltreemodel.h"

G_BEGIN_DECLS

#define XML_TREE_VIEW_TYPE				(xml_tree_view_get_type ())
#define XML_TREE_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_TREE_VIEW_TYPE, xmlTreeView))
#define XML_TREE_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XML_TREE_VIEW_TYPE, xmlTreeViewClass))
#define IS_XML_TREE_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_TREE_VIEW_TYPE))
#define IS_XML_TREE_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XML_TREE_VIEW_TYPE))

typedef struct _xmlTreeView		xmlTreeView;
typedef struct _xmlTreeViewClass	xmlTreeViewClass;

struct _xmlTreeView
{
	GtkTreeView treeView;

};

struct _xmlTreeViewClass
{
	GtkTableClass parent_class;

	void (* xml_tree_view) (xmlTreeView *ttt);
};

GType			xml_tree_view_get_type				(void);
GtkWidget*		xml_tree_view_new					(void);


G_END_DECLS

#endif /* __XML_TREE_VIEW_H__ */
