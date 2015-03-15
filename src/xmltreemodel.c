#include "xmltools.h"
#include "xmltreemodel.h"

enum {
	XML_TREE_MODEL_CHANGED,
	XML_TREE_MODEL_ERROR,
	XML_TREE_MODEL_XSL_ERROR,
	XML_TREE_MODEL_LAST_SIGNAL
};

static guint xml_tree_model_signals[XML_TREE_MODEL_LAST_SIGNAL] = { 0 };


/* boring declarations of local functions */
 
static void         xml_tree_model_init            (xmlTreeModel      *model);
 
static void         xml_tree_model_class_init      (xmlTreeModelClass *klass);
 
static void         xml_tree_model_tree_model_init (GtkTreeModelIface *iface);
 
static void         xml_tree_model_finalize        (GObject           *object);
 
static GtkTreeModelFlags xml_tree_model_get_flags  (GtkTreeModel      *model);
 
static gint         xml_tree_model_get_n_columns   (GtkTreeModel      *model);
 
static GType        xml_tree_model_get_column_type (GtkTreeModel      *model,
                                                 gint               index);
 
static gboolean     xml_tree_model_get_iter        (GtkTreeModel      *model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreePath       *path);
 
static GtkTreePath *xml_tree_model_get_path        (GtkTreeModel      *model,
													GtkTreeIter       *iter);
 
static void         xml_tree_model_get_value       (GtkTreeModel      *model,
													 GtkTreeIter       *iter,
													 gint               column,
													 GValue            *value);
 
static gboolean     xml_tree_model_iter_next       (GtkTreeModel      *model,
													GtkTreeIter       *iter);
 
static gboolean     xml_tree_model_iter_children   (GtkTreeModel      *model,
													GtkTreeIter       *iter,
													GtkTreeIter       *parent);
 
static gboolean     xml_tree_model_iter_has_child  (GtkTreeModel      *model,
													GtkTreeIter       *iter);
 
static gint         xml_tree_model_iter_n_children (GtkTreeModel      *model,
													GtkTreeIter       *iter);
 
static gboolean     xml_tree_model_iter_nth_child  (GtkTreeModel      *model,
													GtkTreeIter		*iter,
													GtkTreeIter		*parent,
													gint               n);
 
static gboolean     xml_tree_model_iter_parent     (GtkTreeModel      *model,
													GtkTreeIter       *iter,
													GtkTreeIter       *child);
 
static void 		throw_xml_error					(xmlTreeModel * model,
													 xmlErrorPtr error);
												 

static xmlNodePtr	xmlGetParentNode				(xmlNodePtr child);
static xmlNodePtr xmlDocGetRootElementN(xmlNodePtr node);
static xmlNodePtr xmlNextElementSiblingN(xmlNodePtr node);
static xmlNodePtr xmlFirstElementChildN(xmlNodePtr node);
static unsigned long xmlChildElementCountN(xmlNodePtr node);
static xmlNodePtr xmlPreviousElementSiblingN(xmlNodePtr node);

void get_value_content(xmlNodePtr record, GValue * value);
void get_value_ns(xmlNodePtr record, GValue * value);

static GObjectClass *parent_class = NULL;  /* GObject stuff - nothing to worry about */

static xmlXPathObjectPtr evaluate_xpath(xmlDoc *doc, gchar *xpath) {
	
	g_return_val_if_fail(doc != NULL, NULL);
	g_return_val_if_fail(xpath != NULL, NULL);
	
	xmlXPathContextPtr xpathCtx; 
	xmlXPathObjectPtr xpathObj;
	int i = 0;

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);

	g_return_val_if_fail(xpathCtx != NULL, NULL);
	
	xmlNsPtr *nsp;
	nsp = xmlGetNsList(doc,xmlDocGetRootElement(doc));

	if (nsp != NULL) {
	  while (nsp[i] != NULL)
	    i++;
	}

	xpathCtx->namespaces = nsp;
	xpathCtx->nsNr = i;
	
	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression((xmlChar *)xpath, xpathCtx);

	xmlFree(nsp);
	xmlXPathFreeContext(xpathCtx);

	return xpathObj;
}

/* XML Error Handling */ 
static void throw_xml_error(xmlTreeModel * model, xmlErrorPtr error){
	g_return_if_fail(XML_IS_TREE_MODEL(model));
	g_return_if_fail(error != NULL);

	g_signal_emit(model, xml_tree_model_signals[XML_TREE_MODEL_ERROR],0,error);
}


