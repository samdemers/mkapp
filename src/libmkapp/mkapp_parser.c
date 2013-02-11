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
#include <gmodule.h>

#include "module.h"
#include "parser.h"


/**
 * Command function type. All mkapp commands are executed by a command_...
 * fonction with this signature.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules table of declared modules
 * @return        an error string, or NULL if there are no errors
 */
typedef const gchar*(*CommandFunc)(const gchar**    tokens,
                                   const gsize      length,
                                   MkModuleContext* modules);


typedef struct {
    MkModuleContext* modules;
} MkappParserData;


/**
 * Execute a command based on its token list. A function called
 * "mk_command_...", where ... is the command name, will be looked up.
 * If it exists, it will be executed.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules the module context that commands will work on
 */
void execute_command(const char** tokens,
                     const gsize length,
                     MkModuleContext* modules)
{
    // Use GModule to find the function for this command
    gboolean command_is_supported = FALSE;

    if (g_module_supported()) {
        
        GModule* module = g_module_open(NULL, 0); // Main program
        CommandFunc fun;

        // Expand all escape sequences
        gchar** tokens_expanded = g_new(gchar*, length+1);
        for (gsize i = 0; i < length; ++i)
            tokens_expanded[i] = g_strcompress(tokens[i]);
        tokens_expanded[length] = NULL;
        
        // Find the function
        gchar* symbol_name = g_strconcat("mk_command_", tokens[0], NULL);
        g_debug("%s()", symbol_name);
        g_module_symbol(module, symbol_name, (gpointer*)&(fun));
        g_free(symbol_name);

        // Execute the function found
        if (fun != NULL) {
            command_is_supported = TRUE;
            const gchar* error = fun((const gchar**)tokens_expanded,
                                     length, modules);
            if (error != NULL)
                g_fprintf(stderr, "%s: %s\n", tokens[0], error);
        }

        g_module_close(module);
    }

    if (!command_is_supported)
        g_fprintf(stderr, "%s: command not found.\n", tokens[0]);

}


void command_end(MkParserContext* parser, gchar c, MkappParserData* data)
{
    mk_parser_token_cut(parser);

    if (parser->tokens != NULL && parser->tokens->len > 0) {
        execute_command(mk_parser_token_get(parser),
                        mk_parser_token_size(parser),
                        data->modules);
        mk_parser_token_clear(parser);
    }
    
}


void eof_received(MkParserContext* parser, gchar c, MkappParserData* data)
{
    mk_module_eof_received(data->modules);
}


MkParserContext* mk_app_parser_new(MkModuleContext* modules)
{
    MkappParserData* data = g_malloc(sizeof(MkappParserData));
    data->modules = modules;

    MkParserContext* parser = mk_parser_new(data);
    mk_parser_set_eof_func(parser, (MkParserFunc)eof_received);

    mk_parser_configure_default(parser, (MkParserFunc)mk_parser_token_append);
    mk_parser_enable_defaults(parser);
    mk_parser_configure(parser, ';', (MkParserFunc)command_end);
    mk_parser_configure_all(parser, " \t\n",
                            (MkParserFunc)mk_parser_token_cut);

    return parser;
}

/* TODO: mk_app_parser_free() */
