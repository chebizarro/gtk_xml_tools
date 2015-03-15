#ifndef __XML_CELL_RENDERER_H__
#define __XML_CELL_RENDERER_H__

#include <gtk/gtkcellrenderer.h>


G_BEGIN_DECLS


#define XML_TYPE_CELL_RENDERER			(xml_cell_renderer_get_type ())
#define XML_CELL_RENDERER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), XML_TYPE_CELL_RENDERER, xmlCellRenderer))
#define XML_CELL_RENDERER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XML_TYPE_CELL_RENDERER, xmlCellRendererClass))
#define XML_IS_CELL_RENDERER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XML_TYPE_CELL_RENDERER))
#define XML_IS_CELL_RENDERER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XML_TYPE_CELL_RENDERER))
#define XML_CELL_RENDERER_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), XML_TYPE_CELL_RENDERER, xmlCellRendererClass))

typedef struct _xmlCellRenderer xmlCellRenderer;
typedef struct _xmlCellRendererClass xmlCellRendererClass;

struct _xmlCellRenderer
{
  GtkCellRenderer parent;

  /*< private >*/
  GdkPixbuf *pixbuf;
  GdkPixbuf *pixbuf_expander_open;
  GdkPixbuf *pixbuf_expander_closed;
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
