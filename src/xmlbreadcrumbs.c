//#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "xmltools.h"
#include "xmlbreadcrumbs.h"


enum {
	BREADCRUMB_PATH_CLICKED,
	BREADCRUMB_PATH_EVENT,
	BREADCRUMB_LAST_SIGNAL
};

#define BUTTON_DATA(x) ((ButtonData *)(x))

#define SCROLL_TIMEOUT           150
#define INITIAL_SCROLL_TIMEOUT   300

static guint xml_breadcrumb_signals [BREADCRUMB_LAST_SIGNAL] = { 0 };

#define XML_BREADCRUMBS_ICON_SIZE 16
#define XML_BREADCRUMBS_BUTTON_MAX_WIDTH 250

typedef struct {
	GtkWidget		*button;
	xmlElementType	type;
	gchar			*node_name;
	gchar			*ns;
	gchar			*xpath;
	GtkTreePath		*path;

	/* custom icon */ 
	GdkPixbuf *custom_icon;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *bold_label;

    guint ignore_changes : 1;
    guint is_root : 1;
} ButtonData;

struct _XmlBreadcrumbsDetails {
	GdkWindow		*event_window;
	xmlTreeModel	*model;

	GtkTreePath		*current_path;
	gpointer		current_button_data;

	GList			*button_list;
	GList			*first_scrolled_button;
	GtkWidget		*up_slider_button;
	GtkWidget		*down_slider_button;
	guint			settings_signal_id;
	gint16			slider_width;
	guint			timer;
	guint			slider_visible : 1;
	guint			need_timer : 1;
	guint			ignore_click : 1;

};


G_DEFINE_TYPE (XmlBreadcrumbs, xml_breadcrumbs, GTK_TYPE_CONTAINER);

static void     xml_breadcrumbs_scroll_up                (XmlBreadcrumbs *path_bar);
static void     xml_breadcrumbs_scroll_down              (XmlBreadcrumbs *path_bar);
static void     xml_breadcrumbs_stop_scrolling           (XmlBreadcrumbs *path_bar);
static gboolean xml_breadcrumbs_slider_button_press      (GtkWidget       *widget,
														  GdkEventButton  *event,
														  XmlBreadcrumbs *path_bar);
static gboolean xml_breadcrumbs_slider_button_release    (GtkWidget       *widget,
														  GdkEventButton  *event,
														  XmlBreadcrumbs *path_bar);
static void     xml_breadcrumbs_check_icon_theme         (XmlBreadcrumbs *path_bar);
static void     xml_breadcrumbs_update_button_appearance (ButtonData      *button_data);
static void     xml_breadcrumbs_update_button_state      (ButtonData      *button_data,
														  gboolean         current_dir);
static void     xml_breadcrumbs_update_path              (XmlBreadcrumbs *path_bar,
														  GtkTreePath 	*treepath);

static void		xml_breadcrumbs_size_request			(GtkWidget *widget,
														GtkRequisition *requisition);
static gboolean	xml_breadcrumbs_check_parent_path		( XmlBreadcrumbs *path_bar,
														GtkTreePath * treepath,
														ButtonData **current_button_data);


static GtkWidget *
get_slider_button ( XmlBreadcrumbs  *path_bar,
					GtkArrowType	arrow_type)
{
	GtkWidget *button;
	
	GValue value = G_VALUE_INIT;
	
	g_value_init (&value, G_TYPE_INT);
	g_value_set_int(&value,GTK_PACK_END);
	
	
	gtk_widget_push_composite_child ();
	button = gtk_button_new ();
	gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
	gtk_widget_add_events (button, GDK_SCROLL_MASK);
    gtk_container_add (GTK_CONTAINER (button), gtk_arrow_new (arrow_type, GTK_SHADOW_OUT));
	gtk_container_add (GTK_CONTAINER (path_bar), button);
	
	if(arrow_type == GTK_ARROW_RIGHT)
		gtk_container_child_set_property(GTK_CONTAINER (path_bar), button, "pack-type", &value);
	
	gtk_widget_show_all (button);
    gtk_widget_pop_composite_child ();
    return button;
}

static void
xml_breadcrumbs_init (XmlBreadcrumbs *path_bar)
{
	path_bar->priv = G_TYPE_INSTANCE_GET_PRIVATE (path_bar, XML_TYPE_BREADCRUMBS, XmlBreadcrumbsDetails);

	gtk_widget_set_has_window (GTK_WIDGET (path_bar), FALSE);
    gtk_widget_set_redraw_on_allocate (GTK_WIDGET (path_bar), FALSE);

    path_bar->priv->up_slider_button = get_slider_button (path_bar, GTK_ARROW_LEFT);
    path_bar->priv->down_slider_button = get_slider_button (path_bar, GTK_ARROW_RIGHT);

    g_signal_connect_swapped (path_bar->priv->up_slider_button, "clicked", G_CALLBACK (xml_breadcrumbs_scroll_up), path_bar);
    g_signal_connect_swapped (path_bar->priv->down_slider_button, "clicked", G_CALLBACK (xml_breadcrumbs_scroll_down), path_bar);

    g_signal_connect (path_bar->priv->up_slider_button, "button_press_event", G_CALLBACK (xml_breadcrumbs_slider_button_press), path_bar);
    g_signal_connect (path_bar->priv->up_slider_button, "button_release_event", G_CALLBACK (xml_breadcrumbs_slider_button_release), path_bar);
    g_signal_connect (path_bar->priv->down_slider_button, "button_press_event", G_CALLBACK (xml_breadcrumbs_slider_button_press), path_bar);
    g_signal_connect (path_bar->priv->down_slider_button, "button_release_event", G_CALLBACK (xml_breadcrumbs_slider_button_release), path_bar);

	//gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (path_bar)), GTK_STYLE_CLASS_LINKED);
}

static void
xml_breadcrumbs_finalize (GObject *object)
{
    XmlBreadcrumbs *path_bar;

    path_bar = XML_BREADCRUMBS (object);

	xml_breadcrumbs_stop_scrolling (path_bar);

    g_list_free (path_bar->priv->button_list);

    G_OBJECT_CLASS (xml_breadcrumbs_parent_class)->finalize (object);
}

