#ifndef _xml_tools_h_included_
#define _xml_tools_h_included_
 
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

/* XML/XSLT Libraries */
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#define ICON_LOCATION "/media/shared/Development/Source/gtk_xml_tools/icons/"

typedef struct
{
	gchar *stock_id;
	gchar *label;
} XmlNodeTypes;

extern XmlNodeTypes XmlNodes[];

GtkIconFactory * xml_icon_factory_new();


#endif /* _xml_tools_h_included_ */
