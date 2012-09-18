/* gtkcellrendererpixbuf.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __XML_CELL_RENDERER_H__
#define __XML_CELL_RENDERER_H__

#include <gtk/gtk.h>
#include <gtk/gtkcellrenderer.h>


G_BEGIN_DECLS


#define XML_TYPE_CELL_RENDERER			(xml_cell_renderer_get_type ())
#define XML_CELL_RENDERER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_TYPE_CELL_RENDERER, xmlCellRenderer))
#define XML_CELL_RENDERER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XML_TYPE_CELL_RENDERER, xmlCellRendererClass))
#define XML_IS_CELL_RENDERER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_TYPE_CELL_RENDERER))
#define XML_IS_CELL_RENDERER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XML_TYPE_CELL_RENDERER))
#define XML_CELL_RENDERER_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), XML_TYPE_CELL_RENDERER, xmlCellRendererClass))

typedef struct _xmlCellRenderer              xmlCellRenderer;
typedef struct _xmlCellRendererPrivate       xmlCellRendererPrivate;
typedef struct _xmlCellRendererClass         xmlCellRendererClass;

struct _xmlCellRenderer
{
  GtkCellRenderer parent;

  /*< private >*/
  xmlCellRendererPrivate *priv;
};

struct _xmlCellRendererClass
{
  GtkCellRendererClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GType            xml_cell_renderer_get_type (void) G_GNUC_CONST;
GtkCellRenderer *xml_cell_renderer_new      (void);


G_END_DECLS


#endif /* __XML_CELL_RENDERER_H__ */
