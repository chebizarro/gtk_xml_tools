#include "xmltools.h"
#include "xmltreemodel.h"
 
/* boring declarations of local functions */
 
static void         xml_list_init            (XmlList      *pkg_tree);
 
static void         xml_list_class_init      (XmlListClass *klass);
 
static void         xml_list_tree_model_init (GtkTreeModelIface *iface);
 
static void         xml_list_finalize        (GObject           *object);
 
static GtkTreeModelFlags xml_list_get_flags  (GtkTreeModel      *tree_model);
 
static gint         xml_list_get_n_columns   (GtkTreeModel      *tree_model);
 
static GType        xml_list_get_column_type (GtkTreeModel      *tree_model,
                                                 gint               index);
 
static gboolean     xml_list_get_iter        (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreePath       *path);
 
static GtkTreePath *xml_list_get_path        (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);
 
static void         xml_list_get_value       (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 gint               column,
                                                 GValue            *value);
 
static gboolean     xml_list_iter_next       (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);
 
static gboolean     xml_list_iter_children   (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *parent);
 
static gboolean     xml_list_iter_has_child  (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);
 
static gint         xml_list_iter_n_children (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);
 
static gboolean     xml_list_iter_nth_child  (GtkTreeModel      *tree_model,
                                                 GtkTreeIter		*iter,
                                                 GtkTreeIter		*parent,
                                                 gint               n);
 
static gboolean     xml_list_iter_parent     (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *child);
 
static xmlDocPtr throw_xml_error();
												 

static xmlNodePtr xmlGetParentNode(xmlNodePtr child);
static xmlNodePtr xmlDocGetRootElementN(xmlNodePtr node);
static xmlNodePtr xmlNextElementSiblingN(xmlNodePtr node);
static xmlNodePtr xmlFirstElementChildN(xmlNodePtr node);
static unsigned long xmlChildElementCountN(xmlNodePtr node);
static xmlNodePtr xmlPreviousElementSiblingN(xmlNodePtr node);


static GObjectClass *parent_class = NULL;  /* GObject stuff - nothing to worry about */



static xmlXPathObjectPtr evaluate_xpath(xmlDoc *doc, gchar *xpath) {
	if(xpath && doc) {

		xmlXPathContextPtr xpathCtx; 
		xmlXPathObjectPtr xpathObj;
		/* Create xpath evaluation context */
		xpathCtx = xmlXPathNewContext(doc);
		if(xpathCtx == NULL) {
			return NULL;
		}
    	/* Evaluate xpath expression */
		xpathObj = xmlXPathEvalExpression((xmlChar *)xpath, xpathCtx);
		xmlXPathFreeContext(xpathCtx);
		if(xpathObj == NULL) {
			return NULL;
		}
		return xpathObj;
	}
	return NULL;
}

/* XML Error Handling */ 
static xmlDocPtr throw_xml_error(){
	xmlErrorPtr err = xmlGetLastError();
	xmlDocPtr doc = NULL;
	xmlNodePtr errorNode = NULL;	
    char buff[256];

    doc = xmlNewDoc((xmlChar *)"1.0");
	sprintf(buff, "%s", err->file);
	xmlNewDocProp(doc, (xmlChar *) "name", (xmlChar *) buff);
	
    errorNode = xmlNewNode(NULL,(xmlChar *) "ERROR");
    xmlDocSetRootElement(doc, errorNode);

	sprintf(buff, "%d", err->domain);
	xmlNewProp(errorNode, (xmlChar *) "domain", (xmlChar *) buff);
	sprintf(buff, "%d", err->code);
	xmlNewProp(errorNode, (xmlChar *) "code", (xmlChar *) buff);
	sprintf(buff, "%d", err->level);
	xmlNewProp(errorNode, (xmlChar *) "level", (xmlChar *) buff);
	sprintf(buff, "%s", err->file);
	xmlNewProp(errorNode, (xmlChar *) "file", (xmlChar *) buff);
	sprintf(buff, "%d", err->line);
	xmlNewProp(errorNode, (xmlChar *) "line", (xmlChar *) buff);
	
	xmlAddChild(errorNode, xmlNewText(err->message));
	
	return doc;
	
}

static xmlNodePtr xmlGetRoot(XmlList *xml_list) {
	if(xml_list->xpath != NULL)
		return (xmlNodePtr) xml_list->xpath_results;

	return (xmlNodePtr)xml_list->xmldoc;
}

