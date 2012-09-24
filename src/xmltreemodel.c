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
 
static void         xml_tree_model_init            (xmlTreeModel      *pkg_tree);
 
static void         xml_tree_model_class_init      (xmlTreeModelClass *klass);
 
static void         xml_tree_model_tree_model_init (GtkTreeModelIface *iface);
 
static void         xml_tree_model_finalize        (GObject           *object);
 
static GtkTreeModelFlags xml_tree_model_get_flags  (GtkTreeModel      *tree_model);
 
static gint         xml_tree_model_get_n_columns   (GtkTreeModel      *tree_model);
 
static GType        xml_tree_model_get_column_type (GtkTreeModel      *tree_model,
                                                 gint               index);
 
static gboolean     xml_tree_model_get_iter        (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreePath       *path);
 
static GtkTreePath *xml_tree_model_get_path        (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);
 
static void         xml_tree_model_get_value       (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 gint               column,
                                                 GValue            *value);
 
static gboolean     xml_tree_model_iter_next       (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);
 
static gboolean     xml_tree_model_iter_children   (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *parent);
 
static gboolean     xml_tree_model_iter_has_child  (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);
 
static gint         xml_tree_model_iter_n_children (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter);
 
static gboolean     xml_tree_model_iter_nth_child  (GtkTreeModel      *tree_model,
                                                 GtkTreeIter		*iter,
                                                 GtkTreeIter		*parent,
                                                 gint               n);
 
static gboolean     xml_tree_model_iter_parent     (GtkTreeModel      *tree_model,
                                                 GtkTreeIter       *iter,
                                                 GtkTreeIter       *child);
 
static void throw_xml_error(xmlTreeModel * xml_tree_model, xmlErrorPtr error);
												 

static xmlNodePtr xmlGetParentNode(xmlNodePtr child);
static xmlNodePtr xmlDocGetRootElementN(xmlNodePtr node);
static xmlNodePtr xmlNextElementSiblingN(xmlNodePtr node);
static xmlNodePtr xmlFirstElementChildN(xmlNodePtr node);
static unsigned long xmlChildElementCountN(xmlNodePtr node);
static xmlNodePtr xmlPreviousElementSiblingN(xmlNodePtr node);

void get_value_content(xmlNodePtr record, GValue * value);
void get_value_ns(xmlNodePtr record, GValue * value);

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

		xmlNsPtr *nsp;
		nsp = xmlGetNsList(doc,xmlDocGetRootElement(doc));

		int i = 0;
		
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
		if(xpathObj == NULL) {
			return NULL;
		}
		return xpathObj;
	}
	return NULL;
}

/* XML Error Handling */ 
static void throw_xml_error(xmlTreeModel * xml_tree_model, xmlErrorPtr error){
	//g_log(XML_TREE_MESSAGE, G_LOG_LEVEL_WARNING, "XML Error: %s", error->message);

	g_signal_emit(xml_tree_model, xml_tree_model_signals[XML_TREE_MODEL_ERROR],0,error);

}


static void throw_xsl_error(xmlTreeModel * xml_tree_model, const char * fmt,...)
{
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

	g_signal_emit(xml_tree_model, xml_tree_model_signals[XML_TREE_MODEL_XSL_ERROR],0,&error);
    
}


static xmlNodePtr xmlGetRoot(xmlTreeModel *xml_tree_model) {
	return (xmlNodePtr)xml_tree_model->xmldoc;
}

