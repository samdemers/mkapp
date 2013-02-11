/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <glib.h>
#include "store_node.h"


GNode* mk_store_node_new(const gchar* name)
{
    MkNodeInfo* info = g_malloc(sizeof(MkNodeInfo));
    info->name = g_strdup(name);
    info->value = NULL;
    
    GNode* node = g_node_new(info);
    return node;
}


void mk_store_node_info_free(const GNode* node, MkNodeInfo* info)
{
    // Free name
    if (info->name != NULL)
        g_free(info->name);

    // Free value
    if (info->value != NULL)
        g_free(info->value);
}


void mk_store_node_free(GNode* node)
{
    g_node_traverse(node,
                    G_IN_ORDER,
                    G_TRAVERSE_ALL,
                    -1,
                    (GNodeTraverseFunc)mk_store_node_info_free,
                    node->data);
    g_node_destroy(node);
}


GNode* mk_store_node_get_by_name(GNode*         root,
                                 const gchar*   name,
                                 const gboolean create)
{
    gchar** tokens = g_strsplit(name, ".", 2);
    GNode* node = NULL;

    // If we have reached the end of the name, stop.
    if (tokens[0] == NULL)
        return NULL;

    // Find the first part of name (before the next dot) under root.
    GNode* n = g_node_first_child(root);
    for(; n != NULL; n = g_node_next_sibling(n)) {
        MkNodeInfo* info = (MkNodeInfo*)(n->data);
        if (!g_strcmp0(tokens[0], info->name)) {
            if (tokens[1] == NULL)
                node = n;
            else
                node = mk_store_node_get_by_name(n, tokens[1], create);
        }

        if (node != NULL)
            break;
    }

    // If requested, create the missing node
    if (create && node == NULL) {
        node = mk_store_node_new(tokens[0]);

        g_node_append(root, node);

        if (tokens[1] != NULL)
            node = mk_store_node_get_by_name(node, tokens[1], create);
    }

    g_strfreev(tokens);
    return node;
}