static void throw_xsl_error(xmlTreeModel * model, const char * fmt,...)
{
	g_return_if_fail(XML_IS_TREE_MODEL(model));

	va_list ap;
    int i = 0;
    char *s;
	xslErrorMessage error = { NULL, NULL, 0, NULL };

	va_start(ap, fmt);

    while (*fmt)
		switch (*fmt++) {
        case 's':
            s = va_arg(ap, char *);
            
            int n = strlen(s);
         
			if (s[n-1] == 10) {
			  s[n-1] = NULL;
			}
			
            if(i == 0)
				error.error = s;
            
            if(i == 1)
				error.file = s;
				
			if(i == 2)
				error.element = s;
            
            i++;
            break;
        case 'd': 
            error.line = va_arg(ap, int);
            break;
        }
    va_end(ap);
	g_signal_emit(model, xml_tree_model_signals[XML_TREE_MODEL_XSL_ERROR],0,&error);
}

static xmlNodePtr xmlGetRoot(xmlTreeModel *model) {
	g_return_val_if_fail(XML_IS_TREE_MODEL(model), NULL);
	g_return_val_if_fail(model->xmldoc != NULL, NULL);
	return (xmlNodePtr)model->xmldoc;
}

static xmlNodePtr xmlGetParentNode(xmlNodePtr child) {
	g_return_val_if_fail(child != NULL, NULL);
	return child->parent;
}

static xmlNodePtr xmlDocGetRootElementN(xmlNodePtr node) {
	g_return_val_if_fail(node != NULL, NULL);

	return (xmlNodePtr)node->doc;
}

static xmlNodePtr xmlNextElementSiblingN(xmlNodePtr node) {

	g_return_val_if_fail(node != NULL, NULL);
	xmlNodePtr record = node->next;
	switch(node->type)
	{
		case XML_ATTRIBUTE_NODE:
			if(record == NULL)
				record = xmlGetParentNode(node)->children;
			break;
		default:
			break;
	}
	
	record != (NULL) ? record : NULL;

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
		case XML_ELEMENT_NODE:
		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
		case XML_PI_NODE:
				record = (xmlNodePtr)node->properties;
		case XML_DTD_NODE:
			if(record == NULL)
				record = node->children;
				
			while(xmlIsBlankNode(record)==1) {
				record = record->next;
			}
			break;
		default:
			break;
	}

	return record;
}

static unsigned long xmlChildElementCountN(xmlNodePtr node) {

	g_return_val_if_fail(node != NULL, 0);

	xmlNodePtr elements = NULL;
	unsigned long count = 0;

	switch(node->type)
	{
		case XML_ELEMENT_NODE:
		case XML_DOCUMENT_NODE:
		case XML_PI_NODE:
		case XML_HTML_DOCUMENT_NODE:

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
		default:
			break;
	}

	return count;
}

