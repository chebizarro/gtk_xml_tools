/*
 *	Custom Widget Template
 *
 */

#ifndef __XPATH_EXPLORER_H__
#define __XPATH_EXPLORER_H__

#include <gtk/gtk.h>
#include "xmltreemodel.h"
#include "xmlcellrenderer.h"


G_BEGIN_DECLS

#define XPATH_EXPLORER_TYPE				(xpath_explorer_get_type ())
#define XPATH_EXPLORER(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XPATH_EXPLORER_TYPE, XpathExplorer))
#define XPATH_EXPLORER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XPATH_EXPLORER_TYPE, XpathExplorerClass))
#define IS_XPATH_EXPLORER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XPATH_EXPLORER_TYPE))
#define IS_XPATH_EXPLORER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XPATH_EXPLORER_TYPE))

typedef struct _XpathExplorer			 XpathExplorer;
typedef struct _XpathExplorerClass	XpathExplorerClass;

struct _XpathExplorer
{
	GtkVBox		 widget;

	GtkEntry	 *entry;
	GtkTreeView	 *results;

	xmlTreeModel *model;

};

struct _XpathExplorerClass
{
	GtkVBoxClass parent_class;

	void (* xpath_row_activated)	(XpathExplorer *ttt);
	void (* xpath_model_changed) 	(XpathExplorer *ttt);
	void (* xpath_show_in_nav)	 	(XpathExplorer *ttt);
	void (* xpath_button_press)	 	(XpathExplorer *ttt);
};

GType		xpath_explorer_get_type	(void);
GtkWidget*	xpath_explorer_new		(void);

void		xpath_explorer_set_model(XpathExplorer *ttt, xmlTreeModel * xmltreemodel);

G_END_DECLS

#endif /* __XPATH_EXPLORER_H__ */
