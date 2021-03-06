//#include <config.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "xmltools.h"
#include "xmltreemodel.h"
#include "xmlcellrenderer.h"

static void xml_cell_renderer_get_property  (GObject                    *object,
						    guint                       param_id,
						    GValue                     *value,
						    GParamSpec                 *pspec);
static void xml_cell_renderer_set_property  (GObject                    *object,
						    guint                       param_id,
						    const GValue               *value,
						    GParamSpec                 *pspec);
static void xml_cell_renderer_init       (xmlCellRenderer      *celltext);
static void xml_cell_renderer_class_init (xmlCellRendererClass *class);
static void xml_cell_renderer_finalize   (GObject                    *object);
static void xml_cell_renderer_create_stock_pixbuf (xmlCellRenderer *cellpixbuf,
							  GtkWidget             *widget);
static void xml_cell_renderer_create_named_icon_pixbuf (xmlCellRenderer *cellpixbuf,
							       GtkWidget             *widget);
static void xml_cell_renderer_get_size   (GtkCellRenderer            *cell,
						 GtkWidget                  *widget,
						 GdkRectangle               *rectangle,
						 gint                       *x_offset,
						 gint                       *y_offset,
						 gint                       *width,
						 gint                       *height);
static void xml_cell_renderer_render     (GtkCellRenderer            *cell,
						 GdkDrawable                *window,
						 GtkWidget                  *widget,
						 GdkRectangle               *background_area,
						 GdkRectangle               *cell_area,
						 GdkRectangle               *expose_area,
						 GtkCellRendererState        flags);


enum {
	PROP_ZERO,
	PROP_PIXBUF,
	PROP_PIXBUF_EXPANDER_OPEN,
	PROP_PIXBUF_EXPANDER_CLOSED,
	PROP_STOCK_ID,
	PROP_STOCK_SIZE,
	PROP_STOCK_DETAIL,
	PROP_FOLLOW_STATE,
	PROP_ICON_NAME,
	PROP_NODE_ID
};

static gpointer parent_class;


#define XML_CELL_RENDERER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XML_TYPE_CELL_RENDERER, xmlCellRendererPrivate))

typedef struct _xmlCellRendererPrivate xmlCellRendererPrivate;
struct _xmlCellRendererPrivate
{
  gchar *stock_id;
  GtkIconSize stock_size;
  gchar *stock_detail;
  gboolean follow_state;

  gchar *icon_name;
gint node_id;
};


GType
xml_cell_renderer_get_type (void)
{
  static GType cell_pixbuf_type = 0;

  if (!cell_pixbuf_type)
    {
      static const GTypeInfo cell_pixbuf_info =
      {
	sizeof (xmlCellRendererClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) xml_cell_renderer_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (xmlCellRenderer),
	0,              /* n_preallocs */
	(GInstanceInitFunc) xml_cell_renderer_init,
      };

      cell_pixbuf_type =
	g_type_register_static (GTK_TYPE_CELL_RENDERER, "xmlCellRenderer",
			        &cell_pixbuf_info, 0);
    }

  return cell_pixbuf_type;
}

static void
xml_cell_renderer_init (xmlCellRenderer *cellpixbuf)
{
  xmlCellRendererPrivate *priv;

  priv = XML_CELL_RENDERER_GET_PRIVATE (cellpixbuf);
  priv->stock_size = GTK_ICON_SIZE_MENU;
}

static void
xml_cell_renderer_class_init (xmlCellRendererClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

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

  g_object_class_install_property (object_class,
				   PROP_NODE_ID,
				   g_param_spec_uint ("node-id",
						      "Node ID",
						      "The XML Node Type",
						      0,
						      G_MAXUINT,
						      GTK_ICON_SIZE_MENU,
						      G_PARAM_READWRITE));
	
  g_type_class_add_private (object_class, sizeof (xmlCellRendererPrivate));
}