/* Removes the settings signal handler.  It's safe to call multiple times */
static void
remove_settings_signal (XmlBreadcrumbs *path_bar,
			GdkScreen  *screen)
{
	if (path_bar->priv->settings_signal_id) {
 	 	GtkSettings *settings;
		settings = gtk_settings_get_for_screen (screen);
		g_signal_handler_disconnect (settings,
									 path_bar->priv->settings_signal_id);
      	path_bar->priv->settings_signal_id = 0;
	}
}

static void
xml_breadcrumbs_dispose (GObject *object)
{
	remove_settings_signal (XML_BREADCRUMBS (object), gtk_widget_get_screen (GTK_WIDGET (object)));

	G_OBJECT_CLASS (xml_breadcrumbs_parent_class)->dispose (object);
}

static char *
get_node_name (ButtonData *button_data)
{
	gchar **str;
	gchar *name;
	
	str = g_strsplit(button_data->xpath, "/", 0);
	
	name = g_strdup(str[g_strv_length(str) - 1]);
	
	g_strfreev(str);

	/*	
	if(g_utf8_strlen(button_data->ns,-1) > 0) {
		str = g_strdup_printf("%s:%s", button_data->ns, button_data->node_name);
	} else {
		str = g_strdup(button_data->node_name);
	}
	*/
	
	return name;
}

/* We always want to request the same size for the label, whether
 * or not the contents are bold
 */
static void
set_label_size_request (ButtonData *button_data)
{
    gint width, height;
	GtkRequisition min_req, bold_req;

	if (button_data->label == NULL) {
		return;
	}

	gtk_label_set_ellipsize (GTK_LABEL (button_data->label), PANGO_ELLIPSIZE_NONE);

	gtk_widget_size_request(button_data->label, &min_req);
	gtk_label_set_ellipsize (GTK_LABEL (button_data->label), PANGO_ELLIPSIZE_MIDDLE);	

	gtk_widget_size_request(button_data->bold_label, &bold_req);

	width = MAX (min_req.width, bold_req.width);
	width = MIN (width, XML_BREADCRUMBS_BUTTON_MAX_WIDTH);
	height = MAX (min_req.height, bold_req.height);

	gtk_widget_set_size_request (button_data->label, width, height);
}

/* Size requisition:
 * 
 * Ideally, our size is determined by another widget, and we are just filling
 * available space.
 */

static void
xml_breadcrumbs_update_slider_buttons (XmlBreadcrumbs *path_bar)
{
	if (path_bar->priv->button_list) {
                	
		GtkWidget *button;

		button = BUTTON_DATA (path_bar->priv->button_list->data)->button;
   		if (gtk_widget_get_child_visible (button)) {
			gtk_widget_set_sensitive (path_bar->priv->down_slider_button, FALSE);
		} else {
			gtk_widget_set_sensitive (path_bar->priv->down_slider_button, TRUE);
		}
       	button = BUTTON_DATA (g_list_last (path_bar->priv->button_list)->data)->button;
        if (gtk_widget_get_child_visible (button)) {
			gtk_widget_set_sensitive (path_bar->priv->up_slider_button, FALSE);
        } else {
			gtk_widget_set_sensitive (path_bar->priv->up_slider_button, TRUE);
		}
	}
}

static void
xml_breadcrumbs_unmap (GtkWidget *widget)
{
	xml_breadcrumbs_stop_scrolling (XML_BREADCRUMBS (widget));
	gdk_window_hide (XML_BREADCRUMBS (widget)->priv->event_window);

	GTK_WIDGET_CLASS (xml_breadcrumbs_parent_class)->unmap (widget);
}

static void
xml_breadcrumbs_map (GtkWidget *widget)
{
	gdk_window_show (XML_BREADCRUMBS (widget)->priv->event_window);

	GTK_WIDGET_CLASS (xml_breadcrumbs_parent_class)->map (widget);
}


static void
child_ordering_changed (XmlBreadcrumbs *path_bar)
{
	GList *l;

	if (path_bar->priv->up_slider_button) {
		//gtk_style_context_invalidate (gtk_widget_get_style_context (path_bar->priv->up_slider_button));
	}
	if (path_bar->priv->down_slider_button) {
		//gtk_style_context_invalidate (gtk_widget_get_style_context (path_bar->priv->down_slider_button));
	}

	for (l = path_bar->priv->button_list; l; l = l->next) {
		ButtonData *data = l->data;
		//gtk_style_context_invalidate (gtk_widget_get_style_context (data->button));		
	}
}

