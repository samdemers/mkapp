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

#include <stdlib.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "parser.h"
#include "mkmachine_parser.h"
#include "transition.h"

#define PACKAGE_NAME         "machine2dot"
#define PACKAGE_VERSION      "0.1"
#define PACKAGE_PARAM_STRING "FILE"

gchar** files = NULL; // Input files
gboolean version;     // Obtain version information ?


/**
 * Command-line options.
 */
static GOptionEntry entries[] = {
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &files,
      "State machine definition file", NULL },
    { "version", 'V', 0, G_OPTION_ARG_NONE,
      (gpointer)&version, "Print version information", NULL },
    { NULL }
};


void state2node(const gchar* state_name,
               const GPtrArray* transitions,
               const gchar* default_state)
{
    gchar* e_state_name = g_strescape(state_name, NULL);

    if (!g_ascii_strcasecmp(state_name, default_state)) {
        g_printf("\t\t{\n");
        g_printf("\t\t\tnode [style=bold]\n");
        g_printf("\t\t\t%s;\n", e_state_name);
        g_printf("\t\t}\n");
    } else {
        g_printf("\t\t%s;\n", e_state_name);
    }

    g_free(e_state_name);
}

gchar* substitute(gchar** str, const gchar* from, const gchar* to)
{
    gchar** tokens = g_strsplit(*str, from, -1);
    g_free(*str);
    *str = g_strjoinv(to, tokens);
    g_strfreev(tokens);
    return *str;
}


void transition2node(const MkTransition* t,
                     const gpointer unused)
{
    gchar* e_signal = g_strescape(t->signal, NULL);
    gchar* e_src_state = g_strescape(t->src_state, NULL);

    gchar* e_output = g_strdup(t->output);

    substitute(&e_signal, ">", "&gt;");
    substitute(&e_signal, "<", "&lt;");

    if (t->output != NULL) {
        substitute(&e_output, ">", "&gt;");
        substitute(&e_output, "<", "&lt;");

        gchar** out_lines = g_strsplit(e_output, "\n", -1);
        g_free(e_output);
        e_output = g_strdup("");

        for(gint i = 0; out_lines[i] != NULL; ++i) {
            if (!g_regex_match_simple("^\\s*$", out_lines[i], 0, 0)) {
                gchar* new_output;
                new_output = g_strconcat(e_output, "<br align=\"left\"/>",
                                         out_lines[i], "\n", NULL);
                g_free(e_output);
                e_output = new_output;
            }
        }
        g_strfreev(out_lines);
    }

    g_printf("\t\t\"%s-%s\"[label=<\n", e_src_state, e_signal);
    g_printf("<table align=\"left\" border=\"0\">\n");
    g_printf("<tr><td><font point-size=\"16\">%s</font></td></tr>\n",
             e_signal);
    if (t->output != NULL) {
        g_printf("<tr><td align=\"text\">"
                 "<font face=\"courier\" point-size=\"14\">%s</font>"
                 "</td></tr>\n",
                 e_output);
    }
    g_printf("</table>\n");
    g_printf("\t\t>];\n");
    
    g_free(e_output);
    g_free(e_src_state);
    g_free(e_signal);
}

void transition2edge(const MkTransition* t,
                     const gpointer unused)
{
    gchar* e_signal = g_strescape(t->signal, NULL);
    gchar* e_src_state = g_strescape(t->src_state, NULL);
    gchar* e_dst_state = g_strescape(t->dst_state, NULL);

    substitute(&e_signal, ">", "&gt;");
    substitute(&e_signal, "<", "&lt;");

    g_printf("\t\t\"%s\" -> \"%s-%s\";\n", e_src_state, e_src_state, e_signal);
    g_printf("\t\t\"%s-%s\" -> \"%s\";\n", e_src_state, e_signal, e_dst_state);
    
    g_free(e_dst_state);
    g_free(e_src_state);
    g_free(e_signal);
}

void transitions2dot(const gchar* src_state_name,
                     GPtrArray* transitions,
                     const gpointer unused)
{
    g_printf("\t{\n");
    g_printf("\t\tnode [shape=box]\n");
    g_ptr_array_foreach(transitions, (GFunc)transition2node, NULL);
    g_printf("\t}\n");
    g_printf("\n");

    g_printf("\t{\n");
    g_printf("\t\tedge []\n");
    g_ptr_array_foreach(transitions, (GFunc)transition2edge, NULL);
    g_printf("\t}\n");
    g_printf("\n");
}

/**
 * Main
 */
int main(int argc, char* argv[])
{
    GHashTable* transitions;
    gchar* default_state;

    // Make critical errors fatal to abort when they happen
    g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);

    // Read command-line arguments
    GError* error = NULL;
    GOptionContext* context;
    context = g_option_context_new(PACKAGE_PARAM_STRING);
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error))
        g_critical("%s", error->message);

    if (version) {
        g_printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        exit(EXIT_SUCCESS);
    }

    if (files == NULL || files[0] == NULL)
        g_critical("No input file");
    if (files[1] != NULL)
        g_critical("Too many input files.");

    // Parse the state machine definition
    transitions = g_hash_table_new_full
        (g_str_hash, g_str_equal, NULL,
         (GDestroyNotify)mk_transition_list_delete);
    MkParserContext* parser = mk_machine_parser_new(transitions);
    mk_parser_parse_file(parser, files[0]);
    default_state = g_strdup(mk_machine_parser_get_default_state(parser));
   

    // Write the dot file
    g_printf("digraph \"%s\" {\n", files[0]);
    
    // Nodes
    g_printf("\n");
    g_printf("\t// States\n");
    g_printf("\t{\n");
    g_printf("\t\tnode [shape=circle]\n");
    g_hash_table_foreach(transitions, (GHFunc)state2node, default_state);
    g_printf("\t}\n");

    // Edges
    g_printf("\n\n");
    g_printf("\t// Transitions\n");
    g_hash_table_foreach(transitions, (GHFunc)transitions2dot, NULL);
    g_printf("}\n");

    g_free(files);
}
