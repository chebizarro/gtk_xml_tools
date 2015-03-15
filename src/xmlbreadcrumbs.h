/* nautilus-pathbar.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * 
 */

#ifndef XML_BREADCRUMBS_H
#define XML_BREADCRUMBS_H

#include <gtk/gtk.h>
#include <gio/gio.h>
#include "xmltreemodel.h"

typedef struct _XmlBreadcrumbs      XmlBreadcrumbs;
typedef struct _XmlBreadcrumbsClass XmlBreadcrumbsClass;
typedef struct _XmlBreadcrumbsDetails XmlBreadcrumbsDetails;

#define XML_TYPE_BREADCRUMBS                 (xml_breadcrumbs_get_type ())
#define XML_BREADCRUMBS(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_TYPE_BREADCRUMBS, XmlBreadcrumbs))
#define XML_BREADCRUMBS_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), XML_TYPE_BREADCRUMBS, XmlBreadcrumbsClass))
#define XML_IS_BREADCRUMBS(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_TYPE_BREADCRUMBS))
#define XML_IS_BREADCRUMBS_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), XML_TYPE_BREADCRUMBS))
#define XML_BREADCRUMBS_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), XML_TYPE_BREADCRUMBS, XmlBreadcrumbsClass))

struct _XmlBreadcrumbs
{
	GtkContainer parent;
	
	XmlBreadcrumbsDetails *priv;
};

struct _XmlBreadcrumbsClass
{
	GtkContainerClass parent_class;

  	void (* path_clicked)   (XmlBreadcrumbs  *path_bar,
							GtkTreePath		 *path);
    void (* path_event)     (XmlBreadcrumbs  *path_bar,
                                 GdkEventButton   *event,
                                 GFile            *location);
};

GType    xml_breadcrumbs_get_type (void) G_GNUC_CONST;

void     xml_breadcrumbs_set_path_from_xpath (XmlBreadcrumbs *path_bar, 	gchar * xpath);

void     xml_breadcrumbs_set_path_from_position (XmlBreadcrumbs *path_bar, 	gint position);

void	xml_breadcrumbs_set_model(XmlBreadcrumbs *breadcrumbs, xmlTreeModel *model);


XmlBreadcrumbs *xml_breadcrumbs_new ( void );

#endif /* XML_BREADCRUMBS_H */
