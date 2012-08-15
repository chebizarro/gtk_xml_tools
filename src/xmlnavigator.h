/*
 *	XML XmlNavigator Template
 *
 */

#ifndef __XML_NAVIGATOR_H__
#define __XML_NAVIGATOR_H__

#include <gtk/gtk.h>
#include "xmltreemodel.h"

G_BEGIN_DECLS

#define XML_NAVIGATOR_TYPE				(xml_navigator_get_type ())
#define XML_NAVIGATOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_NAVIGATOR_TYPE, XmlNavigator))
#define XML_NAVIGATOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XML_NAVIGATOR_TYPE, XmlNavigatorClass))
#define IS_XML_NAVIGATOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_NAVIGATOR_TYPE))
#define IS_XML_NAVIGATOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XML_NAVIGATOR_TYPE))

typedef struct _XmlNavigator			XmlNavigator;
typedef struct _XmlNavigatorClass	XmlNavigatorClass;

struct _XmlNavigator
{
	GtkVBox		widget;
	
	GtkWidget	*toolbar;
	GtkWidget	*scrolled_window;
	GtkWidget	*navigator_view_vbox;
	GtkWidget	*navigator_view;
	GtkWidget	*xpath_entry;
	
	XmlList	*model;
	
};

struct _XmlNavigatorClass
{
	GtkVBoxClass parent_class;

	void (* navigator_selected) (XmlNavigator *ttt);
};

GType		xml_navigator_get_type	(void);
GtkWidget*	xml_navigator_new		(void);

void		xml_navigator_set_model	(XmlNavigator *ttt, XmlList *xmllist);

G_END_DECLS

#endif /* __XML_NAVIGATOR_H__ */