/* This is a tad complicated */
static void
xml_breadcrumbs_size_allocate ( GtkWidget     *widget,
								GtkAllocation *allocation)
{
	GtkWidget *child;
	XmlBreadcrumbs *path_bar;
	GtkTextDirection direction;
	GtkAllocation child_allocation;
	GList *list, *first_button;
	gint width;
	gint largest_width;
	gboolean need_sliders;
	gint up_slider_offset;
	gint down_slider_offset;
	GtkRequisition child_requisition;
	gboolean needs_reorder = FALSE;

	need_sliders = FALSE;
	up_slider_offset = 0;
	down_slider_offset = 0;
	path_bar = XML_BREADCRUMBS (widget);

	gtk_widget_set_allocation (widget, allocation);

	if (gtk_widget_get_realized (widget)) {
		gdk_window_move_resize (path_bar->priv->event_window,
					allocation->x, allocation->y,
					allocation->width, allocation->height);
	}

    /* No path is set so we don't have to allocate anything. */
    if (path_bar->priv->button_list == NULL) {
		return;
	}
    
    direction = gtk_widget_get_direction (widget);

  	/* First, we check to see if we need the scrollbars. */
	width = 0;

	for (list = path_bar->priv->button_list; list; list = list->next) {
		child = BUTTON_DATA (list->data)->button;
		gtk_widget_size_request(child, &child_requisition);
		width += child_requisition.width;
	}

	if (width <= allocation->width) {
		first_button = g_list_last (path_bar->priv->button_list);
	} else {
		gboolean reached_end;
		gint slider_space;
		reached_end = FALSE;
		slider_space = 2 * (path_bar->priv->slider_width);
		if (path_bar->priv->first_scrolled_button) {
			first_button = path_bar->priv->first_scrolled_button;
		} else {
			first_button = path_bar->priv->button_list;
		}        
		need_sliders = TRUE;
		/* To see how much space we have, and how many buttons we can display.
		* We start at the first button, count forward until hit the new
		* button, then count backwards.
		*/
		/* Count down the path chain towards the end. */
		gtk_widget_size_request(BUTTON_DATA (first_button->data)->button, &child_requisition);

		width = child_requisition.width;
		list = first_button->prev;
		while (list && !reached_end) {
	  		child = BUTTON_DATA (list->data)->button;
			gtk_widget_size_request(child, &child_requisition);
	  		if (width + child_requisition.width + slider_space > allocation->width) {
	    		reached_end = TRUE;
	  		} else {
				width += child_requisition.width;
			}

	  		list = list->prev;
		}

		/* Finally, we walk up, seeing how many of the previous buttons we can add*/

		while (first_button->next && ! reached_end) {
			child = BUTTON_DATA (first_button->next->data)->button;
			gtk_widget_size_request(child, &child_requisition);
	  		if (width + child_requisition.width + slider_space > allocation->width) {
      			reached_end = TRUE;
    		} else {
      			width += child_requisition.width;
      			first_button = first_button->next;
    		}
		}
	}
	/* Now, we allocate space to the buttons */
	child_allocation.y = allocation->y;
	child_allocation.height = allocation->height;

	if (direction == GTK_TEXT_DIR_RTL) {
		child_allocation.x = allocation->x + allocation->width;
		if (need_sliders) {
			child_allocation.x -= path_bar->priv->slider_width;
			up_slider_offset = allocation->width - path_bar->priv->slider_width;
		}
	} else {
		child_allocation.x = allocation->x;
		if (need_sliders) {
			up_slider_offset = 0;
			child_allocation.x += path_bar->priv->slider_width;
		}
     }

	/* Determine the largest possible allocation size */
	largest_width = allocation->width;
	if (need_sliders) {
		largest_width -= (path_bar->priv->slider_width) * 2;
	}

	for (list = first_button; list; list = list->prev) {
		child = BUTTON_DATA (list->data)->button;
		gtk_widget_size_request(child, &child_requisition);
		child_allocation.width = MIN (child_requisition.width, largest_width);
		if (direction == GTK_TEXT_DIR_RTL) {
			child_allocation.x -= child_allocation.width;
		}
		/* Check to see if we've don't have any more space to allocate buttons */
		if (need_sliders && direction == GTK_TEXT_DIR_RTL) {
			if (child_allocation.x - path_bar->priv->slider_width < allocation->x) {
				break;
			}
		} else {
			if (need_sliders && direction == GTK_TEXT_DIR_LTR) {
	  			if (child_allocation.x + child_allocation.width + path_bar->priv->slider_width > allocation->x + allocation->width) {
	    			break;	
				}	
			}
		}

		needs_reorder |= gtk_widget_get_child_visible (child) == FALSE;
		gtk_widget_set_child_visible (child, TRUE);
		gtk_widget_size_allocate (child, &child_allocation);

		if (direction == GTK_TEXT_DIR_RTL) {
	  		down_slider_offset = child_allocation.x - allocation->x - path_bar->priv->slider_width;
		} else {
	  		down_slider_offset += child_allocation.width;
	  		child_allocation.x += child_allocation.width;
		}
	}
	/* Now we go hide all the widgets that don't fit */
	while (list) {
		child = BUTTON_DATA (list->data)->button;
		needs_reorder |= gtk_widget_get_child_visible (child) == TRUE;
		gtk_widget_set_child_visible (child, FALSE);
		list = list->prev;
	}
	for (list = first_button->next; list; list = list->next) {
		child = BUTTON_DATA (list->data)->button;
		needs_reorder |= gtk_widget_get_child_visible (child) == TRUE;
		gtk_widget_set_child_visible (child, FALSE);
	}
	if (need_sliders) {
		child_allocation.width = path_bar->priv->slider_width;
		child_allocation.x = up_slider_offset + allocation->x;
		gtk_widget_size_allocate (path_bar->priv->up_slider_button, &child_allocation);

		needs_reorder |= gtk_widget_get_child_visible (path_bar->priv->up_slider_button) == FALSE;
		gtk_widget_set_child_visible (path_bar->priv->up_slider_button, TRUE);
		gtk_widget_show_all (path_bar->priv->up_slider_button);

		if (direction == GTK_TEXT_DIR_LTR) {
			down_slider_offset += path_bar->priv->slider_width;
		}
	} else {
		needs_reorder |= gtk_widget_get_child_visible (path_bar->priv->up_slider_button) == TRUE;
		gtk_widget_set_child_visible (path_bar->priv->up_slider_button, FALSE);
	}
	
	if (need_sliders) {
		child_allocation.width = path_bar->priv->slider_width;
		child_allocation.x = down_slider_offset + allocation->x;
		gtk_widget_size_allocate (path_bar->priv->down_slider_button, &child_allocation);

		needs_reorder |= gtk_widget_get_child_visible (path_bar->priv->down_slider_button) == FALSE;
      	gtk_widget_set_child_visible (path_bar->priv->down_slider_button, TRUE);
      	gtk_widget_show_all (path_bar->priv->down_slider_button);
      	xml_breadcrumbs_update_slider_buttons (path_bar);
    } else {
		needs_reorder |= gtk_widget_get_child_visible (path_bar->priv->down_slider_button) == TRUE;
    	gtk_widget_set_child_visible (path_bar->priv->down_slider_button, FALSE);
	}

	if (needs_reorder) {
		child_ordering_changed (path_bar);
	}
}

static void
xml_breadcrumbs_style_updated (GtkWidget *widget)
{
	//GTK_WIDGET_CLASS (xml_breadcrumbs_parent_class)->style_updated (widget);

    xml_breadcrumbs_check_icon_theme (XML_BREADCRUMBS (widget));
}

static void
xml_breadcrumbs_screen_changed (GtkWidget *widget,
								GdkScreen *previous_screen)
{
    if (GTK_WIDGET_CLASS (xml_breadcrumbs_parent_class)->screen_changed)
    {
       GTK_WIDGET_CLASS (xml_breadcrumbs_parent_class)->screen_changed (widget, previous_screen);
	}
    /* We might nave a new settings, so we remove the old one */
    if (previous_screen) {
       remove_settings_signal (XML_BREADCRUMBS (widget), previous_screen);
	}
    xml_breadcrumbs_check_icon_theme (XML_BREADCRUMBS (widget));
}

