/*
 *	Custom Widget Template
 *
 */

#ifndef __XML_TREE_MODEL_FILTER_H__
#define __XML_TREE_MODEL_FILTER_H__

#include <gtk/gtk.h>
#include "xmltreemodel.h"

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

G_BEGIN_DECLS

#define XML_TREE_MODEL_FILTER_TYPE				(xml_tree_model_filter_get_type ())
#define XML_TREE_MODEL_FILTER(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_TREE_MODEL_FILTER_TYPE, xmlTreeModelFilter))
#define XML_TREE_MODEL_FILTER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XML_TREE_MODEL_FILTER_TYPE, xmlTreeModelFilterClass))
#define IS_XML_TREE_MODEL_FILTER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_TREE_MODEL_FILTER_TYPE))
#define IS_XML_TREE_MODEL_FILTER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XML_TREE_MODEL_FILTER_TYPE))

typedef struct _xmlTreeModelFilter			 xmlTreeModelFilter;
typedef struct _xmlTreeModelFilterClass	xmlTreeModelFilterClass;

struct _xmlTreeModelFilter
{
	GObject		parent;

	xmlTreeModel		*model;
	GtkTreePath 		*root;
	
	gboolean			row_visible[XML_N_NODE_TYPES];

};

struct _xmlTreeModelFilterClass
{
	GObjectClass parent_class;
};

GType			xml_tree_model_filter_get_type		(void);
GtkWidget*		xml_tree_model_filter_new			(GtkTreeModel *child_model, GtkTreePath *root);

void			xml_tree_model_filter_set_visible	(xmlTreeModelFilter *xmltreemodel, xmlElementType nodetype, gboolean visible);
gboolean		xml_tree_model_filter_get_visible 	(xmlTreeModelFilter *xmltreemodel, xmlElementType nodetype);


G_END_DECLS

#endif /* __XML_TREE_MODEL_FILTER_H__ */
