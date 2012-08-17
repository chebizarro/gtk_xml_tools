#include "xmltools.h"

static void add_stock_icon (GtkIconFactory *factory,
				gchar *location,
				gchar *stock_id);


XmlNodeTypes XmlNodes[] =
{
	{ "xml-base-node", NULL },
	{ "xml-element-node", "Element Node" },
	{ "xml-attribute-node", "Attribute Node" }, 
	{ "xml-text-node", "Text Node" }, 
	{ "xml-cdata-section-node", "CDATA Node" }, 
	{ "xml-entity-ref-node", "Entity Reference Node" }, 
	{ "xml-entity-node", "Entity Node" }, 
	{ "xml-pi-node", "Processing Instruction" }, 
	{ "xml-comment-node", "Comment" }, 
	{ "xml-document-node", "Document" }, 
	{ "xml-document-type-node", "Document Type" }, 
	{ "xml-document-frag-node", "" }, 
	{ "xml-notation-node", "" }, 
	{ "xml-html-document-node", "" }, 
	{ "xml-dtd-node", "" }, 
	{ "xml-element-decl", "" }, 
	{ "xml-attribute-decl", "" }, 
	{ "xml-entity-decl", "" }, 
	{ "xml-namespace-decl", "" }, 
	{ "xml-xinclude-start", "" }, 
	{ "xml-xinclude-end", "" }, 
	{ "xml-docb-document-node", "" },
	{ NULL, NULL }
};

GtkIconFactory *
xml_icon_factory_new() {
	GtkIconFactory *factory;
	factory = gtk_icon_factory_new ();
	gint i = 0;

	/* Loop through the list of items and add new stock items. */
	while (XmlNodes[i].stock_id != NULL)
	{
		gchar * iconpath;
		iconpath = g_strconcat(ICON_LOCATION,XmlNodes[i].stock_id, ".png", NULL);
		add_stock_icon (factory, iconpath, XmlNodes[i].stock_id);
		i++;
	}
	gtk_icon_factory_add_default (factory);
	return factory;
}

/* Add a new stock icon from the given location and with the given stock id. */
static void
add_stock_icon (GtkIconFactory *factory,
				gchar *location,
				gchar *stock_id)
{
	GtkIconSource *source;
	GtkIconSet *set;
	source = gtk_icon_source_new ();
	set = gtk_icon_set_new ();
	gtk_icon_source_set_filename (source, location);
	gtk_icon_set_add_source (set, source);
	gtk_icon_factory_add (factory, stock_id, set);
}