static gboolean
xml_breadcrumbs_scroll (GtkWidget      *widget,
						GdkEventScroll *event)
{
	XmlBreadcrumbs *path_bar;

	path_bar = XML_BREADCRUMBS (widget);

	switch (event->direction) {
		case GDK_SCROLL_RIGHT:
		case GDK_SCROLL_DOWN:
			xml_breadcrumbs_scroll_down (path_bar);
			return TRUE;

		case GDK_SCROLL_LEFT:
		case GDK_SCROLL_UP:
			xml_breadcrumbs_scroll_up (path_bar);
			return TRUE;
	}

	return FALSE;
}

static void
xml_breadcrumbs_realize (GtkWidget *widget)
{
	XmlBreadcrumbs *path_bar;
	GtkAllocation allocation;
	GdkWindow *window;
	GdkWindowAttr attributes;
	gint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(XML_IS_BREADCRUMBS(widget));

	gtk_widget_set_realized (widget, TRUE);

	path_bar = XML_BREADCRUMBS (widget);
	window = gtk_widget_get_parent_window (widget);
	gtk_widget_set_window (widget, window);
	g_object_ref (window);

	gtk_widget_get_allocation (widget, &allocation);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.wclass = GDK_INPUT_ONLY;
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= GDK_SCROLL_MASK |
							 GDK_BUTTON_PRESS_MASK |
							 GDK_BUTTON_RELEASE_MASK;
	attributes_mask = GDK_WA_X | GDK_WA_Y;

	widget->style = gtk_style_attach(widget->style, widget->window);
	gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);

	path_bar->priv->event_window = gdk_window_new ( gtk_widget_get_parent_window (widget),
													&attributes,
													attributes_mask);
													
	gdk_window_set_user_data (path_bar->priv->event_window, widget);
}

static void
xml_breadcrumbs_unrealize (GtkWidget *widget)
{
	XmlBreadcrumbs *path_bar;

	path_bar = XML_BREADCRUMBS (widget);

	gdk_window_set_user_data (path_bar->priv->event_window, NULL);
	gdk_window_destroy (path_bar->priv->event_window);
	path_bar->priv->event_window = NULL;

	GTK_WIDGET_CLASS (xml_breadcrumbs_parent_class)->unrealize (widget);
}

static void
xml_breadcrumbs_add (GtkContainer *container,
					 GtkWidget    *widget)
{
    gtk_widget_set_parent (widget, GTK_WIDGET (container));
}

static void
xml_breadcrumbs_remove_1 (GtkContainer *container,
		       	    GtkWidget    *widget)
{
	gboolean was_visible = gtk_widget_get_visible (widget);
	gtk_widget_unparent (widget);
	if (was_visible) {
		gtk_widget_queue_resize (GTK_WIDGET (container));
	}
}

static void
xml_breadcrumbs_remove (GtkContainer *container,
		          GtkWidget    *widget)
{
	XmlBreadcrumbs *path_bar;
	GList *children;

	path_bar = XML_BREADCRUMBS (container);

    if (widget == path_bar->priv->up_slider_button)
    {
		xml_breadcrumbs_remove_1 (container, widget);
		path_bar->priv->up_slider_button = NULL;
		return;
	}

    if (widget == path_bar->priv->down_slider_button)
    {
		xml_breadcrumbs_remove_1 (container, widget);
		path_bar->priv->down_slider_button = NULL;
		return;
    }

    children = path_bar->priv->button_list;
    while (children) {              
		if (widget == BUTTON_DATA (children->data)->button) {
			xml_breadcrumbs_remove_1 (container, widget);
	  		path_bar->priv->button_list = g_list_remove_link (path_bar->priv->button_list, children);
	  		g_list_free_1 (children);
	  		return;
		}
	children = children->next;
	}
}

static void
xml_breadcrumbs_forall (GtkContainer *container,
						gboolean      include_internals,
						GtkCallback   callback,
						gpointer      callback_data)
{
	XmlBreadcrumbs *path_bar;
	GList *children;

	g_return_if_fail (callback != NULL);
	path_bar = XML_BREADCRUMBS (container);

	children = path_bar->priv->button_list;
    while (children) {
		GtkWidget *child;
		child = BUTTON_DATA (children->data)->button;
		children = children->next;
		(* callback) (child, callback_data);
    }

    if (path_bar->priv->up_slider_button) {
       (* callback) (path_bar->priv->up_slider_button, callback_data);
	}

    if (path_bar->priv->down_slider_button) {
       (* callback) (path_bar->priv->down_slider_button, callback_data);
	}
}

static void
xml_breadcrumbs_grab_notify (GtkWidget *widget,
							 gboolean   was_grabbed)
{
	if (!was_grabbed) {
		xml_breadcrumbs_stop_scrolling (XML_BREADCRUMBS (widget));
	}
}

static void
xml_breadcrumbs_state_changed (GtkWidget    *widget,
			         GtkStateType  previous_state)
{
	if (!gtk_widget_get_sensitive (widget)) {
		xml_breadcrumbs_stop_scrolling (XML_BREADCRUMBS (widget));
	}
}
/*
static GtkWidgetPath *
xml_breadcrumbs_get_path_for_child ( GtkContainer *container,
									 GtkWidget    *child)
{
	XmlBreadcrumbs *path_bar = XML_BREADCRUMBS (container);
	GtkWidgetPath *path;

	path = gtk_widget_path_copy (gtk_widget_get_path (GTK_WIDGET (path_bar)));

	if (gtk_widget_get_visible (child) &&
	    gtk_widget_get_child_visible (child)) {
		GtkWidgetPath *sibling_path;
		GList *visible_children;
		GList *l;
		int pos;
*/
		/* 1. Build the list of visible children, in visually left-to-right order
		 * (i.e. independently of the widget's direction).  Note that our
		 * button_list is stored in innermost-to-outermost path order!
		 */