static xmlNodePtr xmlPreviousElementSiblingN(xmlNodePtr node) {

	g_return_val_if_fail(node != NULL, NULL);

	xmlNodePtr record = NULL; // = node->prev;
	
	switch(node->type)
	{
	case XML_ELEMENT_NODE:
		record = node->prev;

		while(xmlIsBlankNode(record)==1)
			record = record->prev;
						
		if(record == NULL) {
			record = xmlGetParentNode(node);
			record = record->properties;
			if(record != NULL) {
				while(record->next != NULL)
					record = record->next;
			}
		}
		
		break;
		
	case XML_TEXT_NODE:
		while(xmlIsBlankNode(record)==1)
			record = record->prev;
		break;
	case XML_ATTRIBUTE_NODE:
		record = node->prev;
		break;
	case XML_DOCUMENT_NODE:
	case XML_HTML_DOCUMENT_NODE:
		record = NULL;
		break;
	default:
		record = xmlPreviousElementSibling(node);
		break;
	}
		
	return record;
}

 
/*****************************************************************************
 *
 *  xml_tree_model_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/
 
GType
xml_tree_model_get_type (void)
{
  static GType xml_tree_model_type = 0;
 
  /* Some boilerplate type registration stuff */
  if (xml_tree_model_type == 0)
  {
    static const GTypeInfo xml_tree_model_info =
    {
      sizeof (xmlTreeModelClass),
      NULL,                                         /* base_init */
      NULL,                                         /* base_finalize */
      (GClassInitFunc) xml_tree_model_class_init,
      NULL,                                         /* class finalize */
      NULL,                                         /* class_data */
      sizeof (xmlTreeModel),
      0,                                           /* n_preallocs */
      (GInstanceInitFunc) xml_tree_model_init
    };
    static const GInterfaceInfo tree_model_info =
    {
      (GInterfaceInitFunc) xml_tree_model_tree_model_init,
      NULL,
      NULL
    };
 
    /* First register the new derived type with the GObject type system */
    xml_tree_model_type = g_type_register_static (G_TYPE_OBJECT, "xmlTreeModel",
                                               &xml_tree_model_info, (GTypeFlags)0);
  
    /* Now register our GtkTreeModel interface with the type system */
    g_type_add_interface_static (xml_tree_model_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
  }
 
  return xml_tree_model_type;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/
 
static void
xml_tree_model_class_init (xmlTreeModelClass *klass)
{
	GObjectClass *object_class;
 
	parent_class = (GObjectClass*) g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
 
	object_class->finalize = xml_tree_model_finalize;
  
	xml_tree_model_signals[XML_TREE_MODEL_CHANGED] = g_signal_new ("xml-tree-model-changed",
											G_TYPE_FROM_CLASS (klass),
											G_SIGNAL_RUN_FIRST,
											G_STRUCT_OFFSET (xmlTreeModelClass, xml_tree_model_changed),
											NULL, 
											NULL,								
											g_cclosure_marshal_VOID__POINTER,
											G_TYPE_NONE, 1, G_TYPE_POINTER);

	xml_tree_model_signals[XML_TREE_MODEL_ERROR] = g_signal_new ("xml-tree-model-error",
											G_TYPE_FROM_CLASS (klass),
											G_SIGNAL_RUN_FIRST,
											G_STRUCT_OFFSET (xmlTreeModelClass, xml_tree_model_error),
											NULL, 
											NULL,								
											g_cclosure_marshal_VOID__POINTER,
											G_TYPE_NONE, 1, G_TYPE_POINTER);

	xml_tree_model_signals[XML_TREE_MODEL_XSL_ERROR] = g_signal_new ("xml-tree-model-xsl-error",
											G_TYPE_FROM_CLASS (klass),
											G_SIGNAL_RUN_FIRST,
											G_STRUCT_OFFSET (xmlTreeModelClass, xml_tree_model_xsl_error),
											NULL, 
											NULL,								
											g_cclosure_marshal_VOID__POINTER,
											G_TYPE_NONE, 1, G_TYPE_POINTER);

}
 
/*****************************************************************************
 *
 *  xml_tree_model_tree_model_init: init callback for the interface registration
 *                               in xml_tree_model_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/
 
static void
xml_tree_model_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags       = xml_tree_model_get_flags;
  iface->get_n_columns   = xml_tree_model_get_n_columns;
  iface->get_column_type = xml_tree_model_get_column_type;
  iface->get_iter        = xml_tree_model_get_iter;
  iface->get_path        = xml_tree_model_get_path;
  iface->get_value       = xml_tree_model_get_value;
  iface->iter_next       = xml_tree_model_iter_next;
  iface->iter_children   = xml_tree_model_iter_children;
  iface->iter_has_child  = xml_tree_model_iter_has_child;
  iface->iter_n_children = xml_tree_model_iter_n_children;
  iface->iter_nth_child  = xml_tree_model_iter_nth_child;
  iface->iter_parent     = xml_tree_model_iter_parent;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_init: this is called everytime a new xml list object
 *                    instance is created (we do that in xml_tree_model_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/
 
static void
xml_tree_model_init (xmlTreeModel *model)
{
	xmlSetStructuredErrorFunc(model, throw_xml_error);
	xsltSetGenericErrorFunc(model, throw_xsl_error);
	
	model->n_columns       = XML_TREE_MODEL_N_COLUMNS;
	model->column_types[0] = G_TYPE_INT;	/* XML_TREE_MODEL_COL_TYPE	*/
	model->column_types[1] = G_TYPE_STRING;	/* XML_TREE_MODEL_COL_NS	*/
	model->column_types[2] = G_TYPE_STRING;	/* XML_TREE_MODEL_COL_NAME	*/
	model->column_types[3] = G_TYPE_STRING;	/* XML_TREE_MODEL_COL_CONTENT	*/
	model->column_types[4] = G_TYPE_INT;		/* XML_TREE_MODEL_COL_LINE	*/
	model->column_types[5] = G_TYPE_ULONG;		/* XML_TREE_MODEL_COL_POS	*/
	model->column_types[6] = G_TYPE_STRING;	/* XML_TREE_MODEL_COL_PATH	*/
	g_assert (XML_TREE_MODEL_N_COLUMNS == 7);
 
	model->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */
	model->xmldoc = NULL;
	model->xsldoc = NULL;
	model->xpath = NULL;
	model->valid = FALSE;
	
	model->parser = xmlNewParserCtxt();
	model->nodeinfo = NULL;
	model->offset = 0;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_finalize: this is called just before a xml list is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/
 
static void
xml_tree_model_finalize (GObject *object)
{
	xmlTreeModel *model = XML_TREE_MODEL(object);

	if(model->xsldoc) {
		xsltFreeStylesheet(model->xsldoc);
		model->xmldoc = NULL;
	}
	
	if(model->xmldoc)
		xmlFreeDoc(model->xmldoc);
	
	xmlCleanupParser();

	/* must chain up - finalize parent */
	(* parent_class->finalize) (object);
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics.
 *
 *****************************************************************************/
 
static GtkTreeModelFlags
xml_tree_model_get_flags (GtkTreeModel *model)
{
  g_return_val_if_fail (XML_IS_TREE_MODEL(model), (GtkTreeModelFlags)0);
 
  return ((GtkTreeModelFlags)0);
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/
 
static gint
xml_tree_model_get_n_columns (GtkTreeModel *model)
{
  g_return_val_if_fail (XML_IS_TREE_MODEL(model), 0);
 
  return XML_TREE_MODEL(model)->n_columns;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/
 
static GType
xml_tree_model_get_column_type (GtkTreeModel *model,
                             gint          index)
{
  g_return_val_if_fail (XML_IS_TREE_MODEL(model), G_TYPE_INVALID);
  g_return_val_if_fail (index < XML_TREE_MODEL(model)->n_columns && index >= 0, G_TYPE_INVALID);
 
  return XML_TREE_MODEL(model)->column_types[index];
}


/*****************************************************************************
 *
 *  xml_tree_model_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure 
 *
 *****************************************************************************/

static gboolean
xml_tree_model_get_iter (GtkTreeModel *model,
                      GtkTreeIter  *iter,
                      GtkTreePath  *path)
{
	g_return_val_if_fail(XML_IS_TREE_MODEL(model), FALSE);

	xmlNodePtr 	tree = xmlGetRoot(model);
	xmlNodePtr 	result = NULL;
 
	gint *indices = gtk_tree_path_get_indices(path);
	gint depth   = gtk_tree_path_get_depth(path);

	for (int d = 0; d < depth; d++) {
		result = tree;
		for(int i = 0; i < indices[d]; i++) {
			tree = xmlNextElementSiblingN (tree);
		}
		result = tree;
		tree = xmlFirstElementChildN (tree);			
	}

	if(result == NULL)
		return FALSE;

	iter->user_data = result;
	iter->stamp = XML_TREE_MODEL(model)->stamp; 
	
	return TRUE; 
}

/*****************************************************************************
 *
 *  xml_tree_model_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/
 
static GtkTreePath *
xml_tree_model_get_path (GtkTreeModel *model,
						GtkTreeIter  *iter)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL(model), NULL); 
	g_return_val_if_fail (iter != NULL, NULL); 

	xmlNodePtr node = iter->user_data;
	xmlNodePtr tree = xmlDocGetRootElementN(xmlGetRoot(XML_TREE_MODEL(model)));
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
 *  xml_tree_model_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/
 
static void
xml_tree_model_get_value (	GtkTreeModel *model,
							GtkTreeIter  *iter,
							gint          column,
							GValue       *value)
{
	g_return_if_fail (XML_IS_TREE_MODEL (model));
	g_return_if_fail (XML_TREE_MODEL (model)->xmldoc != NULL);
	g_return_if_fail (iter != NULL);
	g_return_if_fail (iter->user_data != NULL );
	g_return_if_fail (column < XML_TREE_MODEL(model)->n_columns);

	xmlNodePtr	record = iter->user_data;
	GType 		column_type = XML_TREE_MODEL(model)->column_types[column];

	//if(g_value_get_gtype(value) != column_type) {
		g_value_init (value, column_type);
	//}

	switch(column)
	{
		case XML_TREE_MODEL_COL_TYPE:
			g_value_set_int(value, record->type);
			break;
 
		case XML_TREE_MODEL_COL_NS:
			get_value_ns (record, value);
			break;
 
 		case XML_TREE_MODEL_COL_NAME:
		{
			switch(record->type)
			{
				case XML_CDATA_SECTION_NODE:
					g_value_set_string(value,"CDATA");
					break;
				case XML_DOCUMENT_NODE:
				case XML_HTML_DOCUMENT_NODE:
				{
					xmlDocPtr dorec = record;
					g_value_set_string(value,(const gchar *)dorec->version);
					break;
				}
				default:
					g_value_set_string(value,(gchar *) record->name);
					break;
			}
			break;
		}

		case XML_TREE_MODEL_COL_CONTENT:
			get_value_content(record, value);
			break;
			
		case XML_TREE_MODEL_COL_LINE:
			g_value_set_int(value, (gint) xmlGetLineNo(record));
			break;

		case XML_TREE_MODEL_COL_POS:
		{
			xmlParserNodeInfo *nodeinfo;
			gulong pos = NULL;

			nodeinfo = xmlParserFindNodeInfo(XML_TREE_MODEL (model)->parser, record);
			
			if(nodeinfo != NULL)
				pos = nodeinfo->begin_pos;
			
			g_value_set_ulong(value, pos);
			break;
		}
		case XML_TREE_MODEL_COL_XPATH:
			g_value_set_string(value, (const gchar *)xmlGetNodePath(record));
			break;
	}

}

void
get_value_ns(xmlNodePtr record, GValue * value) {

	g_return_if_fail(record != NULL);

	switch(record->type)
	{	
		case XML_ELEMENT_DECL:
		{
			xmlElementPtr element = (xmlElementPtr)record;
			g_value_set_string(value, element->prefix);
			break;
		}
		case XML_ATTRIBUTE_NODE:
		case XML_ELEMENT_NODE:
		{
			if(record->ns)
				if(record->ns->prefix)
					g_value_set_string(value, record->ns->prefix);
			break;
		}
		default:
			break;
	}
}

void
get_value_content(xmlNodePtr record, GValue * value) {

	g_return_if_fail(record != NULL);
	
	switch(record->type)
	{
		case XML_ATTRIBUTE_NODE:
		case XML_TEXT_NODE:
		case XML_COMMENT_NODE:
		case XML_CDATA_SECTION_NODE:
		{
			g_value_set_string(value,g_strstrip((gchar *)xmlNodeGetContent(record)));			
			break;
		}
		case XML_DTD_NODE:
		{
			xmlDtdPtr dtd = (xmlDtdPtr)record;
			g_value_set_string(value,dtd->SystemID);			
			break;
		}
		case XML_DOCUMENT_NODE:
		case XML_ELEMENT_NODE:
		{
			if(record->children) {
				record = record->children;
				while(xmlIsBlankNode(record)==1)
					record = record->next;
					
				g_return_if_fail(record != NULL);
					
				if(record->type == XML_TEXT_NODE)
					g_value_set_string(value,g_strstrip((gchar *)xmlNodeGetContent(record)));
			}
			break;
		}
		case XML_PI_NODE:
		{
		/*
			int n;
			gchar** tokens;
			tokens = g_strsplit_set(g_strstrip((gchar *)xmlNodeGetContent(record)),"=\"\' ",-1);
			gchar * token;
			n = g_strv_length(tokens);
			for(int i = 0; i<n; ++i)
			{
				token = tokens[i];
			}
			g_strfreev(tokens);
		*/
			g_value_set_string(value,g_strstrip((gchar *)xmlNodeGetContent(record)));
			break;
		}
		default:
			break;
	}
}
 
/*****************************************************************************
 *
 *  xml_tree_model_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/
 
static gboolean
xml_tree_model_iter_next (	GtkTreeModel  *model,
							GtkTreeIter   *iter)
{
	xmlNodePtr record;

	g_return_val_if_fail (XML_IS_TREE_MODEL (model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (iter->user_data != NULL, FALSE);

	record = iter->user_data;
	record = xmlNextElementSiblingN(record);

 	//g_return_val_if_fail (record != NULL, FALSE);

	iter->stamp     = XML_TREE_MODEL(model)->stamp;
	iter->user_data = record;
 
	return record != NULL ? TRUE : FALSE;

}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/
 
static gboolean
xml_tree_model_iter_children (	GtkTreeModel *model,
								GtkTreeIter  *iter,
								GtkTreeIter  *parent)
{
	g_return_val_if_fail (parent != NULL, FALSE);
	g_return_val_if_fail (parent->user_data != NULL, FALSE);
	g_return_val_if_fail (XML_IS_TREE_MODEL (model), FALSE);

	if(parent != NULL) {
		//g_return_val_if_fail (xmlFirstElementChildN(parent->user_data) != NULL, FALSE);
		iter->user_data = xmlFirstElementChildN(parent->user_data);		
	} else {
		if(xmlGetRoot(XML_TREE_MODEL(model)))
			iter->user_data = xmlDocGetRootElementN(xmlGetRoot(XML_TREE_MODEL(model)));
	}
	
	if(iter->user_data == NULL)
		return FALSE;
	
	//g_return_val_if_fail (iter->user_data != NULL, FALSE);
	
	iter->stamp = XML_TREE_MODEL(model)->stamp;
 	return TRUE;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *
 *****************************************************************************/
 
static gboolean
xml_tree_model_iter_has_child (	GtkTreeModel *model,
								GtkTreeIter  *iter)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL (model), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	if(xmlChildElementCountN(iter->user_data) == 0)
		return FALSE;
		
	return TRUE;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/
 
static gint
xml_tree_model_iter_n_children (GtkTreeModel *model,
                       		    GtkTreeIter  *iter)
{

	g_return_val_if_fail (XML_IS_TREE_MODEL (model), -1);
	g_return_val_if_fail (iter != NULL, -1);
	g_return_val_if_fail (iter->user_data != NULL, -1);

	gint num_rows = 0;
 
 	/* special case: if iter == NULL, return number of top-level rows */
	if (iter == NULL) {
		num_rows = xmlChildElementCountN(xmlDocGetRootElementN(xmlGetRoot(XML_TREE_MODEL(model))));
	} else {
		num_rows = xmlChildElementCountN(iter->user_data);
	}

	return num_rows;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/
 
static gboolean
xml_tree_model_iter_nth_child (	GtkTreeModel *model,
								GtkTreeIter  *iter,
								GtkTreeIter  *parent,
								gint          n)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL (model), FALSE);
	g_return_val_if_fail (xml_tree_model_iter_n_children(model, parent) >= n, FALSE);
	g_return_val_if_fail (n >= 0, FALSE);

	xmlNodePtr	cursor, record;
	gint		i;
 
	/* special case: if parent == NULL, set iter to n-th top-level row */
	if(parent == NULL) {
		record = xmlGetRoot(XML_TREE_MODEL(model));
	} else {
		record = parent->user_data;
	}
	
	cursor = xmlFirstElementChildN(record);

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
 
  iter->stamp = XML_TREE_MODEL(model)->stamp;
 
  return TRUE;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/
 
static gboolean
xml_tree_model_iter_parent (	GtkTreeModel *model,
								GtkTreeIter  *iter,
								GtkTreeIter  *child)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL (model), FALSE);

	xmlNodePtr node = xmlGetParentNode (child->user_data);

	if(node == NULL)
		return FALSE;

	iter->user_data = node;
	iter->stamp = XML_TREE_MODEL(model)->stamp;
 	return TRUE;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_new: This is what you use in your own code to create a
 *                    	new xml tree model for you to use.
 *
 *****************************************************************************/
 
