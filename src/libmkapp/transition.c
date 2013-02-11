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
#include "transition.h"

MkTransition* mk_transition_new(const gchar* src_state,
                                const gchar* signal,
                                const gchar* dst_state,
                                const gchar* output)
{
    MkTransition* t = g_malloc(sizeof(MkTransition));
    t->src_state = g_strdup(src_state);
    t->signal = g_strdup(signal);
    t->dst_state = g_strdup(dst_state);
    t->output = g_strdup(output);
    return t;
}


void mk_transition_delete(MkTransition* t)
{
    g_free(t->src_state);
    g_free(t->signal);
    g_free(t->dst_state);
    g_free(t->output);
    g_free(t);
}


void mk_transition_add(GHashTable*   table,
                       MkTransition* t)
{
    // Find or create the array of transitions for this source state
    GPtrArray* array = g_hash_table_lookup(table, t->src_state);
    if (array == NULL) {
        gchar* src_state = g_strdup(t->src_state);
        array = g_ptr_array_new();
        g_hash_table_insert(table, src_state, array);
    }

    // If a transition already exists for that signal, remove it
    guint i;
    for (i = 0; i < array->len; ++i) {
        MkTransition* t2 = (MkTransition*)g_ptr_array_index(array, i);

        if (!g_strcmp0(t->signal, t2->signal)) {
            g_ptr_array_remove_index(array, i);
            mk_transition_delete(t2);
            break;
        }
    }

    // Add the new transition
    g_ptr_array_add(array, t);
}


MkTransition* mk_transition_lookup(GHashTable* table,
                                   const gchar* src_state,
                                   const gchar* signal,
                                   gchar** output)
{
    if (output != NULL)
        *output = NULL;

    // Find all transitions that start from the source state
    GPtrArray* array = g_hash_table_lookup(table, src_state);
    if (array == NULL)
        return NULL;

    // Find the first transition that corresponds to the signal. A transition
    // corresponds to the signal if the signal's regular expression matches
    // the signal received.
    gint i;
    for (i = 0; i < array->len; ++i) {
        MkTransition* t = g_ptr_array_index(array, i);
        GError* error = NULL;
        GRegex* regex = g_regex_new(t->signal, 0, 0, &error);
        if (error != NULL)
            g_critical("regular expression error: %s", error->message);
        
        GMatchInfo* match_info;
        gboolean matches = g_regex_match(regex, signal, 0, &match_info);

        
        if (matches) {
            if (output != NULL && t->output != NULL) {
                *output = g_match_info_expand_references(match_info,
                                                         t->output,
                                                         &error);
                if (error != NULL)
                    g_critical("error expanding output string: %s",
                               error->message);
            }
        }


        if (match_info != NULL)
            g_match_info_free(match_info);

        g_regex_unref(regex);


        if (matches)        
            return t;
    }

    return NULL;
}


void mk_transition_list_delete(GPtrArray* array)
{
    if (array == NULL)
        return;

    gint i;
    for(i = 0; i < array->len; ++i)
        mk_transition_delete(g_ptr_array_index(array, i));
}