static xmlNodePtr xmlGetParentNode(xmlNodePtr child) {
	if(child != NULL)
		return child->parent;
		
	return NULL;
}

static xmlNodePtr xmlDocGetRootElementN(xmlNodePtr node) {
	g_return_val_if_fail(node != NULL, NULL);
	
	//if(xml_list->xpath != NULL)
	//	return (xmlNodePtr) xml_list->xpath_results;

	return (xmlNodePtr)node->doc;
	
}

static xmlNodePtr xmlNextElementSiblingN(xmlNodePtr node) {

	g_return_val_if_fail(node != NULL, NULL);

	xmlNodePtr record = NULL;
	
	switch(node->type)
	{
	case XML_ELEMENT_NODE:
	case XML_DTD_NODE:
	case XML_ELEMENT_DECL:
	case XML_ATTRIBUTE_DECL:
	case XML_ENTITY_DECL:
	case XML_NAMESPACE_DECL:
	case XML_TEXT_NODE:
	case XML_DOCUMENT_NODE:
	case XML_CDATA_SECTION_NODE:
	case XML_ENTITY_REF_NODE:
	case XML_ENTITY_NODE:
	case XML_PI_NODE:
	case XML_COMMENT_NODE:
	case XML_DOCUMENT_TYPE_NODE:
	case XML_DOCUMENT_FRAG_NODE:
	case XML_NOTATION_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_XINCLUDE_START:
	case XML_XINCLUDE_END:
	case XML_DOCB_DOCUMENT_NODE:
		record = node->next;
		break;
	case XML_ATTRIBUTE_NODE:
		record = node->next;
		if(record == NULL)
			record = xmlGetParentNode(node)->children;
		break;
	}
	
	while(xmlIsBlankNode(record)==1) {
		record = record->next;
	}
	
	return record;
}

static xmlNodePtr xmlFirstElementChildN(xmlNodePtr node) {

	g_return_val_if_fail(node != NULL, NULL);

	xmlNodePtr record = NULL;
	switch(node->type)
	{
	case XML_ELEMENT_DECL:
	case XML_ATTRIBUTE_DECL:
	case XML_ENTITY_DECL:
	case XML_NAMESPACE_DECL:
	case XML_ATTRIBUTE_NODE:
	case XML_CDATA_SECTION_NODE:
	case XML_ENTITY_REF_NODE:
	case XML_ENTITY_NODE:
	case XML_PI_NODE:
	case XML_COMMENT_NODE:
	case XML_DOCUMENT_TYPE_NODE:
	case XML_DOCUMENT_FRAG_NODE:
	case XML_NOTATION_NODE:
	case XML_XINCLUDE_START:
	case XML_XINCLUDE_END:
	case XML_DOCB_DOCUMENT_NODE:
	case XML_TEXT_NODE:
		return NULL;
	case XML_ELEMENT_NODE:
	case XML_DOCUMENT_NODE:
	case XML_HTML_DOCUMENT_NODE:
			record = (xmlNodePtr)node->properties;
	case XML_DTD_NODE:
		if(record == NULL)
			record = node->children;
			
		while(xmlIsBlankNode(record)==1) {
			record = record->next;
		}
		break;
	}
	
	if(record == NULL)
		return NULL;


	return record;

}

static unsigned long xmlChildElementCountN(xmlNodePtr node) {

	g_return_val_if_fail(node != NULL, NULL);

	xmlNodePtr elements = NULL;
	unsigned long count = 0;

	switch(node->type)
	{
	case XML_ELEMENT_DECL:
	case XML_ATTRIBUTE_DECL:
	case XML_ENTITY_DECL:
	case XML_NAMESPACE_DECL:
	case XML_ATTRIBUTE_NODE:
	case XML_CDATA_SECTION_NODE:
	case XML_ENTITY_REF_NODE:
	case XML_ENTITY_NODE:
	case XML_PI_NODE:
	case XML_COMMENT_NODE:
	case XML_DOCUMENT_TYPE_NODE:
	case XML_DOCUMENT_FRAG_NODE:
	case XML_NOTATION_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_XINCLUDE_START:
	case XML_XINCLUDE_END:
	case XML_DOCB_DOCUMENT_NODE:
	case XML_TEXT_NODE:
		return count;
	case XML_ELEMENT_NODE:
	case XML_DOCUMENT_NODE:
		elements = (xmlNodePtr)node->properties;

		while(elements != NULL) {
			count++;
			elements = elements->next;
		}

	case XML_DTD_NODE:
		elements = node->children;

		while (elements != NULL) {
			if(xmlIsBlankNode(elements)==0)
				count++;
			elements = elements->next;
		}

	}


	return count;
}