/*
		visible_children = NULL;

		if (gtk_widget_get_visible (path_bar->priv->down_slider_button) &&
		    gtk_widget_get_child_visible (path_bar->priv->down_slider_button)) {
			visible_children = g_list_prepend (visible_children, path_bar->priv->down_slider_button);
		}

		for (l = path_bar->priv->button_list; l; l = l->next) {
			ButtonData *data = l->data;
				
			if (gtk_widget_get_visible (data->button) &&
			    gtk_widget_get_child_visible (data->button))
				visible_children = g_list_prepend (visible_children, data->button);
		}

		if (gtk_widget_get_visible (path_bar->priv->up_slider_button) &&
		    gtk_widget_get_child_visible (path_bar->priv->up_slider_button)) {
			visible_children = g_list_prepend (visible_children, path_bar->priv->up_slider_button);
		}

		if (gtk_widget_get_direction (GTK_WIDGET (path_bar)) == GTK_TEXT_DIR_RTL) {
			visible_children = g_list_reverse (visible_children);
		}
*/
		/* 2. Find the index of the child within that list */
/*
		pos = 0;

		for (l = visible_children; l; l = l->next) {
			GtkWidget *button = l->data;

			if (button == child) {
				break;
			}

			pos++;
		}
*/
		/* 3. Build the path */
/*
		sibling_path = gtk_widget_path_new ();

		for (l = visible_children; l; l = l->next) {
			gtk_widget_path_append_for_widget (sibling_path, l->data);
		}

		gtk_widget_path_append_with_siblings (path, sibling_path, pos);

		g_list_free (visible_children);
		gtk_widget_path_unref (sibling_path);
	} else {
		gtk_widget_path_append_for_widget (path, child);
	}

	return path;
}
*/

static void
xml_breadcrumbs_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(XML_IS_BREADCRUMBS(widget));
	g_return_if_fail(requisition != NULL);

	GtkRequisition child_requisition;

	ButtonData *button_data;
	XmlBreadcrumbs *path_bar;
	GList *list;
	gint child_height;
	gint height;
	gint child_min, child_nat;

	path_bar = XML_BREADCRUMBS (widget);

	gint minimum = 0;
	gint natural = 0;
	height = 0;

	for (list = path_bar->priv->button_list; list; list = list->next) {
		button_data = BUTTON_DATA (list->data);
		//set_label_size_request (button_data);

		gtk_widget_size_request(button_data->button, &child_requisition);

		height = MAX (height, child_requisition.height);

		/* Use 2*Height as button width because of ellipsized label.  */
		child_min = MAX (child_requisition.width, child_requisition.height * 2);
		child_nat = MAX (child_min, child_requisition.height * 2);

		minimum = MAX (minimum, child_min);
		natural = MAX (natural, child_nat);
	}

	/* Add space for slider, if we have more than one path */
	/* Theoretically, the slider could be bigger than the other button.  But we're
	 * not going to worry about that now.
	 */
	path_bar->priv->slider_width = MIN (height * 2 / 3 + 5, height);

	if (path_bar->priv->button_list && path_bar->priv->button_list->next != NULL) {
		minimum += (path_bar->priv->slider_width) * 2;
		natural += (path_bar->priv->slider_width) * 2;
	}


	requisition->width = XML_BREADCRUMBS_BUTTON_MAX_WIDTH; //natural;
	requisition->height = 28;
}


static void
xml_breadcrumbs_class_init (XmlBreadcrumbsClass *path_bar_class)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	gobject_class = (GObjectClass *) path_bar_class;
	widget_class = (GtkWidgetClass *) path_bar_class;
	container_class = (GtkContainerClass *) path_bar_class;

	gobject_class->finalize = xml_breadcrumbs_finalize;
	gobject_class->dispose = xml_breadcrumbs_dispose;

	//widget_class->get_preferred_height = xml_breadcrumbs_get_preferred_height;
	//widget_class->get_preferred_width = xml_breadcrumbs_get_preferred_width;
	
	widget_class->size_request = xml_breadcrumbs_size_request;
	
	/*
	widget_class->expose_event = xml_breadcrumbs_expose;
	*/

	widget_class->realize = xml_breadcrumbs_realize;
	widget_class->unrealize = xml_breadcrumbs_unrealize;
	widget_class->unmap = xml_breadcrumbs_unmap;
	widget_class->map = xml_breadcrumbs_map;
    widget_class->size_allocate = xml_breadcrumbs_size_allocate;
    // widget_class->style_updated = xml_breadcrumbs_style_updated;
    widget_class->screen_changed = xml_breadcrumbs_screen_changed;
    widget_class->grab_notify = xml_breadcrumbs_grab_notify;
    widget_class->state_changed = xml_breadcrumbs_state_changed;
	widget_class->scroll_event = xml_breadcrumbs_scroll;

    container_class->add = xml_breadcrumbs_add;
    container_class->forall = xml_breadcrumbs_forall;
    container_class->remove = xml_breadcrumbs_remove;
	//container_class->get_path_for_child = xml_breadcrumbs_get_path_for_child;

	xml_breadcrumb_signals [BREADCRUMB_PATH_CLICKED] =	g_signal_new ("xml-breadcrumb-path-clicked",
													  G_OBJECT_CLASS_TYPE (path_bar_class),
													  G_SIGNAL_RUN_FIRST,
													  G_STRUCT_OFFSET (XmlBreadcrumbsClass, path_clicked),
													  NULL, NULL,
													  g_cclosure_marshal_VOID__OBJECT,
													  G_TYPE_NONE, 1, G_TYPE_POINTER);

	xml_breadcrumb_signals [BREADCRUMB_PATH_EVENT] = 	g_signal_new ("xml-breadcrumb-path-event",
													  G_OBJECT_CLASS_TYPE (path_bar_class),
													  G_SIGNAL_RUN_FIRST,
													  G_STRUCT_OFFSET (XmlBreadcrumbsClass, path_event),
													  NULL, NULL, NULL,
													  G_TYPE_NONE, 2,
													  G_TYPE_FILE,
													  GDK_TYPE_EVENT);

	 //gtk_container_class_handle_border_width (container_class);
	 g_type_class_add_private (path_bar_class, sizeof (XmlBreadcrumbsDetails));
}

