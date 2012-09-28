#ifndef _xml_tree_model_h_included_
#define _xml_tree_model_h_included_
 
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

/* XML/XSLT Libraries */
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
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
	XML_TREE_MODEL_COL_POS,
	XML_TREE_MODEL_COL_XPATH,
	XML_TREE_MODEL_N_COLUMNS,
};

#define XML_N_NODE_TYPES 22
#define XML_TREE_MESSAGE "xml_tree_message"


typedef struct _xslErrorMessage		 xslErrorMessage;

struct _xslErrorMessage
{
	gchar	* error;
	gchar	* file;
	gint	line;
	gchar	* element;
};

typedef struct _xmlTreeModel			 xmlTreeModel;
typedef struct _xmlTreeModelClass	xmlTreeModelClass;

 
/* xmlTreeModel: this structure contains everything we need for our
 *						 model implementation. It is
 *						 crucial that 'parent' is the first member of the
 *						 structure.																					*/
 
struct _xmlTreeModel
{
	GObject				parent;			/* this MUST be the first member */
 
	xmlDocPtr			xmldoc;

	xsltStylesheetPtr	xsldoc;
	
	xmlParserCtxtPtr	parser;

	GList				*nodeinfo;

	gchar				*xpath;
	
	gchar				*filename;
	
	gboolean			valid;
	
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
	
	/* Signals */
	void (* xml_tree_model_changed)	(xmlTreeModel *ttt);
	void (* xml_tree_model_error)	(xmlTreeModel *ttt);
	void (* xml_tree_model_xsl_error)	(xmlTreeModel *ttt);

};
 
 
GType	xml_tree_model_get_type (void);


xmlTreeModel	*xml_tree_model_new (void);

void	xml_tree_model_add_file (xmlTreeModel   *xml_tree_model, gchar  *filename);

GtkListStore * xml_tree_model_get_xpath_results(xmlTreeModel *xmllist, gchar *xpath);

gboolean	xml_tree_model_validate(xmlTreeModel *tree_model);

GtkListStore * xml_tree_model_get_stylesheet_params(xmlTreeModel *xmltreemodel);

xmlTreeModel *xml_tree_model_transform (xmlTreeModel * xml, xmlTreeModel * xslt, GHashTable *params);

gint xml_tree_model_write_to_file(xmlTreeModel * ttt, gint fd, gint format);

gboolean xml_tree_model_is_stylesheet(xmlTreeModel *ttt); 

void	xml_tree_model_reload (xmlTreeModel *xmltreemodel);

GtkTreePath * xml_tree_model_get_path_from_xpath(xmlTreeModel *ttt, gchar *xpath);

GtkTreePath * xml_tree_model_get_path_from_position(xmlTreeModel *ttt, gint position);


#endif /* _xml_tree_model_h_included_ */