static xmlNodePtr xmlPreviousElementSiblingN(xmlNodePtr node) {

	g_return_val_if_fail(node != NULL, NULL);

	xmlNodePtr record = NULL;
	
	switch(node->type)
	{
	case XML_DTD_NODE:
	case XML_ELEMENT_DECL:
	case XML_ATTRIBUTE_DECL:
	case XML_ENTITY_DECL:
	case XML_NAMESPACE_DECL:
	case XML_TEXT_NODE:
	case XML_CDATA_SECTION_NODE:
	case XML_ENTITY_REF_NODE:
	case XML_ENTITY_NODE:
	case XML_PI_NODE:
	case XML_COMMENT_NODE:
	case XML_DOCUMENT_TYPE_NODE:
	case XML_DOCUMENT_FRAG_NODE:
	case XML_NOTATION_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_XINCLUDE_START:
	case XML_XINCLUDE_END:
	case XML_DOCB_DOCUMENT_NODE:
	case XML_ATTRIBUTE_NODE:
		record = node->prev;
		break;

	case XML_ELEMENT_NODE:
	case XML_DOCUMENT_NODE:
		record = node->prev;
		if(record == NULL)
			record = xmlGetParentNode(node)->properties;
		break;
	}
	
	while(xmlIsBlankNode(record)==1) {
		record = record->prev;
	}
	
	return record;
}


 
/*****************************************************************************
 *
 *  xml_list_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/
 
GType
xml_list_get_type (void)
{
  static GType xml_list_type = 0;
 
  /* Some boilerplate type registration stuff */
  if (xml_list_type == 0)
  {
    static const GTypeInfo xml_list_info =
    {
      sizeof (XmlListClass),
      NULL,                                         /* base_init */
      NULL,                                         /* base_finalize */
      (GClassInitFunc) xml_list_class_init,
      NULL,                                         /* class finalize */
      NULL,                                         /* class_data */
      sizeof (XmlList),
      0,                                           /* n_preallocs */
      (GInstanceInitFunc) xml_list_init
    };
    static const GInterfaceInfo tree_model_info =
    {
      (GInterfaceInitFunc) xml_list_tree_model_init,
      NULL,
      NULL
    };
 
    /* First register the new derived type with the GObject type system */
    xml_list_type = g_type_register_static (G_TYPE_OBJECT, "XmlList",
                                               &xml_list_info, (GTypeFlags)0);

	/*xml_list_type = g_type_module_register_type(G_TYPE_OBJECT, "XmlList",
                                               &xml_list_info, (GTypeFlags)0);*/
//g_type_register_dynamic(G_TYPE_OBJECT, "XmlList", , (GTypeFlags)0);
	  
    /* Now register our GtkTreeModel interface with the type system */
    g_type_add_interface_static (xml_list_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
  }
 
  return xml_list_type;
}
 
 
/*****************************************************************************
 *
 *  xml_list_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/
 
static void
xml_list_class_init (XmlListClass *klass)
{
  GObjectClass *object_class;
 
  parent_class = (GObjectClass*) g_type_class_peek_parent (klass);
  object_class = (GObjectClass*) klass;
 
  object_class->finalize = xml_list_finalize;
}
 
/*****************************************************************************
 *
 *  xml_list_tree_model_init: init callback for the interface registration
 *                               in xml_list_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/
 
static void
xml_list_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags       = xml_list_get_flags;
  iface->get_n_columns   = xml_list_get_n_columns;
  iface->get_column_type = xml_list_get_column_type;
  iface->get_iter        = xml_list_get_iter;
  iface->get_path        = xml_list_get_path;
  iface->get_value       = xml_list_get_value;
  iface->iter_next       = xml_list_iter_next;
  iface->iter_children   = xml_list_iter_children;
  iface->iter_has_child  = xml_list_iter_has_child;
  iface->iter_n_children = xml_list_iter_n_children;
  iface->iter_nth_child  = xml_list_iter_nth_child;
  iface->iter_parent     = xml_list_iter_parent;
}
 
 
/*****************************************************************************
 *
 *  xml_list_init: this is called everytime a new xml list object
 *                    instance is created (we do that in xml_list_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/
 
static void
xml_list_init (XmlList *xml_list)
{

	LIBXML_TEST_VERSION /* Arrrgh - where should this go??? */
	
	xml_list->n_columns       = XML_LIST_N_COLUMNS;
	xml_list->column_types[0] = G_TYPE_STRING;	/* XML_LIST_COL_TYPE	*/
	xml_list->column_types[1] = G_TYPE_STRING;	/* XML_LIST_COL_NAME	*/
	xml_list->column_types[2] = G_TYPE_STRING;	/* XML_LIST_COL_CONTENT	*/
	xml_list->column_types[3] = G_TYPE_INT;		/* XML_LIST_COL_LINE	*/
	xml_list->column_types[4] = G_TYPE_BOOLEAN;	/* XML_LIST_COL_VISIBLE	*/
	xml_list->column_types[5] = G_TYPE_STRING;	/* XML_LIST_COL_PATH	*/


	g_assert (XML_LIST_N_COLUMNS == 6);
 
	xml_list->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */
	xml_list->xmldoc = NULL;
	xml_list->xpath_results = NULL;
	xml_list->xpath = NULL;

	xml_list->row_visible[XML_ELEMENT_NODE] = TRUE;
	xml_list->row_visible[XML_DOCUMENT_NODE] = TRUE;
	xml_list->row_visible[XML_ATTRIBUTE_NODE] = TRUE;
	xml_list->row_visible[XML_TEXT_NODE] = FALSE;
}
 
 
/*****************************************************************************
 *
 *  xml_list_finalize: this is called just before a xml list is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/
 
static void
xml_list_finalize (GObject *object)
{
	XmlList *xml_list = XML_LIST(object);
	if(xml_list->xpath)
		//xmlFreeDoc(xml_list->xpath_results);

	if(xml_list->xmldoc)
		xmlFreeDoc(xml_list->xmldoc);
	
	xmlCleanupParser();

	/* must chain up - finalize parent */
	(* parent_class->finalize) (object);
}
 
 
/*****************************************************************************
 *
 *  xml_list_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics.
 *
 *****************************************************************************/
 