static void
xml_breadcrumbs_scroll_down (XmlBreadcrumbs *path_bar)
{
        GList *list;
        GList *down_button;
        GList *up_button;
        gint space_available;
        gint space_needed;
        GtkTextDirection direction;
	GtkAllocation allocation, button_allocation, slider_allocation;

	down_button = NULL;
	up_button = NULL;

        if (path_bar->priv->ignore_click) {
                path_bar->priv->ignore_click = FALSE;
                return;   
        }

        gtk_widget_queue_resize (GTK_WIDGET (path_bar));

        direction = gtk_widget_get_direction (GTK_WIDGET (path_bar));
  
        /* We find the button at the 'down' end that we have to make */
        /* visible */
        for (list = path_bar->priv->button_list; list; list = list->next) {
        	if (list->next && gtk_widget_get_child_visible (BUTTON_DATA (list->next->data)->button)) {
			down_button = list;
	  		break;
			}
        }

	if (down_button == NULL) {
		return;
	}
  
        /* Find the last visible button on the 'up' end */
        for (list = g_list_last (path_bar->priv->button_list); list; list = list->prev) {
                if (gtk_widget_get_child_visible (BUTTON_DATA (list->data)->button)) {
	  		up_button = list;
	  		break;
		}
        }

	gtk_widget_get_allocation (BUTTON_DATA (down_button->data)->button, &button_allocation);
	gtk_widget_get_allocation (GTK_WIDGET (path_bar), &allocation);
	gtk_widget_get_allocation (path_bar->priv->down_slider_button, &slider_allocation);

        space_needed = button_allocation.width;
        if (direction == GTK_TEXT_DIR_RTL) {
                space_available = slider_allocation.x - allocation.x;
	} else {
                space_available = (allocation.x + allocation.width) -
                        (slider_allocation.x + slider_allocation.width);
	}

  	/* We have space_available extra space that's not being used.  We
   	* need space_needed space to make the button fit.  So we walk down
   	* from the end, removing buttons until we get all the space we
   	* need. */
	gtk_widget_get_allocation (BUTTON_DATA (up_button->data)->button, &button_allocation);
        while ((space_available < space_needed) &&
	       (up_button != NULL)) {
                space_available += button_allocation.width;
                up_button = up_button->prev;
                path_bar->priv->first_scrolled_button = up_button;
        }
}

static void
xml_breadcrumbs_scroll_up (XmlBreadcrumbs *path_bar)
{
        GList *list;

        if (path_bar->priv->ignore_click) {
                path_bar->priv->ignore_click = FALSE;
                return;   
        }

        gtk_widget_queue_resize (GTK_WIDGET (path_bar));

        for (list = g_list_last (path_bar->priv->button_list); list; list = list->prev) {
                if (list->prev && gtk_widget_get_child_visible (BUTTON_DATA (list->prev->data)->button)) {
			path_bar->priv->first_scrolled_button = list;
	  		return;
		}
        }
}

static gboolean
xml_breadcrumbs_scroll_timeout (XmlBreadcrumbs *path_bar)
{
        gboolean retval = FALSE;

        if (path_bar->priv->timer) {
                if (gtk_widget_has_focus (path_bar->priv->up_slider_button)) {
			xml_breadcrumbs_scroll_up (path_bar);
		} else {
			if (gtk_widget_has_focus (path_bar->priv->down_slider_button)) {
				xml_breadcrumbs_scroll_down (path_bar);
			}
         	}
         	if (path_bar->priv->need_timer) {
			path_bar->priv->need_timer = FALSE;

	  		path_bar->priv->timer = 
				g_timeout_add (SCROLL_TIMEOUT,
					       (GSourceFunc) xml_breadcrumbs_scroll_timeout,
					       path_bar);
	  
		} else {
			retval = TRUE;
		}
        }

        return retval;
}

static void 
xml_breadcrumbs_stop_scrolling (XmlBreadcrumbs *path_bar)
{
	if (path_bar->priv->timer)
	{
		g_source_remove (path_bar->priv->timer);
		path_bar->priv->timer = 0;
		path_bar->priv->need_timer = FALSE;
	}
}

static gboolean
xml_breadcrumbs_slider_button_press (GtkWidget		*widget, 
									 GdkEventButton	*event,
									 XmlBreadcrumbs	*path_bar)
{
	if (!gtk_widget_has_focus (widget))
		gtk_widget_grab_focus (widget);

	if (event->type != GDK_BUTTON_PRESS || event->button != 1)
		return FALSE;
	
	path_bar->priv->ignore_click = FALSE;

	if (widget == path_bar->priv->up_slider_button)
	{
		xml_breadcrumbs_scroll_up (path_bar);
	}
	else
	{
		if (widget == path_bar->priv->down_slider_button)
			xml_breadcrumbs_scroll_down (path_bar);
	}

	if (!path_bar->priv->timer)
	{
		path_bar->priv->need_timer = TRUE;
		path_bar->priv->timer = g_timeout_add (	INITIAL_SCROLL_TIMEOUT,
												(GSourceFunc) xml_breadcrumbs_scroll_timeout,
												path_bar);
	}
	return FALSE;
}

static gboolean
xml_breadcrumbs_slider_button_release (	GtkWidget		*widget, 
										GdkEventButton	*event,
										XmlBreadcrumbs	*path_bar)
{
	if (event->type != GDK_BUTTON_RELEASE)
		return FALSE;

	path_bar->priv->ignore_click = TRUE;
	xml_breadcrumbs_stop_scrolling (path_bar);

	return FALSE;
}

/* Changes the icons wherever it is needed */
static void
reload_icons (XmlBreadcrumbs *path_bar)
{
	GList *list;

	for (list = path_bar->priv->button_list; list; list = list->next)
	{
		ButtonData *button_data;
		button_data = BUTTON_DATA (list->data);
		xml_breadcrumbs_update_button_appearance (button_data);
	}
}

/* Callback used when a GtkSettings value changes */
static void
settings_notify_cb (GObject    *object,
					GParamSpec *pspec,
					XmlBreadcrumbs *path_bar)
{
	const char *name;
	name = g_param_spec_get_name (pspec);

	if (! strcmp (name, "gtk-icon-theme-name") || ! strcmp (name, "gtk-icon-sizes")) {
		reload_icons (path_bar);
	}
}

