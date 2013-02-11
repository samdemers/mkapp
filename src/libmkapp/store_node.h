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

/**
 * @file
 * Data store nodes.
 */


#ifndef __STORE_NODE_H__
#define __STORE_NODE_H__

#include <glib.h>


/**
 * Key/value-based hierarchical data store put their data into trees made of
 * GNode nodes. This structure contains these nodes' key and value strings.
 *
 * @brief Data store node information.
 */
typedef struct {
    gchar* name;  /// Node name
    gchar* value; /// Data in the node
} MkNodeInfo;


/**
 * Create a new node with node information as data.
 * @param name node name
 * @return     the new node
 */
GNode* mk_store_node_new(const gchar* name);


/**
 * Free node information.
 * @param node the node
 * @param info the node info that must be freed
 */
void mk_store_node_info_free(const GNode* node, MkNodeInfo* info);


/**
 * Free a node and all its children.
 * @param node the node
 */
void mk_store_node_free(GNode* node);


/**
 * Get a node in a tree, using its path.
 * @param root   the root node to start searching from
 * @param name   the path
 * @param create create the node if it does not exist ?
 * @return     the node (found or created)
 */
GNode* mk_store_node_get_by_name(GNode*         root,
                                 const gchar*   name,
                                 const gboolean create);

#endif // __STORE_NODE_H__