xmlTreeModel *
xml_tree_model_new (void)
{
	return (xmlTreeModel*) g_object_new (XML_TYPE_TREE_MODEL, NULL);
}

 
xmlTreeModel	*
xml_tree_model_new_from_file ( gchar *filename)
{
	xmlTreeModel *model;
	model = (xmlTreeModel*) g_object_new (XML_TYPE_TREE_MODEL, NULL);
	xml_tree_model_add_file(model, filename);
	return model;
}

gint
compare_node_func (	xmlParserNodeInfoPtr a,
					xmlParserNodeInfoPtr b)
{
	if(a->begin_pos > b->begin_pos)
		return 1;
		
	if(a->begin_pos < b->begin_pos)
		return -1;

	if(a->begin_pos == b->begin_pos)
		return 0;

}

gint
get_offset(xmlDocPtr xmldoc, gchar * filename) {

	gchar	buf[256];
	gsize	buf_size = sizeof buf;
	gssize	size_in;
	gint	offset = 0;
	gint	fd;
	gchar	*standalone;

	if(xmldoc->encoding == "UTF-8" || xmldoc->encoding == NULL)
		return 0;

	fd = g_open(filename, 0x0000, 0);
	g_return_if_fail (fd >= 0);
	
	size_in = read (fd, buf, buf_size);
	
	if(xmldoc->standalone > -2) {
		standalone = g_strrstr(buf, "standalone");
		if(standalone != NULL) {
			offset = buf - standalone;
			close(fd);
			return offset;
		}
	}
	
	while(buf[offset] != '>') {
		++offset;
	}
	
	close(fd);
	
	return offset;
	
}

 
 
