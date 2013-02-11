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
#include <string.h>
#include "parser.h"


MkParserContext* mk_parser_new(void* user_data)
{
    MkParserContext* parser = g_malloc(sizeof(MkParserContext));

    parser->eof_func      = NULL;
    parser->user_data     = user_data;
    parser->depth         = 1;
    parser->tokens        = g_ptr_array_new();
    parser->current_token = NULL;
    mk_parser_configure_default(parser, NULL);

    return parser;
}


void mk_parser_free(MkParserContext* parser)
{
    g_free(parser);
}


void mk_parser_set_eof_func(MkParserContext* parser, MkParserFunc f)
{
    parser->eof_func = f;
}


void mk_parser_push(MkParserContext* parser)
{
    g_assert(parser->depth < MK_PARSER_MAX_DEPTH);
    ++(parser->depth);
    mk_parser_configure_default(parser, NULL);
}


void mk_parser_pop(MkParserContext* parser)
{
    --(parser->depth);
    g_assert(parser->depth > 0);
}


void mk_parser_configure(MkParserContext* parser,
                         const gchar      c,
                         MkParserFunc     f)
{
    g_assert(c >= MK_PARSER_FIRST_CHAR && c <= MK_PARSER_LAST_CHAR);
    int i = (int)c - MK_PARSER_FIRST_CHAR;
    parser->f[parser->depth-1][i] = f;
}


void mk_parser_configure_range(MkParserContext* parser,
                               const gchar      c1,
                               const gchar      c2,
                               MkParserFunc     f)
{
    if (c1 < c2) {
        for (gchar c = c1; c < c2; ++c)
            mk_parser_configure(parser, c, f);
        mk_parser_configure(parser, c2, f);
    } else {
        for (gchar c = c2; c < c1; ++c)
            mk_parser_configure(parser, c, f);
        mk_parser_configure(parser, c1, f);
    }
}


void mk_parser_configure_all(MkParserContext* parser,
                             const gchar*     chars,
                             MkParserFunc        f)
{
    for(gsize i = 0; chars[i] != '\0'; ++i) {
        mk_parser_configure(parser, chars[i], f);
    }
}


void mk_parser_configure_default(MkParserContext* parser, MkParserFunc f)
{
    mk_parser_configure_range(parser,
                              MK_PARSER_FIRST_CHAR,
                              MK_PARSER_LAST_CHAR,
                              f);
}


void mk_parser_parse_character(MkParserContext* parser, const gchar c)
{
    MkParserFunc f;
    int i = (int)c - MK_PARSER_FIRST_CHAR;

    // Non-ascii characters are all treated equally
    if (i > MK_PARSER_LAST_CHAR || i < 0)
        i = MK_PARSER_NON_ASCII;

    f = parser->f[parser->depth-1][i];
    if (f != NULL)
        f(parser, c, parser->user_data);
}


void mk_parser_token_append(MkParserContext* parser, gchar c)
{
    if (parser->current_token == NULL)
        parser->current_token = g_string_new("");
    g_string_append_c(parser->current_token, c);
}


void mk_parser_token_cut(MkParserContext* parser)
{
    if (parser->current_token != NULL) {
        g_ptr_array_add(parser->tokens, parser->current_token->str);
        g_string_free(parser->current_token, FALSE);
        parser->current_token = NULL;
    }
}


void mk_parser_token_add(MkParserContext* parser, const gchar* token)
{
    g_ptr_array_add(parser->tokens, g_strdup(token));
}


void mk_parser_token_clear(MkParserContext* parser)
{
    g_ptr_array_foreach(parser->tokens, (GFunc)g_free, NULL);
    g_ptr_array_free(parser->tokens, TRUE);
    parser->tokens = g_ptr_array_new();
}


const gchar** mk_parser_token_get(MkParserContext* parser)
{
    return (const gchar**)(parser->tokens->pdata);
}


gsize mk_parser_token_size(MkParserContext* parser)
{
    return parser->tokens->len;
}


