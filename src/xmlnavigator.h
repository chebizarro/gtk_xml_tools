/*
 *	XML XmlNavigator Template
 *
 */

#ifndef __XML_NAVIGATOR_H__
#define __XML_NAVIGATOR_H__

#include <gtk/gtk.h>
#include "xmltreemodel.h"
#include "xmlcellrenderer.h"

G_BEGIN_DECLS

#define XML_NAVIGATOR_TYPE				(xml_navigator_get_type ())
#define XML_NAVIGATOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_NAVIGATOR_TYPE, XmlNavigator))
#define XML_NAVIGATOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XML_NAVIGATOR_TYPE, XmlNavigatorClass))
#define IS_XML_NAVIGATOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_NAVIGATOR_TYPE))
#define IS_XML_NAVIGATOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XML_NAVIGATOR_TYPE))

typedef struct _XmlNavigator				XmlNavigator;
typedef struct _XmlNavigatorClass		XmlNavigatorClass;

typedef struct _xmlTreeModelFilter		xmlTreeModelFilter;
typedef struct _xmlToolBarButton		xmlToolBarButton;
typedef struct _xmlToolBar				xmlToolBar;
typedef struct _xmlXpathExplorer		xmlXpathExplorer;
typedef struct _xmlValidator				xmlValidator;
typedef struct _xsltTransformer			xsltTransformer;

struct _xmlTreeModelFilter
{
	GtkTreeModelFilter	* filter;
	gboolean			row_visible[XML_N_NODE_TYPES];
	gboolean			show_ns;
};

struct _xmlToolBarButton
{
	GtkToggleToolButton	* button;
	xmlElementType	type;
	xmlTreeModel		* model;
};

struct _xmlToolBar
{
	GtkToolbar			* toolbar;
	xmlToolBarButton	toolbar_buttons[6];
};

struct _xmlXpathExplorer
{
	GtkVBox				* widget;
	
	GtkExpander			* expander;
	GtkEntry			* entry;
	GtkTreeView			* results;
};

struct _xmlValidator
{
	GtkVBox				* widget;
	
	GtkExpander			* expander;
	GtkEntry			* entry;
};

struct _xsltTransformer
{
	GtkVBox		*widget;
	
	GtkExpander	*expander;
	GtkEntry	*entry;
	GtkTreeView	*parameters;
};


struct _XmlNavigator
{
	GtkVBox					widget;

	xmlTreeModel			* model;
	xmlTreeModel			* stylesheet;

	xmlTreeModelFilter	filter;

	xmlToolBar				toolbar;
	xmlXpathExplorer		xpathExplorer;
	xmlValidator			validator;
	xsltTransformer		transformer;
	
	GtkTreeView				* navigator;
	
};

struct _XmlNavigatorClass
{
	GtkVBoxClass parent_class;

	/* signals */
	void (* xml_row_activated)	(XmlNavigator *ttt);
	void (* xml_row_expanded)	(XmlNavigator *ttt);
	void (* xml_row_collapsed)	(XmlNavigator *ttt);
	void (* xml_model_changed)	(XmlNavigator *ttt);
	void (* xpath_model_changed)	(XmlNavigator *ttt);
	void (* xsl_menu_activated)	(XmlNavigator *ttt);
};

GType		xml_navigator_get_type	(void);
GtkWidget*	xml_navigator_new		(void);

void		xml_navigator_set_model	(XmlNavigator *ttt, xmlTreeModel *xmltreemodel);

G_END_DECLS

#endif /* __XML_NAVIGATOR_H__ */