static void
xml_breadcrumbs_check_icon_theme (XmlBreadcrumbs *path_bar)
{
	GtkSettings *settings;

	if (path_bar->priv->settings_signal_id) {
		return;
	}
	settings = gtk_settings_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (path_bar)));
	path_bar->priv->settings_signal_id = g_signal_connect (settings, "notify", G_CALLBACK (settings_notify_cb), path_bar);

	reload_icons (path_bar);
}

/* Public functions and their helpers */
static void
xml_breadcrumbs_clear_buttons (XmlBreadcrumbs *path_bar)
{
	while (path_bar->priv->button_list != NULL) {
		gtk_container_remove (GTK_CONTAINER (path_bar), BUTTON_DATA (path_bar->priv->button_list->data)->button);
	}
    path_bar->priv->first_scrolled_button = NULL;
}

static void
button_clicked_cb (	GtkWidget *button,
					gpointer   data)
{
	ButtonData		*button_data;
	XmlBreadcrumbs	*path_bar;
	GList			*button_list;

    button_data = BUTTON_DATA (data);
    
    if (button_data->ignore_changes) {
		return;
	}

	path_bar = XML_BREADCRUMBS (gtk_widget_get_parent (button));

	button_list = g_list_find (path_bar->priv->button_list, button_data);
	g_assert (button_list != NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

	g_signal_emit (path_bar, xml_breadcrumb_signals [BREADCRUMB_PATH_CLICKED], 0, (gpointer)button_data->path);
	
	xml_breadcrumbs_check_parent_path(path_bar, button_data->path, &button_data); 
}

static gboolean
button_event_cb (GtkWidget *button,
				 GdkEventButton *event,
				 gpointer   data)
{
	ButtonData *button_data;
	XmlBreadcrumbs *path_bar;
	GList *button_list;

	button_data = BUTTON_DATA (data);
	path_bar = XML_BREADCRUMBS (gtk_widget_get_parent (button));

	if (event->type == GDK_BUTTON_PRESS) {
		g_object_set_data (	G_OBJECT (button), "handle-button-release",
							GINT_TO_POINTER (TRUE));
	}

	if (event->type == GDK_BUTTON_RELEASE &&
	    !GPOINTER_TO_UINT (g_object_get_data (	G_OBJECT (button),
												"handle-button-release")))
	{
		return FALSE;
	}

    button_list = g_list_find (path_bar->priv->button_list, button_data);
    g_assert (button_list != NULL);

	g_signal_emit (path_bar, xml_breadcrumb_signals [BREADCRUMB_PATH_EVENT], 0, button_data->path, event);

	return FALSE;
}

static void
button_data_free (ButtonData *button_data)
{
	gtk_tree_path_free (button_data->path);
    g_free (button_data->node_name);
    g_free (button_data->ns);

	if (button_data->custom_icon)
		g_object_unref (button_data->custom_icon);
	
	g_free (button_data);
}

static void
xml_breadcrumbs_update_button_appearance (ButtonData *button_data)
{
	gchar *node_name = get_node_name (button_data);

	if (button_data->label != NULL) {
		char *markup;
		markup = g_markup_printf_escaped ("<b>%s</b>", node_name);

		if (gtk_label_get_use_markup (GTK_LABEL (button_data->label)))
		{
	  		gtk_label_set_markup (GTK_LABEL (button_data->label), markup);
		} else {
			gtk_label_set_text (GTK_LABEL (button_data->label), node_name);
		}

		gtk_label_set_markup (GTK_LABEL (button_data->bold_label), markup);
		g_free (markup);
	}

	if (button_data->custom_icon) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (button_data->image), button_data->custom_icon);  
		gtk_widget_show (GTK_WIDGET (button_data->image));
	} else {
		if(button_data->type > 0 && button_data->type < XML_N_NODE_TYPES) {
			gtk_image_set_from_stock (GTK_IMAGE (button_data->image),
									  XmlNodes[button_data->type].stock_id,
									  GTK_ICON_SIZE_MENU);
			gtk_widget_show (GTK_WIDGET (button_data->image));
		} else {
			gtk_widget_hide (GTK_WIDGET (button_data->image));
		}
	}
}

static void
xml_breadcrumbs_update_button_state (ButtonData *button_data,
									 gboolean    current_dir)
{
	if (button_data->label != NULL) {
		gtk_label_set_label (GTK_LABEL (button_data->label), NULL);
		gtk_label_set_label (GTK_LABEL (button_data->bold_label), NULL);
		gtk_label_set_use_markup (GTK_LABEL (button_data->label), current_dir);
	}

	xml_breadcrumbs_update_button_appearance (button_data);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button_data->button)) != current_dir) {
		button_data->ignore_changes = TRUE;
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button_data->button), current_dir);
		button_data->ignore_changes = FALSE;
	}
	
	set_label_size_request(button_data);
}


static ButtonData *
make_button_data (XmlBreadcrumbs	*path_bar,
				  GtkTreePath		*path,
				  gboolean			current_dir)
{
    GtkWidget *child = NULL;
    ButtonData *button_data;
	GtkTreeIter iter;
	
    button_data = g_new0 (ButtonData, 1);

	if(gtk_tree_model_get_iter(path_bar->priv->model, &iter, path))
	{
		gtk_tree_model_get( path_bar->priv->model,
							&iter,
							XML_TREE_MODEL_COL_NAME, &button_data->node_name,
							XML_TREE_MODEL_COL_NS, &button_data->ns,
							XML_TREE_MODEL_COL_XPATH, &button_data->xpath,
							XML_TREE_MODEL_COL_TYPE, &button_data->type,
							-1 );
	}
	
	button_data->path = gtk_tree_path_copy(path);
 
    button_data->button = gtk_toggle_button_new ();
	gtk_button_set_focus_on_click (GTK_BUTTON (button_data->button), FALSE);
	gtk_widget_set_tooltip_text(button_data->button, button_data->xpath);
	gtk_widget_add_events (button_data->button, GDK_SCROLL_MASK);

	button_data->image = gtk_image_new ();
	button_data->label = gtk_label_new (NULL);
	child = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (child), button_data->image, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (child), button_data->label, FALSE, FALSE, 0);

	if (button_data->label != NULL) {
		gtk_label_set_ellipsize (GTK_LABEL (button_data->label), PANGO_ELLIPSIZE_MIDDLE);
		gtk_label_set_single_line_mode (GTK_LABEL (button_data->label), TRUE);

		button_data->bold_label = gtk_label_new (NULL);
		gtk_widget_set_no_show_all (button_data->bold_label, TRUE);
		gtk_label_set_single_line_mode (GTK_LABEL (button_data->bold_label), TRUE);
		gtk_box_pack_start (GTK_BOX (child), button_data->bold_label, FALSE, FALSE, 0);
	}

    gtk_container_add (GTK_CONTAINER (button_data->button), child);
    gtk_widget_show_all (button_data->button);

    xml_breadcrumbs_update_button_state (button_data, current_dir);

    g_signal_connect (button_data->button, "clicked", G_CALLBACK (button_clicked_cb), button_data);
	//g_signal_connect (button_data->button, "button-press-event", G_CALLBACK (button_event_cb), button_data);
	//g_signal_connect (button_data->button, "button-release-event", G_CALLBACK (button_event_cb), button_data);
    g_object_weak_ref (G_OBJECT (button_data->button), (GWeakNotify) button_data_free, button_data);

    return button_data;
}

