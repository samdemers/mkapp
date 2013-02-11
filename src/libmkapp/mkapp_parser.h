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
 * This is the parser for mkapp files.
 *
 * The mkapp parser is very simple:
 *  - All commands must end with a ";".
 *  - Within a command, the first token represents the command name.
 *  - Whitespace-separated tokens following the command name are arguments.
 *  - Basic rules (comments, quoted strings, ...) are inherited from
 *    the default parser.
 *
 * A valid command is any command for which an mk_command_(command name)
 * function exists.
 */

#ifndef __MKAPP_PARSER_H__
#define __MKAPP_PARSER_H__

#include "module.h"
#include "parser.h"

/**
 * Create a new mkapp parser.
 * @param modules a module context to execute commands on
 * @return        a newly-allocated, properly configured, mkapp parser
 */
MkParserContext* mk_app_parser_new(MkModuleContext* modules);

#endif // __MKAPP_PARSER_H__
