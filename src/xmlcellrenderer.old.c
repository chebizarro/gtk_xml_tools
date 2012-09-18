/* gtkcellrendererpixbuf.c
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

#include <stdlib.h>
#include <gtk/gtk.h>
#include "xmlcellrenderer.h"

//#include "gtkiconhelperprivate.h"


static void xml_cell_renderer_get_property  (GObject                    *object,
						    guint                       param_id,
						    GValue                     *value,
						    GParamSpec                 *pspec);
static void xml_cell_renderer_set_property  (GObject                    *object,
						    guint                       param_id,
						    const GValue               *value,
						    GParamSpec                 *pspec);
static void xml_cell_renderer_get_size   (GtkCellRenderer            *cell,
						 GtkWidget                  *widget,
						 const GdkRectangle         *rectangle,
						 gint                       *x_offset,
						 gint                       *y_offset,
						 gint                       *width,
						 gint                       *height);
static void xml_cell_renderer_render     (GtkCellRenderer            *cell,
						 cairo_t                    *cr,
						 GtkWidget                  *widget,
						 const GdkRectangle         *background_area,
						 const GdkRectangle         *cell_area,
						 GtkCellRendererState        flags);

enum {
  PROP_0,
  PROP_PIXBUF,
  PROP_PIXBUF_EXPANDER_OPEN,
  PROP_PIXBUF_EXPANDER_CLOSED,
  PROP_STOCK_ID,
  PROP_STOCK_SIZE,
  PROP_STOCK_DETAIL,
  PROP_FOLLOW_STATE,
  PROP_ICON_NAME,
  PROP_GICON,
  PROP_NODE_ID
};


struct _xmlCellRendererPrivate
{
  //GtkIconHelper *icon_helper;
  GtkIconSize    icon_size;

  GdkPixbuf *pixbuf_expander_open;
  GdkPixbuf *pixbuf_expander_closed;

  gboolean follow_state;

  gchar *stock_detail;
  gint node_id;
};


G_DEFINE_TYPE (xmlCellRenderer, xml_cell_renderer, GTK_TYPE_CELL_RENDERER)


static void
xml_cell_renderer_init (xmlCellRenderer *cellpixbuf)
{
  xmlCellRendererPrivate *priv;

  cellpixbuf->priv = G_TYPE_INSTANCE_GET_PRIVATE (cellpixbuf,
                                                  XML_TYPE_CELL_RENDERER,
                                                  xmlCellRendererPrivate);
  priv = cellpixbuf->priv;
  //priv->icon_helper = _gtk_icon_helper_new ();
  priv->icon_size = GTK_ICON_SIZE_MENU;
}

static void
xml_cell_renderer_finalize (GObject *object)
{
  xmlCellRenderer *cellpixbuf = XML_CELL_RENDERER (object);
  xmlCellRendererPrivate *priv = cellpixbuf->priv;

  //g_clear_object (&priv->icon_helper);

  if (priv->pixbuf_expander_open)
    g_object_unref (priv->pixbuf_expander_open);
  if (priv->pixbuf_expander_closed)
    g_object_unref (priv->pixbuf_expander_closed);

  g_free (priv->stock_detail);

  G_OBJECT_CLASS (xml_cell_renderer_parent_class)->finalize (object);
}

static void
xml_cell_renderer_class_init (xmlCellRendererClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

  object_class->finalize = xml_cell_renderer_finalize;

  object_class->get_property = xml_cell_renderer_get_property;
  object_class->set_property = xml_cell_renderer_set_property;

  cell_class->get_size = xml_cell_renderer_get_size;
  cell_class->render = xml_cell_renderer_render;

  g_object_class_install_property (object_class,
				   PROP_PIXBUF,
				   g_param_spec_object ("pixbuf",
							"Pixbuf Object",
							"The pixbuf to render",
							GDK_TYPE_PIXBUF,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_PIXBUF_EXPANDER_OPEN,
				   g_param_spec_object ("pixbuf-expander-open",
							"Pixbuf Expander Open",
							"Pixbuf for open expander",
							GDK_TYPE_PIXBUF,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_PIXBUF_EXPANDER_CLOSED,
				   g_param_spec_object ("pixbuf-expander-closed",
							"Pixbuf Expander Closed",
							"Pixbuf for closed expander",
							GDK_TYPE_PIXBUF,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_STOCK_ID,
				   g_param_spec_string ("stock-id",
							"Stock ID",
							"The stock ID of the stock icon to render",
							NULL,
							G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_STOCK_SIZE,
				   g_param_spec_uint ("stock-size",
						      "Size",
						      "The GtkIconSize value that specifies the size of the rendered icon",
						      0,
						      G_MAXUINT,
						      GTK_ICON_SIZE_MENU,
						      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_STOCK_DETAIL,
				   g_param_spec_string ("stock-detail",
							"Detail",
							"Render detail to pass to the theme engine",
							NULL,
							G_PARAM_READWRITE));

  
  /**
   * xmlCellRenderer:icon-name:
   *
   * The name of the themed icon to display.
   * This property only has an effect if not overridden by "stock_id" 
   * or "pixbuf" properties.
   *
   * Since: 2.8 
   */
  g_object_class_install_property (object_class,
				   PROP_ICON_NAME,
				   g_param_spec_string ("icon-name",
							"Icon Name",
							"The name of the icon from the icon theme",
							NULL,
							G_PARAM_READWRITE));

  /**
   * xmlCellRenderer:follow-state:
   *
   * Specifies whether the rendered pixbuf should be colorized
   * according to the #GtkCellRendererState.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
				   PROP_FOLLOW_STATE,
				   g_param_spec_boolean ("follow-state",
 							 "Follow State",
 							 "Whether the rendered pixbuf should be "
							 "colorized according to the state",
 							 FALSE,
 							 G_PARAM_READWRITE));

  /**
   * xmlCellRenderer:gicon:
   *
   * The GIcon representing the icon to display.
   * If the icon theme is changed, the image will be updated
   * automatically.
   *
   * Since: 2.14
   */
  g_object_class_install_property (object_class,
                                   PROP_GICON,
                                   g_param_spec_object ("gicon",
                                                        "Icon",
                                                        "The GIcon being displayed",
                                                        G_TYPE_ICON,
                                                        G_PARAM_READWRITE));




  g_object_class_install_property (object_class,
								   PROP_NODE_ID,
								   g_param_spec_uint (	"node-id",
														"Node ID",
														"The Node ID of the xml icon to render",
														0,
														G_MAXUINT,
														GTK_ICON_SIZE_MENU,
														G_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (xmlCellRendererPrivate));

  //_gtk_cell_renderer_class_set_accessible_type (cell_class, G_TYPE_IMAGE_CELL_ACCESSIBLE);
}