/*****************************************************************************
 *
 *  xml_tree_model_add_file:	Adds am XML file to the list.
 *
 *****************************************************************************/

void
xml_tree_model_add_file (	xmlTreeModel	*model,
							gchar			*filename)
{
	g_return_if_fail (XML_IS_TREE_MODEL(model));
	g_return_if_fail (filename != NULL);

	GtkTreeIter   		iter;
	xsltStylesheetPtr 	xsldoc;
	xmlDocPtr			xmldoc = NULL;
	xmlNodePtr			cur;

    model->parser = xmlCreateURLParserCtxt (filename,
											XML_PARSE_RECOVER);// | 
											//XML_PARSE_DTDVALID);

	g_return_if_fail(model->parser != NULL);
            
    int record_info	= 1;
	xmlSetFeature(model->parser, "gather line info", &record_info);	

	xmlParseDocument(model->parser);

	glong bytes = xmlByteConsumed(model->parser);
		
 	model->valid = (model->parser->valid == 0) ? FALSE : TRUE;

	xmldoc = model->parser->myDoc;

	g_return_if_fail (xmldoc != NULL);

	model->offset = get_offset(xmldoc, filename);

	gint i;
	xmlParserNodeInfoPtr node;

	for(i = 0; i < model->parser->node_seq.length; ++i) {
		node = &model->parser->node_seq.buffer[i];
		model->nodeinfo = g_list_append(model->nodeinfo, node);
	}
	
	model->nodeinfo = g_list_sort(model->nodeinfo, compare_node_func);
	
	cur = xmlDocGetRootElement(xmldoc);

    if ((IS_XSLT_ELEM(cur)) &&
		((IS_XSLT_NAME(cur, "stylesheet")) ||
		(IS_XSLT_NAME(cur, "transform")))) {
		xsldoc = xsltParseStylesheetDoc(xmldoc);
	} else {
		xsldoc = NULL;
	}

	if(model->xsldoc != NULL) {
		xsltFreeStylesheet(model->xsldoc);
		model->xmldoc = NULL;
	}
	
	if(model->xmldoc)
		xmlFreeDoc(model->xmldoc);

	model->xmldoc = xmldoc;
	model->xsldoc = xsldoc;
	model->filename = filename;
	model->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	gtk_tree_model_row_changed(GTK_TREE_MODEL(model),gtk_tree_path_new_first(), &iter);
	
	g_signal_emit(model, xml_tree_model_signals[XML_TREE_MODEL_CHANGED],0,NULL);
	
    //xmlFreeParserCtxt(model->parser);
}

