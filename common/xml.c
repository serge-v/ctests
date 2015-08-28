/* Serge Voilokov, 2015
 * helpers for libxml2.
 */

#include "xml.h"
#include <string.h>
#include <stdbool.h>
#include <err.h>
#include <libxml/tree.h>

const char *
get_ctext(const xmlNodePtr node)
{
	xmlNodePtr n;

	if (node == NULL)
		return NULL;

	for (n = node->children; n != NULL; n = n->next)
		if (n->type == XML_TEXT_NODE)
			return (char *)n->content;

	return NULL;
}

const char *
get_attr(const xmlNodePtr node, const char *name)
{
	if (node == NULL && node->type != XML_ELEMENT_NODE)
		return NULL;

	xmlNodePtr n = (xmlNodePtr)((xmlElementPtr)node)->attributes;

	for (; n != NULL; n = n->next)
		if (n->type == XML_ATTRIBUTE_NODE && xmlStrEqual(n->name, BAD_CAST(name)) == 1)
			return get_ctext(n);

	return NULL;
}

xmlNodePtr
first_el(xmlNodePtr parent_node, const char *name)
{
	xmlNodePtr n;

	if (parent_node == NULL)
		return NULL;

	for (n = parent_node->children; n != NULL; n = n->next)
		if (n->type == XML_ELEMENT_NODE && xmlStrEqual(n->name, BAD_CAST(name)) == 1)
			return n;

	return NULL;
}

xmlNodePtr
next_el(xmlNodePtr node)
{
	xmlNodePtr n;

	for (n = node->next; n != NULL; n = n->next)
		if (n->type == XML_ELEMENT_NODE && xmlStrEqual(n->name, node->name) == 1)
			return n;

	return NULL;
}

void
print_element_names(xmlNode *a_node, int depth)
{
	xmlNode *n = NULL;

	for (n = a_node; n != NULL; n = n->next) {
		if (n->type == XML_ELEMENT_NODE)
			printf("%*s%s\n", depth*4, "", n->name);

		print_element_names(n->children, depth+1);
	}
}