static GtkTreeModelFlags
xml_list_get_flags (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (XML_IS_LIST(tree_model), (GtkTreeModelFlags)0);
 
  return ((GtkTreeModelFlags)0);
}
 
 
/*****************************************************************************
 *
 *  xml_list_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/
 
static gint
xml_list_get_n_columns (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (XML_IS_LIST(tree_model), 0);
 
  return XML_LIST(tree_model)->n_columns;
}
 
 
/*****************************************************************************
 *
 *  xml_list_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/
 
static GType
xml_list_get_column_type (GtkTreeModel *tree_model,
                             gint          index)
{
  g_return_val_if_fail (XML_IS_LIST(tree_model), G_TYPE_INVALID);
  g_return_val_if_fail (index < XML_LIST(tree_model)->n_columns && index >= 0, G_TYPE_INVALID);
 
  return XML_LIST(tree_model)->column_types[index];
}


/*****************************************************************************
 *
 *  xml_list_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure (the content of the iter
 *                        fields will only be used internally by our model).
 *                        We simply store a pointer to our XmlRecord
 *                        structure that represents that row in the tree iter.
 *
 *****************************************************************************/

static gboolean
xml_list_get_iter (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter,
                      GtkTreePath  *path)
{
	XmlList		*xml_list;
 
	g_assert(XML_IS_LIST(tree_model));
	g_assert(path!=NULL);
 
	xml_list = XML_LIST(tree_model);
 
	gint *indices = gtk_tree_path_get_indices(path);
	gint depth   = gtk_tree_path_get_depth(path);

	xmlNodePtr tree = xmlGetRoot(xml_list);
	xmlNodePtr result;
	
	if(tree != NULL) {
		for (int d = 0; d < depth; d++) {
			result = tree;
			for(int i = 0; i < indices[d]; i++) {
				tree = xmlNextElementSiblingN (tree);
			}
			result = tree;
			tree = xmlFirstElementChildN (tree);			
		}
	}

	if (!result)
		return FALSE;

	iter->user_data = result;
	iter->stamp = xml_list->stamp; 
	iter->user_data = result; 
	
	return TRUE; 
}

