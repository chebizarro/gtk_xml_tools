from types import *
from xml.dom import Node
from xml.dom.minidom import parse, parseString, Attr 

import pygtk
pygtk.require("2.0")

import gtk

def _is_blank_node(node) :
	value = None
	if node :
		if node.nodeType == Node.TEXT_NODE:
			if node.data :
				value = node.data.strip().strip("\n").strip("\p")
			if len(value) < 1 :
				return True
	return False

class XMLTreeModelAttr(Attr) :
	def __init__(self, node, parent, index, siblings = None) :
		Attr.__init__(self, node.name,
			namespaceURI=node.namespaceURI, localName=node.localName, prefix=node.prefix)
		self._set_value(node.value)
		self.ownerDocument = node.ownerDocument
		self.ownerElement = node.ownerElement
		self.parentNode = parent
		self.previousSibling = None
		self.nextSibling = None
		
		if siblings :
			self.previousSibling = siblings[0]
			self.nextSibling = siblings[1]

		if not self.previousSibling :
			if(index > 0) :
				idx = index - 1
				self.previousSibling = XMLTreeModelAttr(parent.attributes.item(idx),
					parent, idx, (None, self) )

		if not self.nextSibling :
			if(index < parent.attributes.length-1) :
				idx = index + 1
				self.nextSibling = XMLTreeModelAttr(parent.attributes.item(idx),
					parent, idx, (self, None))
			elif parent.hasChildNodes() :
				node = parent.firstChild
				while _is_blank_node(node) :
					node = node.nextSibling
				self.nextSibling = node
		
	def hasChildNodes(self) :
		return False
		


