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
#include <glib/gprintf.h>

#include "parser.h"
#include "mkmachine_parser.h"
#include "transition.h"

typedef struct {
    GHashTable* transitions;
    gchar*      default_state;
} MkmachineParserData;


void disable_whitespace(MkParserContext* parser, gchar c, GHashTable* data)
{
    mk_parser_configure_all(parser, " \t", NULL);
    mk_parser_token_append(parser, c);
}


void enable_whitespace(MkParserContext* parser, gchar c, GHashTable* data)
{
    mk_parser_configure_all(parser, " \t",
                            (MkParserFunc)mk_parser_token_append);
    mk_parser_configure_all(parser, "\n", (MkParserFunc)disable_whitespace);
    mk_parser_token_append(parser, c);
}


void transition_end(MkParserContext* parser, gchar c, GHashTable* data)
{
    mk_parser_token_cut(parser);
    mk_parser_pop(parser);
}


void transition_begin(MkParserContext*     parser,
                      gchar                c,
                      MkmachineParserData* data)
{
    mk_parser_token_cut(parser);
    mk_parser_push(parser);
    mk_parser_configure_default(parser, (MkParserFunc)enable_whitespace);
    mk_parser_enable_defaults(parser);
    mk_parser_configure(parser, '}', (MkParserFunc)transition_end);
    mk_parser_configure_all(parser, " \t\n", NULL);
}


void src_state_end(MkParserContext* parser, gchar c, MkmachineParserData* data)
{

    gsize size = mk_parser_token_size(parser);
    const gchar** tokens = mk_parser_token_get(parser);

    // Add the transitions found
    if (size > 0) {
        const gchar* src_state = tokens[0];

        for(gsize i = 1; i+2 < size; i += 3) {
            const gchar* signal = tokens[i];
            const gchar* dst_state = tokens[i+1];
            const gchar* output = tokens[i+2];

            MkTransition* t;
            t = mk_transition_new(src_state, signal, dst_state, output);
            mk_transition_add(data->transitions, t);
        }
        
        // Set the default state to the first state we see
        if (data->default_state == NULL)
            data->default_state = g_strdup(src_state);
    }

    mk_parser_token_clear(parser);
    mk_parser_pop(parser);
}


void dst_state_end(MkParserContext*     parser,
                   gchar                c,
                   MkmachineParserData* data)
{
    mk_parser_pop(parser);
    transition_begin(parser, c, data);
}


void dst_state_begin(MkParserContext*     parser,
                     gchar                c,
                     MkmachineParserData* data)
{
    mk_parser_pop(parser);
    mk_parser_push(parser);
    mk_parser_configure_default(parser, (MkParserFunc)mk_parser_token_append);
    mk_parser_enable_defaults(parser);
    mk_parser_configure(parser, '{', (MkParserFunc)dst_state_end);
    mk_parser_configure_all(parser, " \t\n", NULL);
}


void rarrow_operator_begin(MkParserContext*            parser,
                           gchar                c,
                           MkmachineParserData* data)
{
    mk_parser_push(parser);
    mk_parser_token_cut(parser);
    mk_parser_configure(parser, '>', (MkParserFunc)dst_state_begin);
}


void empty_dst_transition_begin(MkParserContext*     parser,
                                gchar                c,
                                MkmachineParserData* data)
{
    const gchar** tokens = mk_parser_token_get(parser);
    g_assert(mk_parser_token_size(parser) >= 1);
    const gchar* src_state = g_strdup(tokens[0]);

    mk_parser_token_cut(parser);
    mk_parser_token_add(parser, src_state);
    transition_begin(parser, c, data);
}


void src_state_begin(MkParserContext*     parser,
                     gchar                c,
                     MkmachineParserData* data)
{
    mk_parser_token_cut(parser);
    mk_parser_push(parser);
    mk_parser_configure_default(parser, (MkParserFunc)mk_parser_token_append);
    mk_parser_enable_defaults(parser);
    mk_parser_configure(parser, '=', (MkParserFunc)rarrow_operator_begin);
    mk_parser_configure(parser, '{', (MkParserFunc)empty_dst_transition_begin);
    mk_parser_configure(parser, '}', (MkParserFunc)src_state_end);
    mk_parser_configure_all(parser, " \t\n", NULL);
}


const gchar* mk_machine_parser_get_default_state(MkParserContext* parser)
{
    MkmachineParserData* data = (MkmachineParserData*)(parser->user_data);
    return data->default_state;
}


MkParserContext* mk_machine_parser_new(GHashTable* transitions)
{
    // Initialize parser data
    MkmachineParserData* data = g_malloc(sizeof(MkmachineParserData));
    data->transitions = transitions;
    data->default_state = NULL;

    // Initialize parser
    MkParserContext* parser = mk_parser_new(data);
    mk_parser_configure_default(parser, (MkParserFunc)mk_parser_token_append);
    mk_parser_enable_defaults(parser);
    mk_parser_configure(parser, '{', (MkParserFunc)src_state_begin);
    mk_parser_configure_all(parser, " \t\n", NULL);

    return parser;
}


/* TODO: mk_machine_parser_free() */
