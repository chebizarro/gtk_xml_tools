#ifndef _xml_list_h_included_
#define _xml_list_h_included_
 
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
 
#define XML_TYPE_LIST				(xml_list_get_type ())
#define XML_LIST(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_TYPE_LIST, XmlList))
#define XML_LIST_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass),	XML_TYPE_LIST, XmlListClass))
#define XML_IS_LIST(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_TYPE_LIST))
#define XML_IS_LIST_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass),	XML_TYPE_LIST))
#define XML_LIST_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj),	XML_TYPE_LIST, XmlListClass))
 
/* The data columns that we export via the tree model interface */
 
enum
{
	XML_LIST_COL_TYPE = 0,
	XML_LIST_COL_NAME,
	XML_LIST_COL_CONTENT,
	XML_LIST_COL_LINE,
	XML_LIST_COL_VISIBLE,
	XML_LIST_COL_XPATH,
	XML_LIST_N_COLUMNS,
};

#define XML_N_NODE_TYPES 22

typedef struct _XmlList			 XmlList;
typedef struct _XmlListClass	XmlListClass;

 
/* XmlList: this structure contains everything we need for our
 *						 model implementation. It is
 *						 crucial that 'parent' is the first member of the
 *						 structure.																					*/
 
struct _XmlList
{
	GObject		parent;			/* this MUST be the first member */
 
	xmlDocPtr	xmldoc;

	xmlDocPtr	xpath_results;
	
	gboolean	row_visible[XML_N_NODE_TYPES];

	gchar		*xpath;
	
	
	/* These two fields are not absolutely necessary, but they		*/
	/*	 speed things up a bit in our get_value implementation		*/
	gint		n_columns;
	GType		column_types[XML_LIST_N_COLUMNS];
 
	gint		stamp;			 /* Random integer to check whether an iter belongs to our model */
};
 
 
 
/* XmlListClass: more boilerplate GObject stuff */
 
struct _XmlListClass
{
	GObjectClass parent_class;
};
 
 
GType	xml_list_get_type (void);

//void	xml_type_register_type(GTypeModule *type_module);
 
XmlList	*xml_list_new (void);

void	xml_list_add_file (XmlList   *xml_list, const gchar  *filename);

void	xml_list_set_visible (XmlList *xml_list, gint nodetype, gboolean visible);

void	xml_list_set_xpath(XmlList *xml_list, gchar * xpath);

GtkListStore * xml_get_xpath_results(XmlList *xmllist, gchar *xpath);

                      
#endif /* _xml_list_h_included_ */