void mk_parser_dquote_begin(MkParserContext* parser, gchar c, void* data)
{
    mk_parser_push(parser);
    mk_parser_configure_default(parser, (MkParserFunc)mk_parser_token_append);
    mk_parser_configure(parser, '"', (MkParserFunc)mk_parser_pop);
    mk_parser_configure(parser, '\\', mk_parser_escape_begin);
}


void mk_parser_squote_begin(MkParserContext* parser, gchar c, void* data)
{
    mk_parser_push(parser);
    mk_parser_configure_default(parser, (MkParserFunc)mk_parser_token_append);
    mk_parser_configure(parser, '\'', (MkParserFunc)mk_parser_pop);
    mk_parser_configure(parser, '\\', mk_parser_strict_escape_begin);
}


void mk_parser_comment_end(MkParserContext* parser, gchar c, void* data)
{
    mk_parser_pop(parser);
    mk_parser_parse_character(parser, '\n');
}


void mk_parser_comment_begin(MkParserContext* parser, gchar c, void* data)
{
    mk_parser_push(parser);
    mk_parser_configure(parser, '\n', mk_parser_comment_end);
}


void mk_parser_strict_escape_end(MkParserContext* parser, gchar c, void* data)
{
    switch(c) {
    case '"':
    case '\'':
        mk_parser_token_append(parser, c);
        break;

    default:
        mk_parser_token_append(parser, '\\');
        mk_parser_token_append(parser, '\\');
        mk_parser_token_append(parser, c);
    }

    mk_parser_pop(parser);
}


void mk_parser_strict_escape_begin(MkParserContext* parser,
                                   gchar            c,
                                   void*            data)
{
    mk_parser_push(parser);
    mk_parser_configure_default(parser, mk_parser_strict_escape_end);
}


void mk_parser_escape_end(MkParserContext* parser, gchar c, void* data)
{
    switch(c) {
    case '"':
    case '\'':
        mk_parser_token_append(parser, c);
        break;

    default:
        mk_parser_token_append(parser, '\\');
        mk_parser_token_append(parser, c);
    }

    mk_parser_pop(parser);
}


void mk_parser_escape_begin(MkParserContext* parser, gchar c, void* data)
{
    mk_parser_push(parser);
    mk_parser_configure_default(parser, mk_parser_escape_end);
}


void mk_parser_enable_defaults(MkParserContext* parser)
{
    mk_parser_configure(parser, '"', mk_parser_dquote_begin);
    mk_parser_configure(parser, '\'', mk_parser_squote_begin);
    mk_parser_configure(parser, '#', mk_parser_comment_begin);
    mk_parser_configure(parser, '\\', mk_parser_escape_begin);
}


gboolean mk_parser_parse_channel(GIOChannel*      source,
                                 GIOCondition     unused,
                                 MkParserContext* parser)
{
    while(1) {
        gchar*    data;
        gsize     length;
        GError*   error = NULL;
        GIOStatus status;
        
        status = g_io_channel_read_line(source, &data, &length, NULL, &error);

        switch(status) {
        case G_IO_STATUS_ERROR:
            // An error occurred
            g_free(data);
            g_critical("Input/output error.");
            return FALSE;
            
        case G_IO_STATUS_NORMAL:
            // Success
            for (gsize i = 0; i < length; ++i)
                mk_parser_parse_character(parser, data[i]);
            break;
            
        case G_IO_STATUS_EOF:
            // End of file. Quit as soon as all the modules have stopped.
            g_free(data);
            if (parser->eof_func != NULL)
                parser->eof_func(parser, 0, parser->user_data);
            
            return FALSE;
            
        case G_IO_STATUS_AGAIN:
            // Resource temporarily unavailable
            g_free(data);
            return TRUE;
        }

        g_free(data);
    }

    return TRUE;
}


void mk_parser_parse_file(MkParserContext* parser, const gchar* filename)
{
    GError* error = NULL;
    GIOChannel* chan = g_io_channel_new_file(filename, "r", &error);

    if (error != NULL) {
        g_critical("Could not read %s: %s", filename, error->message);
        g_free(error);
        return;
    }

    while(mk_parser_parse_channel(chan, 0, parser));
    g_io_channel_unref(chan);
}