void
xml_tree_model_reload (xmlTreeModel *xmltreemodel)
{
	g_return_if_fail (XML_IS_TREE_MODEL(xmltreemodel));
	xml_tree_model_add_file(xmltreemodel, xmltreemodel->filename);
}

void
xml_tree_model_add_xmldoc(xmlTreeModel * model, xmlDocPtr xmldoc) {
	g_return_if_fail (XML_IS_TREE_MODEL(model));
	g_return_if_fail (xmldoc != NULL);

	GtkTreeIter   iter;
	model->xmldoc = xmldoc;
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	gtk_tree_model_row_changed(GTK_TREE_MODEL(model),gtk_tree_path_new_first(), &iter);
	g_signal_emit(model, xml_tree_model_signals[XML_TREE_MODEL_CHANGED],0,NULL);
}

gint
xml_tree_model_write_to_file(xmlTreeModel * model, gint file, gint format) {
	g_return_val_if_fail (XML_IS_TREE_MODEL(model), -1);
	g_return_val_if_fail (model->xmldoc != NULL, -1);

	xmlChar *xmlbuff;
    gint buffersize;
	gint byteswritten;
	
	xmlDocDumpFormatMemory(model->xmldoc, &xmlbuff, &buffersize, format);
	
	byteswritten = write(file, xmlbuff, buffersize);
	
    xmlFree(xmlbuff);

	return byteswritten;
}

