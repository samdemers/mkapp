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
 * Key/value strings parsing.
 *
 * The functions provided here handle the parsing of simple key=value or
 * key = "value" strings. A hierarchy of keys is supported, where a key
 * can have the form "parent2.parent.child". The results of the parsing
 * are stored in a tree of GNode nodes.
 */


#ifndef __store_key_value_h__
#define __store_key_value_h__

#include <glib.h>


/**
 * Write key/value lines for all nodes below root, including root itself.
 * @param prefix string corresponding to the path to root, excluding root
 * @param root   the node to start printing from
 * @param chan   the IO Channel to write to
 */
void mk_key_value_write_lines(const gchar* prefix, 
                              const GNode* root,
                              GIOChannel*  chan);

/**
 * Write the whole data tree to a file.
 * @param root     root node of the tree to populate
 * @param filename the file
 */
void mk_key_value_write_file(GNode* root, const gchar* filename);

/**
 * Read a key/value line and update the tree. If key_ret or value_ret
 * are not NULL, the call must free them after use.
 * @param root      the tree
 * @param line      the key=value string
 * @param ret_key   return location for the key, or NULL
 * @param ret_value return location for the value, or NULL
 * @return          whether changes have been made to root
 */
gboolean mk_key_value_read_line(GNode*       root,
                                const gchar* line,
                                gchar**      ret_key,
                                gchar**      ret_value);

/**
 * Read a whole key/value file and fill the tree.
 * @param root      root of the tree to fill
 * @param filename  the file
 * @param read_only whether the file must be created if it does not exist
 */
void mk_key_value_read_file(GNode*         root,
                            const gchar*   filename,
                            const gboolean read_only);


#endif // __store_key_value_h__
