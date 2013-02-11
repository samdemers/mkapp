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
 * This is the parser for mkmachine files.
 *
 * The mkmachine parser recognizes the following format:
 * 
 * SOURCE_STATE {
 *    INPUT_REGEX [=> DESTINATION_STATE] { OUTPUT }
 * }
 * ...
 *
 * The parser builds a transition hash table that can be used to
 * implement a state machine.
 */

#include <glib.h>


/**
 * Get the name of the default state of the state machine. The default state
 * is the first state defined. A file containing states must have been parsed
 * for this information to be known.
 * @param parser the parser
 * @return       name of the first state parsed
 */
const gchar* mk_machine_parser_get_default_state(MkParserContext* parser);


/**
 * Create a new mkmachine state machine file parser.
 * @param transitions   transition table that will be populated.
 * @return              a newly-allocated state machine parser
 */
MkParserContext* mk_machine_parser_new(GHashTable* transitions);