static void
xml_cell_renderer_get_property (GObject        *object,
				       guint           param_id,
				       GValue         *value,
				       GParamSpec     *pspec)
{
  xmlCellRenderer *cellpixbuf = XML_CELL_RENDERER (object);
  xmlCellRendererPrivate *priv = cellpixbuf->priv;

  switch (param_id)
    {
    case PROP_PIXBUF:
      //g_value_set_object (value, _gtk_icon_helper_peek_pixbuf (priv->icon_helper));
      break;
    case PROP_PIXBUF_EXPANDER_OPEN:
      g_value_set_object (value, priv->pixbuf_expander_open);
      break;
    case PROP_PIXBUF_EXPANDER_CLOSED:
      g_value_set_object (value, priv->pixbuf_expander_closed);
      break;
    case PROP_STOCK_ID:
      //g_value_set_string (value, _gtk_icon_helper_get_stock_id (priv->icon_helper));
      break;
    case PROP_STOCK_SIZE:
      g_value_set_uint (value, priv->icon_size);
      break;
    case PROP_STOCK_DETAIL:
      g_value_set_string (value, priv->stock_detail);
      break;
    case PROP_FOLLOW_STATE:
      g_value_set_boolean (value, priv->follow_state);
      break;
    case PROP_ICON_NAME:
      //g_value_set_string (value, _gtk_icon_helper_get_icon_name (priv->icon_helper));
      break;
    case PROP_GICON:
      //g_value_set_object (value, _gtk_icon_helper_peek_gicon (priv->icon_helper));
      break;
    case PROP_NODE_ID:
      g_value_set_uint (value, priv->node_id);
 
		break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
xml_cell_renderer_reset (xmlCellRenderer *cellpixbuf)
{
  xmlCellRendererPrivate *priv = cellpixbuf->priv;
  //GtkImageType storage_type = _gtk_icon_helper_get_storage_type (priv->icon_helper);
  GtkImageType storage_type = GTK_IMAGE_STOCK;
	
  switch (storage_type)
    {
    case GTK_IMAGE_PIXBUF:
      g_object_notify (G_OBJECT (cellpixbuf), "pixbuf");
      break;
    case GTK_IMAGE_STOCK:
      g_object_notify (G_OBJECT (cellpixbuf), "stock-id");      
      break;
    case GTK_IMAGE_ICON_NAME:
      g_object_notify (G_OBJECT (cellpixbuf), "icon-name");
      break;
    case GTK_IMAGE_GICON:
      g_object_notify (G_OBJECT (cellpixbuf), "gicon");
      break;
    case GTK_IMAGE_EMPTY:
    default:
      break;
    }

//  _gtk_icon_helper_clear (priv->icon_helper);
}

static void
xml_cell_renderer_set_property (GObject      *object,
				       guint         param_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
  xmlCellRenderer *cellpixbuf = XML_CELL_RENDERER (object);
  xmlCellRendererPrivate *priv = cellpixbuf->priv;

  switch (param_id)
    {
    case PROP_PIXBUF:
      xml_cell_renderer_reset (cellpixbuf);
//      _gtk_icon_helper_set_pixbuf (priv->icon_helper, g_value_get_object (value));
      break;
    case PROP_PIXBUF_EXPANDER_OPEN:
      if (priv->pixbuf_expander_open)
        g_object_unref (priv->pixbuf_expander_open);
      priv->pixbuf_expander_open = (GdkPixbuf*) g_value_dup_object (value);
      break;
    case PROP_PIXBUF_EXPANDER_CLOSED:
      if (priv->pixbuf_expander_closed)
        g_object_unref (priv->pixbuf_expander_closed);
      priv->pixbuf_expander_closed = (GdkPixbuf*) g_value_dup_object (value);
      break;
    case PROP_STOCK_ID:
      xml_cell_renderer_reset (cellpixbuf);
//      _gtk_icon_helper_set_stock_id (priv->icon_helper, g_value_get_string (value), priv->icon_size);
      break;
    case PROP_STOCK_SIZE:
      priv->icon_size = g_value_get_uint (value);
 //     _gtk_icon_helper_set_icon_size (priv->icon_helper, priv->icon_size);
      break;
    case PROP_STOCK_DETAIL:
      g_free (priv->stock_detail);
      priv->stock_detail = g_value_dup_string (value);
      break;
    case PROP_ICON_NAME:
      xml_cell_renderer_reset (cellpixbuf);
//      _gtk_icon_helper_set_icon_name (priv->icon_helper, g_value_get_string (value), priv->icon_size);
      break;
    case PROP_FOLLOW_STATE:
      priv->follow_state = g_value_get_boolean (value);
      break;
    case PROP_GICON:
      xml_cell_renderer_reset (cellpixbuf);
//      _gtk_icon_helper_set_gicon (priv->icon_helper, g_value_get_object (value), priv->icon_size);
      break;
	case PROP_NODE_ID:
      priv->node_id = g_value_get_uint (value);
//      _gtk_icon_helper_set_gicon (priv->icon_helper, g_value_get_object (value), priv->icon_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

/**
 * xml_cell_renderer_new:
 * 
 * Creates a new #xmlCellRenderer. Adjust rendering
 * parameters using object properties. Object properties can be set
 * globally (with g_object_set()). Also, with #GtkTreeViewColumn, you
 * can bind a property to a value in a #GtkTreeModel. For example, you
 * can bind the "pixbuf" property on the cell renderer to a pixbuf value
 * in the model, thus rendering a different image in each row of the
 * #GtkTreeView.
 * 
 * Return value: the new cell renderer
 **/
GtkCellRenderer *
xml_cell_renderer_new (void)
{
  return g_object_new (XML_TYPE_CELL_RENDERER, NULL);
}

static void
xml_cell_renderer_get_size (GtkCellRenderer    *cell,
				   GtkWidget          *widget,
				   const GdkRectangle *cell_area,
				   gint               *x_offset,
				   gint               *y_offset,
				   gint               *width,
				   gint               *height)
{
  xmlCellRenderer *cellpixbuf = (xmlCellRenderer *) cell;
  xmlCellRendererPrivate *priv = cellpixbuf->priv;
  gint pixbuf_width  = 0;
  gint pixbuf_height = 0;
  gint calc_width;
  gint calc_height;
  gint xpad, ypad;
  //GtkStyleContext *context;

  //context = gtk_widget_get_style_context (widget);
  //gtk_style_context_save (context);
  //gtk_style_context_add_class (context, GTK_STYLE_CLASS_IMAGE);
/*
  if (!_gtk_icon_helper_get_is_empty (priv->icon_helper))
    _gtk_icon_helper_get_size (priv->icon_helper, 
                               gtk_widget_get_style_context (widget),
                               &pixbuf_width, &pixbuf_height);

  gtk_style_context_restore (context);
*/
  if (priv->pixbuf_expander_open)
    {
      pixbuf_width  = MAX (pixbuf_width, gdk_pixbuf_get_width (priv->pixbuf_expander_open));
      pixbuf_height = MAX (pixbuf_height, gdk_pixbuf_get_height (priv->pixbuf_expander_open));
    }
  if (priv->pixbuf_expander_closed)
    {
      pixbuf_width  = MAX (pixbuf_width, gdk_pixbuf_get_width (priv->pixbuf_expander_closed));
      pixbuf_height = MAX (pixbuf_height, gdk_pixbuf_get_height (priv->pixbuf_expander_closed));
    }

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
  calc_width  = (gint) xpad * 2 + pixbuf_width;
  calc_height = (gint) ypad * 2 + pixbuf_height;
  
  if (cell_area && pixbuf_width > 0 && pixbuf_height > 0)
    {
      gfloat xalign, yalign;

      gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);
      if (x_offset)
	{
	  *x_offset = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                        (1.0 - xalign) : xalign) *
                       (cell_area->width - calc_width));
	  *x_offset = MAX (*x_offset, 0);
	}
      if (y_offset)
	{
	  *y_offset = (yalign *
                       (cell_area->height - calc_height));
          *y_offset = MAX (*y_offset, 0);
	}
    }
  else
    {
      if (x_offset) *x_offset = 0;
      if (y_offset) *y_offset = 0;
    }

  if (width)
    *width = calc_width;
  
  if (height)
    *height = calc_height;
}

static void
_gtk_icon_helper_draw (GtkIconHelper *self,
                       GtkStyleContext *context,
                       cairo_t *cr,
                       gdouble x,
                       gdouble y)
{
  GdkPixbuf *pixbuf;

  pixbuf = _gtk_icon_helper_ensure_pixbuf (self, context);

  if (pixbuf != NULL)
    {
      gtk_render_icon (context, cr, pixbuf, x, y);
      g_object_unref (pixbuf);
    }
}

static void
xml_cell_renderer_render (GtkCellRenderer      *cell,
                                 cairo_t              *cr,
				 GtkWidget            *widget,
				 const GdkRectangle   *background_area,
				 const GdkRectangle   *cell_area,
				 GtkCellRendererState  flags)

{
  xmlCellRenderer *cellpixbuf = (xmlCellRenderer *) cell;
  xmlCellRendererPrivate *priv = cellpixbuf->priv;
  //GtkStyleContext *context;
  GdkRectangle pix_rect;
  GdkRectangle draw_rect;
  gboolean is_expander;
  gint xpad, ypad;
  //GtkStateFlags state;
  //GtkIconHelper *icon_helper = NULL;

  xml_cell_renderer_get_size (cell, widget, (GdkRectangle *) cell_area,
				     &pix_rect.x, 
                                     &pix_rect.y,
                                     &pix_rect.width,
                                     &pix_rect.height);

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
  pix_rect.x += cell_area->x + xpad;
  pix_rect.y += cell_area->y + ypad;
  pix_rect.width -= xpad * 2;
  pix_rect.height -= ypad * 2;

  if (!gdk_rectangle_intersect (cell_area, &pix_rect, &draw_rect))
    return;

  //context = gtk_widget_get_style_context (widget);
  //gtk_style_context_save (context);

  //state = GTK_STATE_FLAG_NORMAL;
/*
  if (!gtk_widget_get_sensitive (widget) ||
      !gtk_cell_renderer_get_sensitive (cell))
    state |= GTK_STATE_FLAG_INSENSITIVE;
  else if (priv->follow_state && 
	   (flags & (GTK_CELL_RENDERER_SELECTED |
		     GTK_CELL_RENDERER_PRELIT)) != 0)
    state = gtk_cell_renderer_get_state (cell, widget, flags);
*/
  //gtk_style_context_set_state (context, state);
  //gtk_style_context_add_class (context, GTK_STYLE_CLASS_IMAGE);

  g_object_get (cell, "is-expander", &is_expander, NULL);
  if (is_expander)
    {
      gboolean is_expanded;

      g_object_get (cell, "is-expanded", &is_expanded, NULL);

      if (is_expanded && priv->pixbuf_expander_open != NULL)
        {
         // icon_helper = _gtk_icon_helper_new ();
         // _gtk_icon_helper_set_pixbuf (icon_helper, priv->pixbuf_expander_open);
        }
      else if (!is_expanded && priv->pixbuf_expander_closed != NULL)
        {
        //  icon_helper = _gtk_icon_helper_new ();
        //  _gtk_icon_helper_set_pixbuf (icon_helper, priv->pixbuf_expander_closed);
        }
    }

 // if (icon_helper == NULL)
    //icon_helper = g_object_ref (priv->icon_helper);
/*
  _gtk_icon_helper_draw (icon_helper,
                         context, cr,
                         pix_rect.x, pix_rect.y);
  g_object_unref (icon_helper);

  gtk_style_context_restore (context);
 */
}
