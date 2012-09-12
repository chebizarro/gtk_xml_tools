#ifndef _xml_tree_model_h_included_
#define _xml_tree_model_h_included_
 
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

/* XML/XSLT Libraries */
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

/* Some boilerplate GObject defines. 'klass' is used
 *	 instead of 'class', because 'class' is a C++ keyword */
 
#define XML_TYPE_TREE_MODEL				(xml_tree_model_get_type ())
#define XML_TREE_MODEL(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_TYPE_TREE_MODEL, xmlTreeModel))
#define XML_TREE_MODEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass),	XML_TYPE_TREE_MODEL, xmlTreeModelClass))
#define XML_IS_TREE_MODEL(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_TYPE_TREE_MODEL))
#define XML_IS_TREE_MODEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	XML_TYPE_TREE_MODEL))
#define XML_TREE_MODEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),	XML_TYPE_TREE_MODEL, xmlTreeModelClass))
 
/* The data columns that we export via the tree model interface */
 
enum
{
	XML_TREE_MODEL_COL_TYPE = 0,
	XML_TREE_MODEL_COL_NS,
	XML_TREE_MODEL_COL_NAME,
	XML_TREE_MODEL_COL_CONTENT,
	XML_TREE_MODEL_COL_LINE,
	XML_TREE_MODEL_COL_VISIBLE,
	XML_TREE_MODEL_COL_XPATH,
	XML_TREE_MODEL_N_COLUMNS,
};

#define XML_N_NODE_TYPES 22

typedef struct _xmlTreeModel			 xmlTreeModel;
typedef struct _xmlTreeModelClass	xmlTreeModelClass;

 
/* xmlTreeModel: this structure contains everything we need for our
 *						 model implementation. It is
 *						 crucial that 'parent' is the first member of the
 *						 structure.																					*/
 
struct _xmlTreeModel
{
	GObject		parent;			/* this MUST be the first member */
 
	xmlDocPtr	xmldoc;
	
	gboolean	row_visible[XML_N_NODE_TYPES];

	gchar		*xpath;
	
	gchar		*filename;
	
	/* These two fields are not absolutely necessary, but they		*/
	/*	 speed things up a bit in our get_value implementation		*/
	gint		n_columns;
	GType		column_types[XML_TREE_MODEL_N_COLUMNS];
 
	gint		stamp;			 /* Random integer to check whether an iter belongs to our model */
};
 
 
 
/* xmlTreeModelClass: more boilerplate GObject stuff */
 
struct _xmlTreeModelClass
{
	GObjectClass parent_class;
};
 
 
GType	xml_tree_model_get_type (void);

//void	xml_type_register_type(GTypeModule *type_module);
 
xmlTreeModel	*xml_tree_model_new (void);

void	xml_tree_model_add_file (xmlTreeModel   *xml_tree_model, gchar  *filename);

void	xml_tree_model_set_visible (xmlTreeModel *xmllist, xmlElementType nodetype, gboolean visible);

gboolean	xml_tree_model_get_visible (xmlTreeModel *xmllist, xmlElementType nodetype);

GtkListStore * xml_get_xpath_results(xmlTreeModel *xmllist, gchar *xpath);

gboolean	xml_tree_model_validate(xmlTreeModel *tree_model);
                      
#endif /* _xml_tree_model_h_included_ */
