#include "xmltools.h"

static void add_stock_icon (	GtkIconFactory *factory,
								gchar *location,
								gchar *stock_id);


XmlNodeTypes XmlNodes[] =
{
	{ "xml-base-node", NULL },
	{ "xml-element-node", "Element" },
	{ "xml-attribute-node", "Attribute" }, 
	{ "xml-text-node", "Text" }, 
	{ "xml-cdata-section-node", "CDATA Section" }, 
	{ "xml-entity-ref-node", "Entity Reference" }, 
	{ "xml-entity-node", "Entity" }, 
	{ "xml-pi-node", "Processing Instruction" }, 
	{ "xml-comment-node", "Comment" }, 
	{ "xml-document-node", "Document" }, 
	{ "xml-document-type-node", "Document Type" }, 
	{ "xml-document-frag-node", "Document Fragment" }, 
	{ "xml-notation-node", "Notation" }, 
	{ "xml-html-document-node", "HTML Document" }, 
	{ "xml-dtd-node", "DTD" }, 
	{ "xml-element-decl", "Element Declaration" }, 
	{ "xml-attribute-decl", "Attribute Declaration" }, 
	{ "xml-entity-decl", "Entity Declaration" }, 
	{ "xml-namespace-decl", "Namespace Declaration" }, 
	{ "xml-xinclude-start", "Xinclude Start" }, 
	{ "xml-xinclude-end", "Xinclude End" }, 
	{ "xml-docb-document-node", "DOCB Document" },
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