static gboolean
xml_breadcrumbs_check_parent_path ( XmlBreadcrumbs *path_bar,
									GtkTreePath * treepath,
									ButtonData **current_button_data)
{
    GList *list;
	ButtonData *button_data, *current_data;
	gboolean is_active;
	gint	pathcompare;

	current_data = NULL;

    for (list = path_bar->priv->button_list; list; list = list->next)
    {
		button_data = list->data;
		pathcompare = gtk_tree_path_compare(treepath, button_data->path);
		if (pathcompare == 0)
		{
			current_data = button_data;
			is_active = TRUE;

			if (!gtk_widget_get_child_visible (current_data->button))
			{
				path_bar->priv->first_scrolled_button = list;
				gtk_widget_queue_resize (GTK_WIDGET (path_bar));
			}
		} else {
			is_active = FALSE;
		}

		xml_breadcrumbs_update_button_state (button_data, is_active);
	}

	if (current_button_data != NULL) {
		*current_button_data = current_data;
	}

	return (current_data != NULL);
}

static void
xml_breadcrumbs_update_path (XmlBreadcrumbs *path_bar,
							 GtkTreePath 	*treepath)
{
    
    GList		*new_buttons, *l;
	ButtonData	*button_data;
	GtkTreePath	*path;
	GtkTreePath	*root;

    g_return_if_fail (XML_IS_BREADCRUMBS (path_bar));
    g_return_if_fail (treepath != NULL);

	path = gtk_tree_path_copy(treepath);
	root = gtk_tree_path_new_first();
	gboolean nextpath = TRUE;
    gboolean first_directory = TRUE;
	new_buttons = NULL;

    gtk_widget_push_composite_child ();

    while (nextpath == TRUE) {
			
		button_data = make_button_data (path_bar, path, first_directory);
        new_buttons = g_list_prepend (new_buttons, button_data);
		
		if (first_directory) {
			first_directory = FALSE;
		}
		
		nextpath = gtk_tree_path_up(path);

		if(gtk_tree_path_compare(root, path) == 0) {
			break;
			//button_data->is_root = 0;
		}
    }

	xml_breadcrumbs_clear_buttons (path_bar);
	path_bar->priv->button_list = g_list_reverse (new_buttons);

	for (l = path_bar->priv->button_list; l; l = l->next) {
		GtkWidget *button;
		button = BUTTON_DATA (l->data)->button;
		gtk_container_add (GTK_CONTAINER (path_bar), button);
	}

	gtk_widget_pop_composite_child ();
	child_ordering_changed (path_bar);
}


void
xml_breadcrumbs_set_path_from_path ( XmlBreadcrumbs *path_bar, 
									  GtkTreePath	*treepath )
{
	ButtonData *button_data;

	g_return_if_fail (XML_IS_BREADCRUMBS (path_bar));
    g_return_if_fail (treepath != NULL);

	if (!xml_breadcrumbs_check_parent_path (path_bar, treepath, &button_data))
	{
		xml_breadcrumbs_update_path (path_bar, treepath);
		button_data = g_list_nth_data (path_bar->priv->button_list, 0);
	}

	if (path_bar->priv->current_path != NULL)
		gtk_tree_path_free(path_bar->priv->current_path);

	path_bar->priv->current_path = gtk_tree_path_copy(treepath);
	path_bar->priv->current_button_data = button_data;

}

void
xml_breadcrumbs_set_path_from_xpath ( XmlBreadcrumbs *path_bar, 
									  gchar * xpath )
{
	GtkTreePath * treepath;

	g_return_if_fail (XML_IS_BREADCRUMBS (path_bar));
    g_return_if_fail (xpath != NULL);
	
	treepath = xml_tree_model_get_path_from_xpath(path_bar->priv->model, xpath);
    g_return_if_fail (treepath != NULL);

	xml_breadcrumbs_set_path_from_path(path_bar, treepath);
   
}

void
xml_breadcrumbs_set_path_from_position ( XmlBreadcrumbs *path_bar, 
										 gint position )
{
	GtkTreePath * treepath;

	g_return_if_fail (XML_IS_BREADCRUMBS (path_bar));
    g_return_if_fail (position != NULL);
	
	treepath = xml_tree_model_get_path_from_position(path_bar->priv->model, position);
    g_return_if_fail (treepath != NULL);

	xml_breadcrumbs_set_path_from_path(path_bar, treepath);
}

void
xml_breadcrumbs_set_model(XmlBreadcrumbs *breadcrumbs, xmlTreeModel *model)
{
	breadcrumbs->priv->model = model;
	xml_breadcrumbs_set_path_from_path(breadcrumbs, gtk_tree_path_new_from_string("0:0"));
}

XmlBreadcrumbs
* xml_breadcrumbs_new ( void )
{
	xml_icon_factory_new();
	XmlBreadcrumbs * newbc;
	guint n_properties;
	newbc = g_object_new (XML_TYPE_BREADCRUMBS, NULL);

	GParamSpec ** nptr = gtk_container_class_list_child_properties (newbc, &n_properties);
	return newbc;
	//return GTK_WIDGET(g_object_new (XML_TYPE_BREADCRUMBS, NULL));
}
