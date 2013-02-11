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

#include <stdio.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "store_node.h"
#include "store_key_value.h"


void mk_key_value_write_lines(const gchar* prefix, 
                           const GNode* root,
                           GIOChannel* chan)
{
    GNode* n = g_node_first_child(root);
    for(; n != NULL; n = g_node_next_sibling(n)) {
        MkNodeInfo* info = (MkNodeInfo*)(n->data);

        gchar* prefix2;
        if (prefix != NULL) {
            prefix2 = g_strconcat(prefix, ".", info->name, NULL);
        } else {
            prefix2 = g_strdup(info->name);
        }

        if (info->name != NULL) {
            if (prefix != NULL) {
                g_io_channel_write_chars(chan, prefix, -1, NULL, NULL);
                g_io_channel_write_chars(chan, ".", -1, NULL, NULL);
            }
            g_io_channel_write_chars(chan, info->name, -1, NULL, NULL);
            g_io_channel_write_chars(chan, "=", -1, NULL, NULL);

            if (info->value != NULL)
                g_io_channel_write_chars(chan, info->value, -1, NULL, NULL);

            g_io_channel_write_chars(chan, "\n", -1, NULL, NULL);
        }

        mk_key_value_write_lines(prefix2, n, chan);
        g_free(prefix2);
    }    
}


void mk_key_value_write_file(GNode* root, const gchar* filename)
{
    GError* error = NULL;
    GIOChannel* chan = g_io_channel_new_file(filename, "w", &error);

    if (error != NULL)
        g_critical("Error opening %s: %s", filename, error->message);

    mk_key_value_write_lines(NULL, root, chan);

    g_io_channel_unref(chan);
}


gboolean mk_key_value_read_line(GNode*       root,
                                const gchar* line,
                                gchar**      key_ret,
                                gchar**      value_ret)
{
    gboolean changed = FALSE;
    gchar* key = NULL;
    gchar* value = NULL;

    // Initialize key_ret and value_ret
    if (key_ret != NULL)
        *key_ret = NULL;
    if (value_ret != NULL)
        *value_ret = NULL;

    // Parse the line. The line is a key=value pair, where value can be
    // surrounded by quotes.
    GError* error = NULL;
    GRegex* regex = g_regex_new
        ("\\s*(\\w+)\\s*=\\s*(?:(?:\"(.*?)\")|(.*?))\\n?$", 0, 0, &error);
    if (error != NULL)
        g_critical("could not compile regular expression: %s", error->message);

    GMatchInfo* match_info;
    g_regex_match(regex, line, 0, &match_info);
    if (g_match_info_matches(match_info)) {
        key = g_match_info_fetch(match_info, 1);
        value = g_match_info_fetch(match_info, 2);
        if (value[0] == 0) {
            g_free(value);
            value = g_match_info_fetch(match_info, 3);
        }
        
    } else {
        g_match_info_free(match_info);
        g_regex_unref(regex);
        return FALSE;
    }
    g_match_info_free(match_info);
    g_regex_unref(regex);

    // Find or create the node and change it if needed.
    GNode* node = mk_store_node_get_by_name(root, key, TRUE);
    g_assert(node != NULL);
    MkNodeInfo* info = (MkNodeInfo*)(node->data);

    if (value != NULL && g_strcmp0(value, info->value)) {
        // Set the value
        if (info->value != NULL) 
            g_free(info->value);

        info->value = g_strdup(value);

	   changed = TRUE;
    }

    // If the caller needs the key and/or the value, return them.
    if (key_ret != NULL)
        *key_ret = key;
    else
        g_free(key);

    if (value_ret != NULL)
        *value_ret = value;
    else
        g_free(value);

    return changed;
}

void mk_key_value_read_file(GNode* root,
                            const gchar* filename,
                            const gboolean read_only)
{
    GError* error = NULL;
    GIOChannel* chan;
    GIOStatus status;
    gchar* line = NULL;
    gsize length;

    // If not in read-only mode, accept that the file may not already exist.
    if (!read_only && !g_file_test(filename, G_FILE_TEST_EXISTS))
        return;

    chan = g_io_channel_new_file(filename, "r", &error);

    if (error != NULL)
        g_critical("Error opening %s: %s", filename, error->message);

    while ((status = g_io_channel_read_line(chan, &line, &length,
                                            NULL, &error))
           == G_IO_STATUS_NORMAL ) {

        if (length > 0)
            g_strchomp(line);

        mk_key_value_read_line(root, line, NULL, NULL);
    }

    g_io_channel_unref(chan);
    g_free(line);
}
