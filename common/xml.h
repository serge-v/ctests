// Serge Voilokov, 2015
// helpers for libxml2.

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <sys/queue.h>

/* get text */
const char *get_ctext(const xmlNodePtr node);

/* get attribute */
const char *get_attr(const xmlNodePtr node, const char *name);

/* iterate */
xmlNodePtr first_el(xmlNodePtr parent_node, const char *name);
xmlNodePtr next_el(xmlNodePtr node);

/* debug */
void dump_tree(xmlNodePtr n);