gint
xml_tree_model_save_xsl (xmlTreeModel *model, gchar *filename)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL(model), -1);
	g_return_val_if_fail (model->xmldoc != NULL, -1);
	
	return xsltSaveResultToFilename(filename, model->xmldoc, model->xsldoc, 0);
	
}


GtkListStore *
xml_tree_model_get_xpath_results(xmlTreeModel *xmltreemodel, gchar *xpath)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL(xmltreemodel), NULL);
	g_return_val_if_fail (xpath != NULL, NULL);
	g_return_val_if_fail (xmltreemodel->xmldoc != NULL, NULL);

	GtkListStore		*list_store;
	GtkTreeIter			itern, iterx;
	gint				i, size, column;
	xmlXPathObjectPtr	xpath_results;

	list_store = gtk_list_store_newv (xmltreemodel->n_columns,xmltreemodel->column_types);

	xpath_results = evaluate_xpath(xmltreemodel->xmldoc, xpath);

	size = (xpath_results) ? xpath_results->nodesetval->nodeNr : 0;

	for(i = 0; i < size; ++i) {
		xmlNodePtr record;
		GValue value = G_VALUE_INIT;
		record = xpath_results->nodesetval->nodeTab[i];
		iterx.user_data = record;
		gtk_list_store_append (list_store, &itern);
		for(column = 0; column < xmltreemodel->n_columns; ++column) {
			xml_tree_model_get_value(GTK_TREE_MODEL(xmltreemodel), &iterx, column, &value);
			gtk_list_store_set_value (	list_store,
										&itern,
										column,
										&value
										);
			g_value_unset(&value);
		}
	}
	xmlXPathFreeObject(xpath_results);
	return list_store;
}

gboolean
xml_tree_model_validate(xmlTreeModel *model)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL(model), FALSE);
    return model->valid;
}

GtkListStore *
xml_tree_model_get_stylesheet_params(xmlTreeModel *xmltreemodel)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL(xmltreemodel), NULL);

	GtkListStore *list_store;
	GtkTreeIter iter;
	xsltStackElemPtr stack;

	list_store = gtk_list_store_new (2,G_TYPE_STRING, G_TYPE_STRING);

	if(xmltreemodel->xsldoc != NULL) {
		stack = xmltreemodel->xsldoc->variables; 
		while(stack != NULL) {
			if(stack->comp->type == XSLT_FUNC_PARAM) {
				gtk_list_store_append (list_store, &iter);
				gtk_list_store_set(GTK_LIST_STORE(list_store), &iter,
									0, stack->name,
									1, "",
									-1);
			}
			stack = stack->next;
		}
	}
	return list_store;
}