/*****************************************************************************
 *
 *  xml_list_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/
 
static GtkTreePath *
xml_list_get_path (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter)
{
	g_return_val_if_fail (XML_IS_LIST(tree_model), NULL); 
	g_return_val_if_fail (iter != NULL, NULL); 

	XmlList *xmllist = XML_LIST(tree_model); 

	xmlNodePtr node = iter->user_data;
	xmlNodePtr tree = xmlDocGetRootElementN(xmlGetRoot(xmllist));
	GtkTreePath *path = gtk_tree_path_new();
	int i = 0;

	while(node != tree && node != NULL) {
		if(xmlPreviousElementSiblingN(node) == NULL) {
			gtk_tree_path_prepend_index (path, i);
			node = xmlGetParentNode (node);
			i = 0;
		} else {
			node = xmlPreviousElementSiblingN(node);
			i++;
		}
	}

	gtk_tree_path_prepend_index (path, 0);
	
	return path; 
}
 
 
/*****************************************************************************
 *
 *  xml_list_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/
 
static void
xml_list_get_value (GtkTreeModel *tree_model,
                       GtkTreeIter  *iter,
                       gint          column,
                       GValue       *value)
{
	xmlNodePtr	record;
 
	g_return_if_fail (XML_IS_LIST (tree_model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (column < XML_LIST(tree_model)->n_columns);
	g_return_if_fail (xmlGetRoot(XML_LIST(tree_model)) != NULL);

	g_value_init (value, XML_LIST(tree_model)->column_types[column]);
 
	record = iter->user_data;

	g_return_if_fail ( record != NULL );

	switch(column)
	{
	case XML_LIST_COL_TYPE:
    	g_value_set_string(value, XmlNodes[record->type].stock_id);
			break;
 
		case XML_LIST_COL_NAME:
		{
			
			if(record->type == XML_DOCUMENT_NODE) {
				xmlDocPtr dorec = record;
				g_value_set_string(value,dorec->version);
			} else {
				g_value_set_string(value,(gchar *) record->name);
			}
			
			break;
		}
		case XML_LIST_COL_CONTENT:
		
		switch(record->type)
		{
		case XML_ATTRIBUTE_NODE:
		case XML_TEXT_NODE:
		case XML_COMMENT_NODE:
		case XML_ATTRIBUTE_DECL:
		case XML_ENTITY_DECL:
		case XML_NAMESPACE_DECL:
		{
			g_value_set_string(value,xmlNodeGetContent(record));			
			break;
		}
		case XML_DTD_NODE:
		{
			//xmlDtdPtr dtd = (xmlDtdPtr)record;
			//g_value_set_string(value,dtd->SystemID);			
			break;
		}
		case XML_DOCUMENT_NODE:
		case XML_ELEMENT_NODE:
		{
			if(record->children) {
				record = record->children;
				while(xmlIsBlankNode(record)==1) {
					record = record->next;
				}
				if(record->type == XML_TEXT_NODE)
					g_value_set_stdring(value,g_strstrip(xmlNodeGetContent(record)));
			}
		break;
		}
	}	
	case XML_LIST_COL_LINE:
		g_value_set_int(value, (gint) xmlGetLineNo(record));
		break;

	case XML_LIST_COL_VISIBLE:
		g_value_set_boolean(value, XML_LIST(tree_model)->row_visible[record->type]);
		break;

	case XML_LIST_COL_XPATH:
		g_value_set_string(value, xmlGetNodePath(record));
		break;

	}

}
 
 
/*****************************************************************************
 *
 *  xml_list_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/
 
static gboolean
xml_list_iter_next (GtkTreeModel  *tree_model,
                       GtkTreeIter   *iter)
{
	xmlNodePtr record, nextrecord;
	XmlList *xml_list;

	g_return_val_if_fail (XML_IS_LIST (tree_model), FALSE);
 
	if (iter == NULL || iter->user_data == NULL)
		return FALSE;
 
	xml_list = XML_LIST(tree_model);
 
	record = iter->user_data;
 
	/* Is this the last node in the list? */
	if(xmlNextElementSiblingN(record) == NULL)
		return FALSE;
 
	nextrecord = xmlNextElementSiblingN(record);
 
	g_assert ( nextrecord != NULL );

	iter->stamp     = xml_list->stamp;
	iter->user_data = nextrecord;
 
	return TRUE;
}
 
 
/*****************************************************************************
 *
 *  xml_list_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/
 
static gboolean
xml_list_iter_children (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter,
                           GtkTreeIter  *parent)
{
	XmlList  	*xml_list;
	xmlNodePtr		record;
	
	g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);

	g_return_val_if_fail (XML_IS_LIST (tree_model), FALSE);
	xml_list = XML_LIST(tree_model);

	if(parent != NULL) {
		record = parent->user_data;
		if(xmlFirstElementChildN(record) == NULL) {
			return FALSE;
		} else {
			iter->user_data = xmlFirstElementChildN(record);		
		}
	} else {
		if(xmlGetRoot(xml_list)) {
			iter->user_data = xmlDocGetRootElementN(xmlGetRoot(xml_list));
		}
	}
	iter->stamp = xml_list->stamp;
 	return TRUE;
}
 
 
/*****************************************************************************
 *
 *  xml_list_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *
 *****************************************************************************/
 
