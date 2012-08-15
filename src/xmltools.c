#include "xmltools.h"

static void add_stock_icon (GtkIconFactory *factory,
				gchar *location,
				gchar *stock_id);


XmlNodeTypes XmlNodes[] =
{
	{ "xml_base_node", NULL },
	{ "xml_element_node", "Element Node" },
	{ "xml_attribute_node", "Attribute Node" }, 
	{ "xml_text_node", "" }, 
	{ "xml_cdata_section_node", "" }, 
	{ "xml_entity_ref_node", "" }, 
	{ "xml_entity_node", "" }, 
	{ "xml_pi_node", "" }, 
	{ "xml_comment_node", "" }, 
	{ "xml_document_node", "" }, 
	{ "xml_document_type_node", "" }, 
	{ "xml_document_frag_node", "" }, 
	{ "xml_notation_node", "" }, 
	{ "xml_html_document_node", "" }, 
	{ "xml_dtd_node", "" }, 
	{ "xml_element_decl", "" }, 
	{ "xml_attribute_decl", "" }, 
	{ "xml_entity_decl", "" }, 
	{ "xml_namespace_decl", "" }, 
	{ "xml_xinclude_start", "" }, 
	{ "xml_xinclude_end", "" }, 
	{ "xml_docb_document_node", "" },
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
		iconpath = g_strconcat("/home/bizarro/Documents/Dev/gtk_xml_tools/gtk_xml_tools/icons/",XmlNodes[i].stock_id, ".png", NULL);
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