static xmlNodePtr xmlGetParentNode(xmlNodePtr child) {
	if(child != NULL)
		return child->parent;
		
	return NULL;
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

		while(xmlIsBlankNode(record)==1) {
			record = record->prev;
		}
				
		if(record == NULL) {
			record = xmlGetParentNode(node);
			record = record->properties;
			if(record != NULL) {
				while(record->next != NULL) {
					record = record->next;
				}
			}
		}
		
		break;
		
	case XML_TEXT_NODE:
		while(xmlIsBlankNode(record)==1) {
			record = record->prev;
		}
		
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
xml_tree_model_init (xmlTreeModel *xml_tree_model)
{

	LIBXML_TEST_VERSION /* Arrrgh - where should this go??? */
	
	xmlSetStructuredErrorFunc(xml_tree_model, throw_xml_error);
	xsltSetGenericErrorFunc(xml_tree_model, throw_xsl_error);
	
	xml_tree_model->n_columns       = XML_TREE_MODEL_N_COLUMNS;
	xml_tree_model->column_types[0] = G_TYPE_INT;	/* XML_TREE_MODEL_COL_TYPE	*/
	xml_tree_model->column_types[1] = G_TYPE_STRING;	/* XML_TREE_MODEL_COL_NS	*/
	xml_tree_model->column_types[2] = G_TYPE_STRING;	/* XML_TREE_MODEL_COL_NAME	*/
	xml_tree_model->column_types[3] = G_TYPE_STRING;	/* XML_TREE_MODEL_COL_CONTENT	*/
	xml_tree_model->column_types[4] = G_TYPE_INT;		/* XML_TREE_MODEL_COL_LINE	*/
	xml_tree_model->column_types[5] = G_TYPE_STRING;	/* XML_TREE_MODEL_COL_PATH	*/
	g_assert (XML_TREE_MODEL_N_COLUMNS == 6);
 
	xml_tree_model->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */
	xml_tree_model->xmldoc = NULL;
	xml_tree_model->xsldoc = NULL;
	xml_tree_model->xpath = NULL;
	xml_tree_model->valid = FALSE;
	
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
	xmlTreeModel *xml_tree_model = XML_TREE_MODEL(object);

	if(xml_tree_model->xsldoc) {
		xsltFreeStylesheet(xml_tree_model->xsldoc);
		xml_tree_model->xmldoc = NULL;
	}
	
	if(xml_tree_model->xmldoc)
		xmlFreeDoc(xml_tree_model->xmldoc);
	
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
xml_tree_model_get_flags (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (XML_IS_TREE_MODEL(tree_model), (GtkTreeModelFlags)0);
 
  return ((GtkTreeModelFlags)0);
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/
 
static gint
xml_tree_model_get_n_columns (GtkTreeModel *tree_model)
{
  g_return_val_if_fail (XML_IS_TREE_MODEL(tree_model), 0);
 
  return XML_TREE_MODEL(tree_model)->n_columns;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/
 
static GType
xml_tree_model_get_column_type (GtkTreeModel *tree_model,
                             gint          index)
{
  g_return_val_if_fail (XML_IS_TREE_MODEL(tree_model), G_TYPE_INVALID);
  g_return_val_if_fail (index < XML_TREE_MODEL(tree_model)->n_columns && index >= 0, G_TYPE_INVALID);
 
  return XML_TREE_MODEL(tree_model)->column_types[index];
}


/*****************************************************************************
 *
 *  xml_tree_model_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure 
 *
 *****************************************************************************/

static gboolean
xml_tree_model_get_iter (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter,
                      GtkTreePath  *path)
{
	xmlTreeModel		*xml_tree_model;
 
	g_assert(XML_IS_TREE_MODEL(tree_model));
	g_assert(path!=NULL);
 
	xml_tree_model = XML_TREE_MODEL(tree_model);
 
	gint *indices = gtk_tree_path_get_indices(path);
	gint depth   = gtk_tree_path_get_depth(path);

	xmlNodePtr tree = xmlGetRoot(xml_tree_model);
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
	iter->stamp = xml_tree_model->stamp; 
	
	return TRUE; 
}

/*****************************************************************************
 *
 *  xml_tree_model_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/
 
static GtkTreePath *
xml_tree_model_get_path (GtkTreeModel *tree_model,
                      GtkTreeIter  *iter)
{
	g_return_val_if_fail (XML_IS_TREE_MODEL(tree_model), NULL); 
	g_return_val_if_fail (iter != NULL, NULL); 

	xmlTreeModel *xmltreemodel = XML_TREE_MODEL(tree_model); 

	xmlNodePtr node = iter->user_data;
	xmlNodePtr tree = xmlDocGetRootElementN(xmlGetRoot(xmltreemodel));
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
	
	gchar *gpath = gtk_tree_path_to_string(path);
	
	return path; 
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/
 
static void
xml_tree_model_get_value (GtkTreeModel *tree_model,
                       GtkTreeIter  *iter,
                       gint          column,
                       GValue       *value)
{
	xmlNodePtr	record;
 
	g_return_if_fail (XML_IS_TREE_MODEL (tree_model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (column < XML_TREE_MODEL(tree_model)->n_columns);

	g_value_init (value, XML_TREE_MODEL(tree_model)->column_types[column]);
 
	record = iter->user_data;

	g_return_if_fail ( record != NULL );

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

		case XML_TREE_MODEL_COL_XPATH:
			g_value_set_string(value, (const gchar *)xmlGetNodePath(record));
			break;
	}

}

void
get_value_ns(xmlNodePtr record, GValue * value) {

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
				while(xmlIsBlankNode(record)==1) {
					record = record->next;
				}
				if(record->type == XML_TEXT_NODE)
					g_value_set_string(value,g_strstrip((gchar *)xmlNodeGetContent(record)));
			}
			break;
		}
		case XML_PI_NODE:
		{
/*			int n;
			gchar** tokens;
			tokens = g_strsplit_set(g_strstrip((gchar *)xmlNodeGetContent(record)),"=\"\' ",-1);
			gchar * token;
			n = g_strv_length(tokens);
			for(int i = 0; i<n; ++i)
			{
				token = tokens[i];
			}
			g_strfreev(tokens);*/

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
xml_tree_model_iter_next (GtkTreeModel  *tree_model,
                       GtkTreeIter   *iter)
{
	xmlNodePtr record, nextrecord;
	xmlTreeModel *xml_tree_model;

	g_return_val_if_fail (XML_IS_TREE_MODEL (tree_model), FALSE);
 
	if (iter == NULL || iter->user_data == NULL)
		return FALSE;
 
	xml_tree_model = XML_TREE_MODEL(tree_model);
 
	record = iter->user_data;
 
	/* Is this the last node in the list? */
	if(xmlNextElementSiblingN(record) == NULL)
		return FALSE;
 
	nextrecord = xmlNextElementSiblingN(record);
 
	g_assert ( nextrecord != NULL );

	iter->stamp     = xml_tree_model->stamp;
	iter->user_data = nextrecord;
 
	return TRUE;
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
xml_tree_model_iter_children (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter,
                           GtkTreeIter  *parent)
{
	xmlTreeModel  	*xml_tree_model;
	xmlNodePtr		record;
	
	g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);

	g_return_val_if_fail (XML_IS_TREE_MODEL (tree_model), FALSE);
	xml_tree_model = XML_TREE_MODEL(tree_model);

	if(parent != NULL) {
		record = parent->user_data;
		if(xmlFirstElementChildN(record) == NULL) {
			return FALSE;
		} else {
			iter->user_data = xmlFirstElementChildN(record);		
		}
	} else {
		if(xmlGetRoot(xml_tree_model)) {
			iter->user_data = xmlDocGetRootElementN(xmlGetRoot(xml_tree_model));
		}
	}
	iter->stamp = xml_tree_model->stamp;
 	return TRUE;
}
 
 
/*****************************************************************************
 *
 *  xml_tree_model_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *
 *****************************************************************************/
 
static gboolean
xml_tree_model_iter_has_child (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
	
	if(xmlChildElementCountN(iter->user_data) > 0)
		return TRUE;
		
	return FALSE;
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
xml_tree_model_iter_n_children (GtkTreeModel *tree_model,
                       		    GtkTreeIter  *iter)
{
	xmlTreeModel  *xml_tree_model;
	gint num_rows = 0;

	g_return_val_if_fail (XML_IS_TREE_MODEL (tree_model), -1);
	g_return_val_if_fail (iter == NULL || iter->user_data != NULL, FALSE);
 
	xml_tree_model = XML_TREE_MODEL(tree_model);
 
	/* special case: if iter == NULL, return number of top-level rows */
	if (iter == NULL) {
		num_rows = xmlChildElementCountN(xmlDocGetRootElementN(xmlGetRoot(xml_tree_model)));
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
xml_tree_model_iter_nth_child (GtkTreeModel *tree_model,
							GtkTreeIter  *iter,
                            GtkTreeIter  *parent,
							gint          n)
{
	xmlNodePtr		cursor, record;
	xmlTreeModel		*xml_tree_model;

	g_return_val_if_fail (XML_IS_TREE_MODEL (tree_model), FALSE);
 
	xml_tree_model = XML_TREE_MODEL(tree_model);
 
	/* special case: if parent == NULL, set iter to n-th top-level row */


	if(parent == NULL) {
		record = xmlGetRoot(xml_tree_model);
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
 
  iter->stamp = xml_tree_model->stamp;
 
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
xml_tree_model_iter_parent (GtkTreeModel *tree_model,
                         GtkTreeIter  *iter,
                         GtkTreeIter  *child)
{
	xmlTreeModel  	*xml_tree_model;
	xmlNodePtr		record;
 
	g_return_val_if_fail (child == NULL || child->user_data != NULL, FALSE);

	g_return_val_if_fail (XML_IS_TREE_MODEL (tree_model), FALSE);
	xml_tree_model = XML_TREE_MODEL(tree_model);

	if(child != NULL) {
		record = child->user_data;
		if(xmlGetParentNode (record) == NULL) {
			return FALSE;
		} else {
			iter->user_data = xmlGetParentNode (record);		
		}
	} else {
		if(xmlGetRoot(xml_tree_model)) {
			iter->user_data = xmlDocGetRootElementN(xmlGetRoot(xml_tree_model));
		}
	}
	iter->stamp = xml_tree_model->stamp;
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
	xmlTreeModel *newxmltreemodel;
 
	newxmltreemodel = (xmlTreeModel*) g_object_new (XML_TYPE_TREE_MODEL, NULL);
 
	g_assert( newxmltreemodel != NULL );
 
	return newxmltreemodel;
}
 
/*****************************************************************************
 *
 *  xml_tree_model_add_file:	Adds am XML file to the list.
 *
 *****************************************************************************/
 
void
xml_tree_model_add_file (	xmlTreeModel	*xml_tree_model,
							gchar			*filename)
{
	g_return_if_fail (XML_IS_TREE_MODEL(xml_tree_model));

	GtkTreeIter   		iter;
	xsltStylesheetPtr 	xsldoc;
	xmlDocPtr			xmldoc = NULL;
	xmlNodePtr			cur;
    xmlParserCtxtPtr	ctxt; /* the parser context */

    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL)
		return FALSE;
    
    /* parse the file, activating the DTD validation option */
    xmldoc = xmlCtxtReadFile (ctxt, filename, NULL,	XML_PARSE_RECOVER | 
													XML_PARSE_DTDVALID );
    
    if (ctxt->valid == 0) {
		xml_tree_model->valid = FALSE;
	} else {
		xml_tree_model->valid = TRUE;
	}
	
	if(xmldoc == NULL) 
		return;
	
	cur = xmlDocGetRootElement(xmldoc);

    if ((IS_XSLT_ELEM(cur)) &&
		((IS_XSLT_NAME(cur, "stylesheet")) ||
		(IS_XSLT_NAME(cur, "transform")))) {
		xsldoc = xsltParseStylesheetDoc(xmldoc);
	} else {
		xsldoc = NULL;
	}

	if(xmldoc != NULL) {		
		if(xml_tree_model->xsldoc != NULL) {
			xsltFreeStylesheet(xml_tree_model->xsldoc);
			xml_tree_model->xmldoc = NULL;
		}
		
		if(xml_tree_model->xmldoc)
			xmlFreeDoc(xml_tree_model->xmldoc);

		xml_tree_model->xmldoc = xmldoc;
		xml_tree_model->xsldoc = xsldoc;

	} else {
		//g_log(XML_TREE_MESSAGE, G_LOG_LEVEL_WARNING, "Failed to load %s\n", filename);
		return;
	}
	
	xml_tree_model->filename = filename;
	
	xml_tree_model->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(xml_tree_model), &iter);
	gtk_tree_model_row_changed(GTK_TREE_MODEL(xml_tree_model),gtk_tree_path_new_first(), &iter);
	
	g_signal_emit(xml_tree_model, xml_tree_model_signals[XML_TREE_MODEL_CHANGED],0,NULL);
	
    xmlFreeParserCtxt(ctxt);

}

void
xml_tree_model_reload (xmlTreeModel *xmltreemodel)
{
	g_return_if_fail (XML_IS_TREE_MODEL(xmltreemodel));
	xml_tree_model_add_file(xmltreemodel, xmltreemodel->filename);
}

void
xml_tree_model_add_xmldoc(xmlTreeModel * xml_tree_model, xmlDocPtr xmldoc) {
	GtkTreeIter   iter;

	xml_tree_model->xmldoc = xmldoc;
	xml_tree_model->stamp = g_random_int();  /* Random int to check whether an iter belongs to our model */

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(xml_tree_model), &iter);
	gtk_tree_model_row_changed(GTK_TREE_MODEL(xml_tree_model),gtk_tree_path_new_first(), &iter);

	g_signal_emit(xml_tree_model, xml_tree_model_signals[XML_TREE_MODEL_CHANGED],0,NULL);
}

gint
xml_tree_model_write_to_file(xmlTreeModel * ttt, gint file, gint format) {
	xmlChar *xmlbuff;
    gint buffersize;
	gint byteswritten;
	
	if(ttt->xmldoc == NULL)
		return -1;

	xmlDocDumpFormatMemory(ttt->xmldoc, &xmlbuff, &buffersize, format);
	
	byteswritten = write(file, xmlbuff, buffersize);
	
    xmlFree(xmlbuff);

	return byteswritten;
}

GtkListStore *
xml_tree_model_get_xpath_results(xmlTreeModel *xmltreemodel, gchar *xpath)
{
	GtkListStore *list_store;
	GtkTreeIter itern, iterx;
	gint i, size, column;

	list_store = NULL;

	if(xmltreemodel->xmldoc != NULL) {
		xmlXPathObjectPtr xpath_results;

		xpath_results = evaluate_xpath(xmltreemodel->xmldoc, xpath);
		
		if(xpath_results != NULL) {

			list_store = gtk_list_store_newv (xmltreemodel->n_columns,xmltreemodel->column_types);

			size = (xpath_results->nodesetval) ? xpath_results->nodesetval->nodeNr : 0;

			//g_log(XML_TREE_MESSAGE, G_LOG_LEVEL_MESSAGE, "XPath returned %i nodes\n", size);

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
		} else {
			//g_log(XML_TREE_MESSAGE, G_LOG_LEVEL_WARNING, "XPath returned no results\n");
		}
	}
	return list_store;
}

gboolean
xml_tree_model_validate(xmlTreeModel *tree_model) {

    return tree_model->valid;
}

GtkListStore *
xml_tree_model_get_stylesheet_params(xmlTreeModel *xmltreemodel) {

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


/* Transform files */
xmlTreeModel *
xml_tree_model_transform (	xmlTreeModel * xml,
							xmlTreeModel * xslt,
							GHashTable * params)
{
	xmlDocPtr 		xslResult;
	xmlTreeModel	* treemodel;
	gint 			hashtablesize = g_hash_table_size(params);
	gint			arraysize = (hashtablesize * 2) + 1;
	const gchar		* params_array[arraysize];

	GHashTableIter iter;
	gchar * key, * value;
	gint i = 0;
	g_hash_table_iter_init (&iter, params);
	
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		params_array[i] = key;
		params_array[++i] = value;
		++i;
	}
	
	params_array[i] = NULL;

	xslResult = xsltApplyStylesheet(xslt->xsldoc, xml->xmldoc, params_array);

	if(!xslResult)
	{
		return NULL;
	}

	treemodel = xml_tree_model_new();
	xml_tree_model_add_xmldoc(treemodel, xslResult);
	
	return treemodel;
}


gboolean
xml_tree_model_is_stylesheet (xmlTreeModel *ttt)
{
	if(ttt->xsldoc != NULL) {
		return TRUE;
	} else {
		return FALSE;
	}
}

GtkTreePath *
xml_tree_model_get_path_from_xpath (xmlTreeModel *ttt, gchar *xpath)
{
	GtkTreeIter iter;
	GtkTreePath * treepath = NULL;
	xmlXPathObjectPtr xpath_results;
	gint size;
	
	g_return_val_if_fail(ttt->xmldoc != NULL, treepath);

	xpath_results = evaluate_xpath(ttt->xmldoc, xpath);

	g_return_val_if_fail(xpath_results != NULL, treepath);

	size = (xpath_results->nodesetval) ? xpath_results->nodesetval->nodeNr : 0;
	if(size > 0) {
		iter.user_data = xpath_results->nodesetval->nodeTab[0];
		iter.stamp = ttt->stamp;
		treepath = xml_tree_model_get_path(ttt, &iter);
	}
	xmlXPathFreeObject(xpath_results);
	
	return treepath;
}
