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
 * Generic text parser.
 *  - The parser maintains a parsing array in which indices are numbers of
 *    ASCII characters and values are callback functions that must be
 *    called whenever the character is encountered.
 *  - Callback functions are passed the parser structure as a first argument.
 *  - Callback functions can push the current parsing array and create a new
 *    one, or pop the current parsing array to restore the previous one.
 *  - Utility functions are provided to handle the most common parsing
 *    features, such as comments and token strings.
 */


#ifndef __PARSER_H__
#define __PARSER_H__

#include <glib.h>

#define MK_PARSER_MAX_DEPTH   8
#define MK_PARSER_FIRST_CHAR -1
#define MK_PARSER_LAST_CHAR   127
#define MK_PARSER_ARRAY_SIZE  129
#define MK_PARSER_NON_ASCII  -1


struct MkParserContext;

/**
 * Parser callback function type. A function of this kind is called
 * for every character that must be parsed.
 * @param parser    the parser
 * @param c         character to parse
 * @param user_data arbitrary data configured with user_data
 */
typedef void(*MkParserFunc)(struct MkParserContext* parser,
                            const gchar             c,
                            void*                   user_data);

/**
 * This structure maintains the current state of a text buffer
 * currently being parsed by calls to mk_parser_parse_character().
 * @brief Parser state.
 */
typedef struct MkParserContext {
    MkParserFunc f[MK_PARSER_MAX_DEPTH][MK_PARSER_ARRAY_SIZE]; /// Function map
    MkParserFunc eof_func;      /// Function to call when EOF is received
    gsize        depth;         /// Current depth of the f stack
    void*        user_data;     /// User data to pass to f's functions
    GPtrArray*   tokens;        /// Token array
    GString*     current_token; /// Token being built
} MkParserContext;

/**
 * Create a new parser state object for use with mk_parser_parse_character().
 * @param user_data pointer that will be handed to any callback function
 *                  configured with mk_parser_configure()
 * @return          the new parser
 */
MkParserContext* mk_parser_new(void* user_data);


/**
 * Free a parser state object created with mk_parser_new().
 * @param parser the parser
 */
void mk_parser_free(MkParserContext* parser);


/**
 * Set the callback function for end of file.
 * @param parser the parser
 * @param f      function to call when EOF is received
 */
void mk_parser_set_eof_func(MkParserContext* parser, MkParserFunc f);


/**
 * Push a parser's stack. A new copy of the current callback function
 * table will be made and will be the one used until mk_parser_pop() is
 * called, increasing the parser's stack depth by one.  The parser's
 * stack cannot have a depth of more than MK_PARSER_MAX_DEPTH.
 * @param parser the parser
 */
void mk_parser_push(MkParserContext* parser);


/**
 * Pop a parser's stack, restoring the callback function table it had
 * before calling mk_parser_push(). This function must be called exactly
 * once for each mk_parser_push() call.
 * @param parser the parser
 */
void mk_parser_pop(MkParserContext* parser);


/**
 * Configure the parser to make it call a function every time character 'c'
 * is encountered.
 * @param parser parser to configure
 * @param c      character that will trigger the function call
 * @param f      function to call
 */
void mk_parser_configure(MkParserContext* parser,
                         const gchar      c,
                         MkParserFunc     f);


/**
 * Configure the parser to make it call a function every time character a
 * character between c1 and c2 is encountered.
 * @param parser parser to configure
 * @param c1     first character of the range
 * @param c2     second character of the range
 * @param f      function to call
 */
void mk_parser_configure_range(MkParserContext* parser,
                               const gchar      c1,
                               const gchar      c2,
                               MkParserFunc     f);


/**
 * Configure the parser to make it call a function every time character a
 * character that is in chars is encountered.
 * @param parser parser to configure
 * @param chars a string that contains all the characters to configure
 * @param f     function to call
 */
void mk_parser_configure_all(MkParserContext* parser,
                             const gchar*     chars,
                             MkParserFunc     f);


/**
 * Configure the parser to call function f whenever any character is
 * received.
 * @param parser parser to configure
 * @param f      function to call
 */
void mk_parser_configure_default(MkParserContext* parser, MkParserFunc f);


/**
 * Parse a single character and update parser state accordingly. When
 * one of the parser's delimiter characters is seen, the parser's
 * callback function is called with the current token list as
 * argument. Then, the token list is reset.
 * @param parser object that contains parser state
 * @param c      the new character
 */