static void
xml_cell_renderer_finalize (GObject *object)
{
  xmlCellRenderer *cellpixbuf = XML_CELL_RENDERER (object);
  xmlCellRendererPrivate *priv;

  priv = XML_CELL_RENDERER_GET_PRIVATE (object);
  
  if (cellpixbuf->pixbuf)
    g_object_unref (cellpixbuf->pixbuf);
  if (cellpixbuf->pixbuf_expander_open)
    g_object_unref (cellpixbuf->pixbuf_expander_open);
  if (cellpixbuf->pixbuf_expander_closed)
    g_object_unref (cellpixbuf->pixbuf_expander_closed);

  g_free (priv->stock_id);
  g_free (priv->stock_detail);
  g_free (priv->icon_name);

  (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
xml_cell_renderer_get_property (GObject        *object,
				       guint           param_id,
				       GValue         *value,
				       GParamSpec     *pspec)
{
  xmlCellRenderer *cellpixbuf = XML_CELL_RENDERER (object);
  xmlCellRendererPrivate *priv;

  priv = XML_CELL_RENDERER_GET_PRIVATE (object);
  
  switch (param_id)
    {
    case PROP_PIXBUF:
      g_value_set_object (value, G_OBJECT (cellpixbuf->pixbuf));
      break;
    case PROP_PIXBUF_EXPANDER_OPEN:
      g_value_set_object (value, G_OBJECT (cellpixbuf->pixbuf_expander_open));
      break;
    case PROP_PIXBUF_EXPANDER_CLOSED:
      g_value_set_object (value, G_OBJECT (cellpixbuf->pixbuf_expander_closed));
      break;
    case PROP_STOCK_ID:
      g_value_set_string (value, priv->stock_id);
      break;
    case PROP_STOCK_SIZE:
      g_value_set_uint (value, priv->stock_size);
      break;
    case PROP_STOCK_DETAIL:
      g_value_set_string (value, priv->stock_detail);
      break;
    case PROP_FOLLOW_STATE:
      g_value_set_boolean (value, priv->follow_state);
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, priv->icon_name);
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
xml_cell_renderer_set_property (GObject      *object,
				       guint         param_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
  xmlCellRenderer *cellpixbuf = XML_CELL_RENDERER (object);
  xmlCellRendererPrivate *priv;

  priv = XML_CELL_RENDERER_GET_PRIVATE (object);
  
  switch (param_id)
    {
    case PROP_PIXBUF:
      if (cellpixbuf->pixbuf)
	g_object_unref (cellpixbuf->pixbuf);
      cellpixbuf->pixbuf = (GdkPixbuf*) g_value_dup_object (value);
      if (cellpixbuf->pixbuf)
        {
          if (priv->stock_id)
            {
              g_free (priv->stock_id);
              priv->stock_id = NULL;
              g_object_notify (object, "stock-id");
            }
          if (priv->icon_name)
            {
              g_free (priv->icon_name);
              priv->icon_name = NULL;
              g_object_notify (object, "icon-name");
            }
        }
      break;
    case PROP_PIXBUF_EXPANDER_OPEN:
      if (cellpixbuf->pixbuf_expander_open)
	g_object_unref (cellpixbuf->pixbuf_expander_open);
      cellpixbuf->pixbuf_expander_open = (GdkPixbuf*) g_value_dup_object (value);
      break;
    case PROP_PIXBUF_EXPANDER_CLOSED:
      if (cellpixbuf->pixbuf_expander_closed)
	g_object_unref (cellpixbuf->pixbuf_expander_closed);
      cellpixbuf->pixbuf_expander_closed = (GdkPixbuf*) g_value_dup_object (value);
      break;
    case PROP_STOCK_ID:
      if (priv->stock_id)
        {
          if (cellpixbuf->pixbuf)
            {
              g_object_unref (cellpixbuf->pixbuf);
              cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
            }
          g_free (priv->stock_id);
        }
      priv->stock_id = g_value_dup_string (value);
      if (priv->stock_id)
        {
          if (cellpixbuf->pixbuf)
            {
              g_object_unref (cellpixbuf->pixbuf);
              cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
            }
          if (priv->icon_name)
            {
              g_free (priv->icon_name);
              priv->icon_name = NULL;
              g_object_notify (object, "icon-name");
            }
        }
      break;
    case PROP_STOCK_SIZE:
      priv->stock_size = g_value_get_uint (value);
      break;
    case PROP_STOCK_DETAIL:
      if (priv->stock_detail)
        g_free (priv->stock_detail);
      priv->stock_detail = g_value_dup_string (value);
      break;
    case PROP_ICON_NAME:
      if (priv->icon_name)
	{
	  if (cellpixbuf->pixbuf)
	    {
	      g_object_unref (cellpixbuf->pixbuf);
	      cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
	    }
	  g_free (priv->icon_name);
	}
      priv->icon_name = g_value_dup_string (value);
      if (priv->icon_name)
        {
	  if (cellpixbuf->pixbuf)
	    {
              g_object_unref (cellpixbuf->pixbuf);
              cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
	    }
          if (priv->stock_id)
            {
              g_free (priv->stock_id);
              priv->stock_id = NULL;
              g_object_notify (object, "stock-id");
            }
        }
      break;
    case PROP_FOLLOW_STATE:
      priv->follow_state = g_value_get_boolean (value);
      break;
	case PROP_NODE_ID:
      if (priv->node_id > 0 && priv->node_id < XML_N_NODE_TYPES)
        {
          if (cellpixbuf->pixbuf)
            {
              g_object_unref (cellpixbuf->pixbuf);
              cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
            }
          priv->node_id = 0;
        }
      priv->node_id = g_value_get_uint(value);
      if (priv->node_id > 0 && priv->node_id < XML_N_NODE_TYPES)
        {
          if (cellpixbuf->pixbuf)
            {
              g_object_unref (cellpixbuf->pixbuf);
              cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
            }
          if (priv->icon_name)
            {
              g_free (priv->icon_name);
              priv->icon_name = NULL;
              g_object_notify (object, "icon-name");
            }
			if (priv->stock_id)
			{
              	g_free (priv->stock_id);
			}
			priv->stock_id = g_strdup(XmlNodes[priv->node_id].stock_id);
			g_object_notify (object, "stock-id");
        }
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
	xml_icon_factory_new();
	return g_object_new (XML_TYPE_CELL_RENDERER, NULL);
}

static void
xml_cell_renderer_create_stock_pixbuf (xmlCellRenderer *cellpixbuf,
                                              GtkWidget             *widget)
{
  xmlCellRendererPrivate *priv;

  priv = XML_CELL_RENDERER_GET_PRIVATE (cellpixbuf);

  if (cellpixbuf->pixbuf)
    g_object_unref (cellpixbuf->pixbuf);

  cellpixbuf->pixbuf = gtk_widget_render_icon (widget,
                                               priv->stock_id,
                                               priv->stock_size,
                                               priv->stock_detail);

  g_object_notify (G_OBJECT (cellpixbuf), "pixbuf");
}

static void 
xml_cell_renderer_create_named_icon_pixbuf (xmlCellRenderer *cellpixbuf,
						   GtkWidget             *widget)
{
  xmlCellRendererPrivate *priv;
  GdkScreen *screen;
  GtkIconTheme *icon_theme;
  GtkSettings *settings;
  gint width, height;
  GError *error = NULL;

  priv = XML_CELL_RENDERER_GET_PRIVATE (cellpixbuf);

  if (cellpixbuf->pixbuf)
    g_object_unref (cellpixbuf->pixbuf);

  screen = gtk_widget_get_screen (GTK_WIDGET (widget));
  icon_theme = gtk_icon_theme_get_for_screen (screen);
  settings = gtk_settings_get_for_screen (screen);

  if (!gtk_icon_size_lookup_for_settings (settings,
					  priv->stock_size,
					  &width, &height))
    {
      g_warning ("Invalid icon size %d\n", priv->stock_size);
      width = height = 24;
    }

  cellpixbuf->pixbuf =
    gtk_icon_theme_load_icon (icon_theme,
			      priv->icon_name,
			      MIN (width, height), 0, &error);
  if (!cellpixbuf->pixbuf) 
    {
      g_warning ("could not load image: %s\n", error->message);
      g_error_free (error);
    }

  g_object_notify (G_OBJECT (cellpixbuf), "pixbuf");
}

static GdkPixbuf *
create_colorized_pixbuf (GdkPixbuf *src, 
			 GdkColor  *new_color)
{
  gint i, j;
  gint width, height, has_alpha, src_row_stride, dst_row_stride;
  gint red_value, green_value, blue_value;
  guchar *target_pixels;
  guchar *original_pixels;
  guchar *pixsrc;
  guchar *pixdest;
  GdkPixbuf *dest;
  
  red_value = new_color->red / 255.0;
  green_value = new_color->green / 255.0;
  blue_value = new_color->blue / 255.0;
  
  dest = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
			 gdk_pixbuf_get_has_alpha (src),
			 gdk_pixbuf_get_bits_per_sample (src),
			 gdk_pixbuf_get_width (src),
			 gdk_pixbuf_get_height (src));
  
  has_alpha = gdk_pixbuf_get_has_alpha (src);
  width = gdk_pixbuf_get_width (src);
  height = gdk_pixbuf_get_height (src);
  src_row_stride = gdk_pixbuf_get_rowstride (src);
  dst_row_stride = gdk_pixbuf_get_rowstride (dest);
  target_pixels = gdk_pixbuf_get_pixels (dest);
  original_pixels = gdk_pixbuf_get_pixels (src);
  
  for (i = 0; i < height; i++) {
    pixdest = target_pixels + i*dst_row_stride;
    pixsrc = original_pixels + i*src_row_stride;
    for (j = 0; j < width; j++) {		
      *pixdest++ = (*pixsrc++ * red_value) >> 8;
      *pixdest++ = (*pixsrc++ * green_value) >> 8;
      *pixdest++ = (*pixsrc++ * blue_value) >> 8;
      if (has_alpha) {
	*pixdest++ = *pixsrc++;
      }
    }
  }
  return dest;
}


static void
xml_cell_renderer_get_size (GtkCellRenderer *cell,
				   GtkWidget       *widget,
				   GdkRectangle    *cell_area,
				   gint            *x_offset,
				   gint            *y_offset,
				   gint            *width,
				   gint            *height)
{
  xmlCellRenderer *cellpixbuf = (xmlCellRenderer *) cell;
  xmlCellRendererPrivate *priv;
  gint pixbuf_width  = 0;
  gint pixbuf_height = 0;
  gint calc_width;
  gint calc_height;

  priv = XML_CELL_RENDERER_GET_PRIVATE (cell);

  if (!cellpixbuf->pixbuf)
    {
      if (priv->stock_id)
	xml_cell_renderer_create_stock_pixbuf (cellpixbuf, widget);
      else if (priv->icon_name)
	xml_cell_renderer_create_named_icon_pixbuf (cellpixbuf, widget);
    }
  
  if (cellpixbuf->pixbuf)
    {
      pixbuf_width  = gdk_pixbuf_get_width (cellpixbuf->pixbuf);
      pixbuf_height = gdk_pixbuf_get_height (cellpixbuf->pixbuf);
    }
  if (cellpixbuf->pixbuf_expander_open)
    {
      pixbuf_width  = MAX (pixbuf_width, gdk_pixbuf_get_width (cellpixbuf->pixbuf_expander_open));
      pixbuf_height = MAX (pixbuf_height, gdk_pixbuf_get_height (cellpixbuf->pixbuf_expander_open));
    }
  if (cellpixbuf->pixbuf_expander_closed)
    {
      pixbuf_width  = MAX (pixbuf_width, gdk_pixbuf_get_width (cellpixbuf->pixbuf_expander_closed));
      pixbuf_height = MAX (pixbuf_height, gdk_pixbuf_get_height (cellpixbuf->pixbuf_expander_closed));
    }
  
  calc_width  = (gint) cell->xpad * 2 + pixbuf_width;
  calc_height = (gint) cell->ypad * 2 + pixbuf_height;
  
  if (x_offset) *x_offset = 0;
  if (y_offset) *y_offset = 0;

  if (cell_area && pixbuf_width > 0 && pixbuf_height > 0)
    {
      if (x_offset)
	{
	  *x_offset = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                        1.0 - cell->xalign : cell->xalign) * 
                       (cell_area->width - calc_width - 2 * cell->xpad));
	  *x_offset = MAX (*x_offset, 0) + cell->xpad;
	}
      if (y_offset)
	{
	  *y_offset = (cell->yalign *
                       (cell_area->height - calc_height - 2 * cell->ypad));
          *y_offset = MAX (*y_offset, 0) + cell->ypad;
	}
    }

  if (width)
    *width = calc_width;
  
  if (height)
    *height = calc_height;
}

static void
xml_cell_renderer_render (GtkCellRenderer      *cell,
				 GdkWindow            *window,
				 GtkWidget            *widget,
				 GdkRectangle         *background_area,
				 GdkRectangle         *cell_area,
				 GdkRectangle         *expose_area,
				 GtkCellRendererState  flags)

{
  xmlCellRenderer *cellpixbuf = (xmlCellRenderer *) cell;
  xmlCellRendererPrivate *priv;
  GdkPixbuf *pixbuf;
  GdkPixbuf *invisible = NULL;
  GdkPixbuf *colorized = NULL;
  GdkRectangle pix_rect;
  GdkRectangle draw_rect;
  cairo_t *cr;

  priv = XML_CELL_RENDERER_GET_PRIVATE (cell);

  xml_cell_renderer_get_size (cell, widget, cell_area,
				     &pix_rect.x,
				     &pix_rect.y,
				     &pix_rect.width,
				     &pix_rect.height);

  pix_rect.x += cell_area->x;
  pix_rect.y += cell_area->y;
  pix_rect.width  -= cell->xpad * 2;
  pix_rect.height -= cell->ypad * 2;

  if (!gdk_rectangle_intersect (cell_area, &pix_rect, &draw_rect) ||
      !gdk_rectangle_intersect (expose_area, &draw_rect, &draw_rect))
    return;

  pixbuf = cellpixbuf->pixbuf;

  if (cell->is_expander)
    {
      if (cell->is_expanded &&
	  cellpixbuf->pixbuf_expander_open != NULL)
	pixbuf = cellpixbuf->pixbuf_expander_open;
      else if (!cell->is_expanded &&
	       cellpixbuf->pixbuf_expander_closed != NULL)
	pixbuf = cellpixbuf->pixbuf_expander_closed;
    }

  if (!pixbuf)
    return;

  if (GTK_WIDGET_STATE (widget) == GTK_STATE_INSENSITIVE || !cell->sensitive)
    {
      GtkIconSource *source;
      
      source = gtk_icon_source_new ();
      gtk_icon_source_set_pixbuf (source, pixbuf);
      /* The size here is arbitrary; since size isn't
       * wildcarded in the source, it isn't supposed to be
       * scaled by the engine function
       */
      gtk_icon_source_set_size (source, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_icon_source_set_size_wildcarded (source, FALSE);
      
     invisible = gtk_style_render_icon (widget->style,
					source,
					gtk_widget_get_direction (widget),
					GTK_STATE_INSENSITIVE,
					/* arbitrary */
					(GtkIconSize)-1,
					widget,
					"gtkcellrendererpixbuf");
     
     gtk_icon_source_free (source);
     
     pixbuf = invisible;
    }
  else if (priv->follow_state && 
	   (flags & (GTK_CELL_RENDERER_SELECTED|GTK_CELL_RENDERER_PRELIT)) != 0)
    {
      GtkStateType state;

      if ((flags & GTK_CELL_RENDERER_SELECTED) != 0)
	{
	  if (GTK_WIDGET_HAS_FOCUS (widget))
	    state = GTK_STATE_SELECTED;
	  else
	    state = GTK_STATE_ACTIVE;
	}
      else
	state = GTK_STATE_PRELIGHT;

      colorized = create_colorized_pixbuf (pixbuf,
					   &widget->style->base[state]);

      pixbuf = colorized;
    }

  cr = gdk_cairo_create (window);
  
  gdk_cairo_set_source_pixbuf (cr, pixbuf, pix_rect.x, pix_rect.y);
  gdk_cairo_rectangle (cr, &draw_rect);
  cairo_fill (cr);

  cairo_destroy (cr);
  
  if (invisible)
    g_object_unref (invisible);

  if (colorized)
    g_object_unref (colorized);
}

#define __XML_CELL_RENDERER_C__