const gchar * xsl_params(GHashTable * params) {
	if(params == NULL)
		return NULL;

	GHashTableIter	iter;
	gint 			hashtablesize = g_hash_table_size(params);
	gint			arraysize = (hashtablesize * 2) + 1;
	gint			i = 0;
	const gchar		* params_array[arraysize];
	gchar			* key, * value;

	g_hash_table_iter_init (&iter, params);
	
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		params_array[i] = key;
		params_array[++i] = value;
		++i;
	}
	
	params_array[i] = NULL;
	
	return params_array;
}

/* Transform files */
xmlTreeModel *
xml_tree_model_transform (	xmlTreeModel * xml,
							xmlTreeModel * xslt,
							GHashTable * params)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL(xml), NULL);
	g_return_val_if_fail (XML_IS_TREE_MODEL(xslt), NULL);

	xmlDocPtr 		xslResult;
	xmlTreeModel	* treemodel;

	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;

	xslResult = xsltApplyStylesheet(xslt->xsldoc, xml->xmldoc, xsl_params(params));

	g_return_val_if_fail (xslResult != NULL, NULL);

	treemodel = xml_tree_model_new();
	xml_tree_model_add_xmldoc(treemodel, xslResult);
	treemodel->xsldoc = xslt->xsldoc;
	return treemodel;
}


gboolean
xml_tree_model_is_stylesheet (xmlTreeModel *model)
{
	g_return_val_if_fail(model != NULL, FALSE);
 	return (model->xsldoc != NULL) ? TRUE : FALSE;
}



xmlNodePtr
xml_tree_model_get_node_from_position (xmlTreeModel *model,
										gint position)
{
	xmlParserNodeInfoPtr nodeinfo, nodeswap; 
	gint		column;
	
	column = position + model->offset;
	
	/* Do a binary search for the key */
    gint lower = 1;
    gint upper = g_list_length(model->nodeinfo);
    gint middle = 0;
    gint found = 0;
    
    if(column < 0)
		column = 0;
    
    while (lower <= upper && !found) {
        middle = lower + (upper - lower) / 2;
        nodeinfo = g_list_nth_data(model->nodeinfo, middle - 1);
        
        if ((column >= nodeinfo->begin_pos)) {
            nodeswap = g_list_nth_data(model->nodeinfo, middle);
            if(nodeswap != NULL) {
				// We've reached the end of the tree and only have close tags...
				if(column < nodeswap->begin_pos) {
					found = 1;
				} else {
					lower = middle + 1;				
				}
			} else {
				//nodeinfo = xmlParserFindNodeInfo(model->parser, nodeinfo->node);
				found = 1;
			}
        } else if (column < nodeinfo->begin_pos) {
            upper = middle - 1;
        } else {
            lower = middle + 1;
        }
    }
	
	g_return_val_if_fail(nodeinfo->node != NULL, NULL);	

	return nodeinfo->node;
}

gchar *
xml_tree_model_get_xpath_from_position (xmlTreeModel *model,
										gint position)
{
	xmlNodePtr node;
	node = xml_tree_model_get_node_from_position(model, position);
	g_return_val_if_fail(node != NULL, NULL);
	return (gchar *)xmlGetNodePath(node);
}

GtkTreePath *
xml_tree_model_get_path_from_position(xmlTreeModel *model, gint position){
	xmlNodePtr node;
	GtkTreeIter	iter;
	GtkTreePath	*treepath;

	g_return_val_if_fail(model != NULL, NULL);

	node = xml_tree_model_get_node_from_position(model, position);

	iter.stamp = model->stamp;
	iter.user_data = node;
	treepath = xml_tree_model_get_path(model, &iter);

	return treepath;
}

GtkTreePath *
xml_tree_model_get_path_from_xpath (xmlTreeModel *model, gchar *xpath)
{

	g_return_val_if_fail(model != NULL, NULL);
	g_return_val_if_fail(model->xmldoc != NULL, NULL);
	g_return_val_if_fail(xpath != NULL, NULL);

	GtkTreeIter			iter;
	GtkTreePath			*treepath;
	xmlXPathObjectPtr	xpath_results;
	gint				size;

	xpath_results = evaluate_xpath(model->xmldoc, xpath);

	g_return_val_if_fail(xpath_results != NULL, NULL);

	size = (xpath_results->nodesetval) ? xpath_results->nodesetval->nodeNr : 0;

	if(size > 0) {
		iter.user_data = xpath_results->nodesetval->nodeTab[0];
		iter.stamp = model->stamp;
		treepath = xml_tree_model_get_path(model, &iter);
	}
	xmlXPathFreeObject(xpath_results);
	
	return treepath;
}