static gboolean
xml_list_iter_has_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
	
	if(xmlChildElementCountN(iter->user_data) > 0)
		return TRUE;
		
	return FALSE;
}
 
 
/*****************************************************************************
 *
 *  xml_list_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/
 
static gint
xml_list_iter_n_children (GtkTreeModel *tree_model,
                             GtkTreeIter  *iter)
{
	XmlList  *xml_list;
	gint num_rows = 0;

	g_return_val_if_fail (XML_IS_LIST (tree_model), -1);
	g_return_val_if_fail (iter == NULL || iter->user_data != NULL, FALSE);
 
	xml_list = XML_LIST(tree_model);
 
	/* special case: if iter == NULL, return number of top-level rows */
	if (iter == NULL) {
		num_rows = xmlChildElementCountN(xmlDocGetRootElementN(xmlGetRoot(xml_list)));
	} else {
		num_rows = xmlChildElementCountN(iter->user_data);
	}
	
	return num_rows;
}
 
 
/*****************************************************************************
 *
 *  xml_list_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/
 
static gboolean
xml_list_iter_nth_child (GtkTreeModel *tree_model,
							GtkTreeIter  *iter,
                            GtkTreeIter  *parent,
							gint          n)
{
	xmlNodePtr		cursor, record;
	XmlList		*xml_list;

	g_return_val_if_fail (XML_IS_LIST (tree_model), FALSE);
 
	xml_list = XML_LIST(tree_model);
 
	/* special case: if parent == NULL, set iter to n-th top-level row */

	//record = iter->user_data;

	if(parent == NULL) {
		record = xmlGetRoot(xml_list);
	} else {
		record = parent->user_data;
	}
	
	cursor = xmlFirstElementChildN(record);

	gint i=0;

	for(i=0; i==n; i++) {
		if(cursor) {
			if(i == n) {
				iter->user_data = cursor;
			} else {
				cursor = xmlNextElementSiblingN(cursor);
			}
		} else {
			return FALSE;
		}
	}
 
  iter->stamp = xml_list->stamp;
 
  return TRUE;
}
 
 
/*****************************************************************************
 *
 *  xml_list_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/
 
static gboolean
xml_list_iter_parent (GtkTreeModel *tree_model,
                         GtkTreeIter  *iter,
                         GtkTreeIter  *child)
{
	XmlList  	*xml_list;
	xmlNodePtr		record;
 
	g_return_val_if_fail (child == NULL || child->user_data != NULL, FALSE);

	g_return_val_if_fail (XML_IS_LIST (tree_model), FALSE);
	xml_list = XML_LIST(tree_model);

	if(child != NULL) {
		record = child->user_data;
		if(xmlGetParentNode (record) == NULL) {
			return FALSE;
		} else {
			iter->user_data = xmlGetParentNode (record);		
		}
	} else {
		if(xmlGetRoot(xml_list)) {
			iter->user_data = xmlDocGetRootElementN(xmlGetRoot(xml_list));
		}
	}
	iter->stamp = xml_list->stamp;
 	return TRUE;
}
 
 
/*****************************************************************************
 *
 *  xml_list_new:  This is what you use in your own code to create a
 *                    new xml list tree model for you to use.
 *
 *****************************************************************************/
 
