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
	XML_TREE_MODEL_N_COLUMNS
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


typedef enum {
    XML_PURGE_FILE			= 1<<0,
    XML_NOTIFY_ON_CHANGE	= 1<<1
} xmlTreeModelOption;

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
	
	gint				offset;

	xmlTreeModelOption	options;
	
	/* These two fields are not absolutely necessary, but they		*/
	/*	 speed things up a bit in our get_value implementation		*/
	gint				n_columns;
	GType				column_types[XML_TREE_MODEL_N_COLUMNS];
 
	gint				stamp;			 /* Random integer to check whether an iter belongs to our model */
};
 
 
 
/* xmlTreeModelClass: more boilerplate GObject stuff */
 
struct _xmlTreeModelClass
{
	GObjectClass parent_class;
	
	/* Signals */
	void (* xml_tree_model_changed)		(xmlTreeModel *model);
	void (* xml_tree_model_error)		(xmlTreeModel *model);
	void (* xml_tree_model_xsl_error)	(xmlTreeModel *model);

};
 
 
GType			xml_tree_model_get_type					(void);

/* Test cases */

xmlTreeModel	*xml_tree_model_new						(void);

xmlTreeModel	*xml_tree_model_new_from_file			(gchar	*filename);


GtkListStore	*xml_tree_model_get_xpath_results		(xmlTreeModel *model,
														 gchar		  *xpath);
GtkListStore	*xml_tree_model_get_stylesheet_params	(xmlTreeModel *model);

xmlTreeModel	*xml_tree_model_transform				(xmlTreeModel *xml,
														 xmlTreeModel *xslt,
														 GHashTable	  *params);
														 				 
/* No test cases */
void			xml_tree_model_add_file					(xmlTreeModel *model,
														 gchar		  *filename);

gboolean		xml_tree_model_validate					(xmlTreeModel *model);


gint			xml_tree_model_write_to_file			(xmlTreeModel *model,
														 gint 		  fd,
														 gint		  format);

gint			xml_tree_model_save_xsl					(xmlTreeModel *model,
														 gchar			*filename);


gboolean		xml_tree_model_is_stylesheet			(xmlTreeModel *model); 

void			xml_tree_model_reload					(xmlTreeModel *model);

GtkTreePath		*xml_tree_model_get_path_from_xpath		(xmlTreeModel *model,
														 gchar		  *xpath);

GtkTreePath 	*xml_tree_model_get_path_from_position	(xmlTreeModel *model,
														 gint		  position);

gchar 			*xml_tree_model_get_xpath_from_position	(xmlTreeModel *model,
														 gint		  position);
														 
#endif /* _xml_tree_model_h_included_ */