class XMLTreeModel(gtk.GenericTreeModel):

	column_types = (
		IntType,
		StringType,
		StringType,
		StringType
	)
	xml = None
	
	def __init__(self, xml_file = None, update_xml = False):
		gtk.GenericTreeModel.__init__(self)
		self.set(xml_file, update_xml)
		return
	"""
	Public set function
	"""
	def set(self, xml_file = None, update_xml = False) :
		if self.xml :
			self.xml.unlink()
			self.invalidate_iters()
			self.xml = None
		if type(xml_file) == Node:
			self.xml = xml_file
		elif type(xml_file) == str:
			try:
				self.xml = parse(xml_file)
			except:
				try:
					self.xml = parseString(xml_file)
				except:
					print("Invalid file " + xml_file)
					self.xml = None
		if self.xml :
			self.xml = self.xml.documentElement
			path = (0,)
			self.row_deleted(path)
			self.row_inserted(path, self.get_iter(path))
			self.row_has_child_toggled(path, self.get_iter(path))

	""" GenericTreeModel Interface handlers """
	"""
	The on_get_flags() method should return a value that is a combination of:
	gtk.TREE_MODEL_ITERS_PERSIST - TreeIters survive all signals emitted by the tree.
	gtk.TREE_MODEL_LIST_ONLY - The model is a list only, and never has children
	"""
	def on_get_flags(self) :
		""" our iterators contain a reference to the actual Node in the DOM so they persist """
		return gtk.TREE_MODEL_ITERS_PERSIST
	"""
	The on_get_n_columns() method should return the number of columns that your model exports
	to the application.		
	"""
	def on_get_n_columns(self) :
		return len(self.column_types)
	"""
	The on_get_column_type() method should return the type of the column with the specified index.
	This method is usually called from a TreeView when its model is set.
	"""
	def on_get_column_type(self, index) :
		if index >= len(self.column_types) or index < 0:
			return None
		return self.column_types[index]
	"""
	The on_get_iter() method should return an rowref for the tree path specified by path.
	The tree path will always be represented using a tuple. 
	"""
	def on_get_iter(self, path) :
		if not self.xml : return None
		tree = self.xml
		result = self.xml
		for pos in path :
			result = tree
			for num in range(0,pos) :
				tree = self._next_element_sibling(tree)
			result = tree
			tree = self._first_element_child(tree)
		if result :
			return self.create_tree_iter(result)
		return None
	"""
	The on_get_path() method should return a tree path given a valid rowref
	"""
	def on_get_path(self, iter) :
		if not iter :
			return None
		if not self.iter_is_valid(iter) :
			return (0,)
		iter_data = self.get_user_data(iter)
		tree = self.xml
		path = ()
		idx = 0
		while not tree.isSameNode(iter_data) and iter_data :
			prev = self._previous_element_sibling(iter_data)
			if not prev :
				path = (idx,) + path
				iter_data = iter_data.parentNode
				idx = 0
			else :
				iter_data = prev
				idx += 1
		path = (0,) + path
		return path
	"""
	The on_get_value() method should return the data stored at the row and column specified by iter and column. 
	XML_TREE_MODEL_COL_TYPE = 0,
	XML_TREE_MODEL_COL_NS,
	XML_TREE_MODEL_COL_NAME,
	XML_TREE_MODEL_COL_CONTENT,
	"""	
	def on_get_value(self, iter, column) :
		if not iter :
			return None
		if not self.iter_is_valid(iter) :
			return None
		iter_data = self.get_user_data(iter)
		if not iter_data:
			return None
		if column == 0:
			return iter_data.nodeType
		elif column == 1:
			return iter_data.prefix
		elif column == 2:
			if iter_data.nodeType == Node.CDATA_SECTION_NODE :
				return "CDATA"
			if iter_data.nodeType == Node.DOCUMENT_NODE :
				return "0"
			if iter_data.nodeType == Node.TEXT_NODE :
				return "TEXT"
			if iter_data.nodeType == Node.ATTRIBUTE_NODE :
				return iter_data.name
			else :
				return iter_data.tagName
		elif column == 3:
			return self._get_node_content(iter_data)
	"""
	The on_iter_next() method should return a TreeIter to the row (at the same level) after the row specified by iter
	or None if there is no next row
	"""
	def on_iter_next(self, iter) :
		if self.iter_is_valid(iter) :
			iter_data = self.get_user_data(iter)
			record = self._next_element_sibling(iter_data)
			if record :
				return self.create_tree_iter(record)
		return None
	"""
	The on_iter_children() method should return a row reference to the first child row
	of the row specified by parent. If parent is None, a reference to the first top level
	row is returned. If there is no child row None is returned.
	"""
	def on_iter_children(self, parent) :
		child = self.xml
		if self.iter_is_valid(parent) :
			iter_data = self.get_user_data(parent)
			child = self._first_element_child(iter_data)
			if not child :
				return None
		return self.create_tree_iter(child)
	"""
	The on_iter_has_child() method should return TRUE if the row specified by iter
	has child rows; FALSE otherwise. Our example returns FALSE since no row can have a child:
	"""
	def on_iter_has_child(self, iter) :
		if iter :
			if self.iter_is_valid(iter) :
				iter_data = self.get_user_data(iter)
				return self._has_child_nodes(iter_data)
		return False
	"""
	The on_iter_n_children() method should return the number of child rows that the row specified by
	iter has. If iter is None, the number of top level rows is returned.
	"""
	def on_iter_n_children(self, iter) :
		if iter:
			if self.iter_is_valid(iter) :
				iter_data = self.get_user_data(iter)
				return self._n_child_nodes(iter_data)
		return 1
	"""
	The on_iter_nth_child() method should return a row reference to the nth child row of the row
	specified by parent. If parent is None, a reference to the nth top level row is returned.
	"""
	def on_iter_nth_child(self, parent, n) :
		node = None 
		if parent :
			if self.iter_is_valid(parent) :
				parent = self.get_user_data(parent)
			if self._n_child_nodes(parent) > n :
				node = self._get_n_child(parent, n)
		else :
			if n == 0 :
				node = self.xml
		if node:
			return self.create_tree_iter(node)			
		return None
		
	"""
	The on_iter_parent() method should return a row reference to the parent row of the row specified by child.
	If rowref points to a top level row, None should be returned.
	"""
	def on_iter_parent(self, child) :
		if child :
			if self.iter_is_valid(child) :
				iter_data = self.get_user_data(child)
				if not iter_data.isSameNode(self.xml) :
					parent = iter_data.parentNode
					return self.create_tree_iter(parent)
		return None
	"""
	
	Internal functions
	
	"""
	def _first_element_child(self, node) :
		if node :
			if node.attributes :
				child = XMLTreeModelAttr(node.attributes.item(0), node, 0)
				return child
			if node.hasChildNodes() :
				child = node.firstChild
				while _is_blank_node(child) :
					child = child.nextSibling
				if child :
					return child
		return None

	def _next_element_sibling(self, node) :
		if not node:
			return None
		result = node.nextSibling

		if node.nodeType == Node.ATTRIBUTE_NODE :
			if not result :
				result = node.parentNode.firstChild

		while _is_blank_node(result) :
			result = result.nextSibling
		return result

	def _previous_element_sibling(self, node) :
		if not node:
			return None
		result = node.previousSibling
		while _is_blank_node(result) :
			result = result.previousSibling
		if not result :
			if node.nodeType != Node.ATTRIBUTE_NODE and node.parentNode :
				if node.parentNode.attributes :
					idx = node.parentNode.attributes.length
					result = XMLTreeModelAttr(node.parentNode.attributes.item(idx-1),node.parentNode,idx-1)
		return result

	def _get_node_content(self, node) :
		if node.nodeType == Node.DOCUMENT_NODE or Node.ELEMENT_NODE:
			if node.hasChildNodes() :
				node.normalize()
				for child in node.childNodes :
					if child.nodeType == Node.TEXT_NODE :
						return child.data.strip()
					else :
						return None
		elif node.nodeType == Node.DOCUMENT_TYPE_NODE :
			return node.systemId
		elif node.nodeType == Node.TEXT_NODE :
			return node.data.strip()
		return node.nodeValue

	def _has_child_nodes(self, parent) :
		if parent.attributes :
			return True
		if parent.hasChildNodes() :
			for node in parent.childNodes :
				if not _is_blank_node(node) :
					return True
		return False

	def _n_child_nodes(self, parent) :
		num_nodes = 0
		if parent.attributes :
			num_nodes += parent.attributes.length
		if self._has_child_nodes(parent) :
			for node in parent.childNodes :
				if not _is_blank_node(node) :
					num_nodes += 1
		return num_nodes

	def _get_n_child(self, parent, n) :
		children = []
		if parent.attributes :
			for idx in range(0,parent.attributes.length) :
				children.append(XMLTreeModelAttr(parent.attributes.item(idx), parent, idx))
		for node in parent.childNodes :
			if not _is_blank_node(node) :
				children.append(node)
		if len(children) > n :
			return children[n]
		return None
		