XmlList *
xml_list_new (void)
{
  XmlList *newxmllist;
 
  newxmllist = (XmlList*) g_object_new (XML_TYPE_LIST, NULL);
 
  g_assert( newxmllist != NULL );
 
  return newxmllist;
}
 
/*****************************************************************************
 *
 *  xml_list_add_file:		Adds am XML file to the list.
 *
 *****************************************************************************/
 
void
xml_list_add_file (XmlList   *xml_list,
                      const gchar  *filename)
{
	xmlDocPtr	xdoc = NULL;

	g_return_if_fail (XML_IS_LIST(xml_list));
	//g_return_if_fail (filename != NULL);

	xdoc = xmlParseFile(filename);

	GtkTreeIter   iter;
	GtkTreePath  *path;

	path = gtk_tree_path_new();
	gtk_tree_path_prepend_index(path, 0);
	xml_list_get_iter(GTK_TREE_MODEL(xml_list), &iter, path);
	gtk_tree_model_row_deleted(GTK_TREE_MODEL(xml_list),path);
	gtk_tree_model_row_has_child_toggled(GTK_TREE_MODEL(xml_list),path, &iter);
	gtk_tree_path_free(path);

	if(xdoc != NULL) {

		//xml_list = xml_list_new();
		
		if(xml_list->xmldoc)
			xmlFreeDoc(xml_list->xmldoc);

		xml_list->xmldoc = xdoc;

	} else {
		//xml_list = xml_list_new();
		xml_list->xmldoc = throw_xml_error();
	}
	
	xml_list->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */

	path = gtk_tree_path_new();
	gtk_tree_path_prepend_index(path, 0);
	xml_list_get_iter(GTK_TREE_MODEL(xml_list), &iter, path);
	gtk_tree_model_row_inserted(GTK_TREE_MODEL(xml_list),path, &iter);
	gtk_tree_model_row_has_child_toggled(GTK_TREE_MODEL(xml_list),path, &iter);
	gtk_tree_path_free(path);
}
 
void
xml_list_set_visible (XmlList *xml_list, gint nodetype, gboolean visible) {
	g_return_if_fail (XML_IS_LIST(xml_list));
	//g_return_if_fail (nodetype == xmlElementType);

	xml_list->row_visible[nodetype] = visible;
	
}

void
xml_list_set_xpath(XmlList *xml_list, const gchar * xpath) {
	GtkTreeIter   iter;
	GtkTreePath *path;

	path = gtk_tree_path_new();
	gtk_tree_path_prepend_index(path, 0);
	xml_list_get_iter(GTK_TREE_MODEL(xml_list), &iter, path);
	gtk_tree_model_row_deleted(GTK_TREE_MODEL(xml_list),path);
	gtk_tree_model_row_has_child_toggled(GTK_TREE_MODEL(xml_list),path, &iter);
	gtk_tree_path_free(path);

	
	if(xml_list->xmldoc != NULL) {
		xmlXPathObjectPtr xpath_results;

		xpath_results = evaluate_xpath(xml_list->xmldoc, xpath);
		
		if(xpath_results != NULL) {
			xmlDocPtr doc = NULL;
			xmlNodePtr result = NULL;
			doc = xmlNewDoc("1.0");
			result = xmlNewNode(NULL,(xmlChar *) "XPATH RESULTS");
			xmlDocSetRootElement(doc, result);
		
			int size, i;
			size = (xpath_results->nodesetval) ? xpath_results->nodesetval->nodeNr : 0;

			for(i = 0; i < size; ++i) {
				if(xpath_results->nodesetval->nodeTab[i]->children)
					xmlAddChild(result, xpath_results->nodesetval->nodeTab[i]);
			}
			xml_list->xpath = xpath;
			xml_list->xpath_results = doc;
			xmlXPathFreeObject(xpath_results);

	path = gtk_tree_path_new();
	gtk_tree_path_prepend_index(path, 0);
	xml_list_get_iter(GTK_TREE_MODEL(xml_list), &iter, path);
	gtk_tree_model_row_inserted(GTK_TREE_MODEL(xml_list),path, &iter);
	gtk_tree_model_row_has_child_toggled(GTK_TREE_MODEL(xml_list),path, &iter);
	gtk_tree_path_free(path);
			
			return TRUE;
		}
	}
	return FALSE;
}