void mk_parser_parse_character(MkParserContext* parser, const gchar c);


/**
 * Append a character to a parser's current token.
 * @param parser the parser
 * @param c      character to append
 */
void mk_parser_token_append(MkParserContext* parser, gchar c);


/**
 * Put the token currently being parsed into the token list and start
 * putting new characters into the next token.
 * @param parser the parser
 */
void mk_parser_token_cut(MkParserContext* parser);


/**
 * Add a token to a parser's token array.
 * @param parser the parser
 * @param token  token to add
 */
void mk_parser_token_add(MkParserContext* parser, const gchar* token);


/**
 * Clear a parser's token list except the current token.
 * @param parser the parser
 */
void mk_parser_token_clear(MkParserContext* parser);


/**
 * Get an array containing all the tokens except the current token.
 * @param parser the parser
 * @return       all the tokens in the token list
 */
const gchar** mk_parser_token_get(MkParserContext* parser);


/**
 * Get the size of the token array.
 * @param parser the parser
 * @return       number of tokens in the array
 */
gsize mk_parser_token_size(MkParserContext* parser);


/**
 * Handle everything until the next double-quote (") as raw token data
 * and add it to the current token.
 * @param parser the parser
 * @param c      character to parse
 * @param data   arbitrary data configured with mk_parser_new()
 */
void mk_parser_dquote_begin(MkParserContext* parser, gchar c, void* data);


/**
 * Handle everything until the next single-quote (') as raw token data
 * and add it to the current token.
 * @param parser the parser
 * @param c      character to parse
 * @param user   arbitrary data configured with mk_parser_new()
 */
void mk_parser_squote_begin(MkParserContext* parser, gchar c, void* data);


/**
 * Stop handling text as a comment, usually because a newline has been
 * encountered, and add a newline to the current token.
 * @param parser the parser
 * @param c      character to parse
 * @param data   arbitrary data configured with mk_parser_new()
 */
void mk_parser_comment_end(MkParserContext* parser, gchar c, void* data);


/**
 * Stop parsing anything until the next newline.
 * @param parser the parser
 * @param c      character to parse
 * @param data   arbitrary data configured with mk_parser_new()
 */
void mk_parser_comment_begin(MkParserContext* parser, gchar c, void* data);


/**
 * Add two '\' before the character unless it is a quoted string
 * delimiter (' or "). Append the result to the current token.
 * @param parser the parser
 * @param c      character to parse
 * @param data   arbitrary data configured with mk_parser_new()
 */
void mk_parser_strict_escape_end(MkParserContext* parser, gchar c, void* data);


/**
 * Disable usual parsing and handle the next character with
 * mk_parser_strict_escape_end().
 * @param parser the parser
 * @param c      character to parse
 * @param data   arbitrary data configured with mk_parser_new()
 */
void mk_parser_strict_escape_begin(MkParserContext* parser,
                                   gchar c, void* data);


/**
 * Add a '\' before the character unless it is a quoted string
 * delimiter (' or "). Append the result to the current token.
 * @param parser the parser
 * @param c      character to parse
 * @param data   arbitrary data configured with mk_parser_new()
 */
void mk_parser_escape_end(MkParserContext* parser, gchar c, void* data);


/**
 * Disable usual parsing and handle the next character with
 * mk_parser_escape_end().
 * @param parser the parser
 * @param c      character to parse
 * @param data   arbitrary data configured with mk_parser_new()
 */
void mk_parser_escape_begin(MkParserContext* parser, gchar c, void* data);


/**
 * Enable default behavior for a parser. Quoted strings ('") will be
 * added to the current token unparsed and single-line comments (#)
 * will be supported.
 * @param parser the parser
 */
void mk_parser_enable_defaults(MkParserContext* parser);


/**
 * Read an IO channel and parse every character until EOF.
 * @param source IO channel to read from
 * @param parser the parser
 * @return       whether the source must still be watched
 */
gboolean mk_parser_parse_channel(GIOChannel*      source,
                                 GIOCondition     unused,
                                 MkParserContext* parser);


/**
 * Read an input file and parse it.
 * @param parser   the parser
 * @param filename file to read
 */
void mk_parser_parse_file(MkParserContext* parser, const gchar* filename);

#endif // __PARSER_H__
