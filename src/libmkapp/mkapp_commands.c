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
 * This file contains all mkapp commands.
 *
 * All functions are named after the following pattern:
 * mk_command_(command name).
 *
 * Functions defined in this file are meant to be used with GModule instead
 * of being referenced directly. This is why no header file is provided.
 */


#include <stdlib.h>
#include <glib.h>

#include "module.h"


#define COMMAND_MODULE_NOT_FOUND       "module not found"
#define COMMAND_MODULE_NOT_RUNNING     "module not running"
#define COMMAND_MODULE_ALREADY_RUNNING "module already running"
#define COMMAND_BINDING_EXISTS         "binding already exists"
#define COMMAND_BINDING_NOT_EXISTS     "no such binding"

#define COMMAND_DEFINE_USAGE       "usage: define module command [arg...]"
#define COMMAND_UNDEFINE_USAGE     "usage: undefine module"
#define COMMAND_BIND_USAGE         "usage: bind out_module in_module"
#define COMMAND_UNBIND_USAGE       "usage: unbind out_module in_module"
#define COMMAND_RUN_USAGE          "usage: run module"
#define COMMAND_KILL_USAGE         "usage: kill module"
#define COMMAND_WAIT_USAGE         "usage: wait module"
#define COMMAND_LISTEN_USAGE       "usage: listen module"
#define COMMAND_IGNORE_USAGE       "usage: ignore module"
#define COMMAND_EOF_USAGE          "usage: eof module"
#define COMMAND_WRITE_USAGE        "usage: write module string"
#define COMMAND_OBEY_USAGE         "usage: obey module"
#define COMMAND_DISOBEY_USAGE      "usage: disobey module"
#define COMMAND_EXIT_USAGE         "usage: exit [status]"


/**
 * Define a new module. The module will be initialized and added to the
 * module table. It will not run until command_run() is called.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_define(const gchar**    tokens,
                               const gsize      length,
                               MkModuleContext* modules)
{
    if (length < 3)
        return COMMAND_DEFINE_USAGE;

    const gchar*  name = tokens[1];
    const gchar** argv = &tokens[2];
    const gint    argc = length - 2;
    
    MkModule* module = mk_module_new(modules, name, argv[0]);
    g_assert(module != NULL);

    if (argc > 1)
        mk_module_append_args(module, argc-1, &argv[1]);

    MkModule* existing = mk_module_lookup(modules, name);
    if (existing) {
        g_debug("Module %s already exists => killing and removing",
                name);
        mk_module_kill(existing);
        mk_module_remove(modules, existing);
        g_debug("Done");
    }
    
    mk_module_add(modules, module);

    return NULL;    
}


/**
 * Undefine a module. The module will be removed from the module
 * table. If it is running, it will be stopped.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_undefine(const gchar**    tokens,
                                 const gsize      length,
                                 MkModuleContext* modules)
{
    if (length != 2)
        return COMMAND_UNDEFINE_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    mk_module_kill(module);
    mk_module_remove(modules, module);

    return NULL;    
}


/**
 * Bind a module's standard output to another module's standard
 * input. Any data written to the first module's stdout will be
 * automatically copied to the second module's stdin.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_bind(const gchar**    tokens,
                             const gsize      length,
                             MkModuleContext* modules)
{
    if (length != 3)
        return COMMAND_BIND_USAGE;

    const gchar* out_name = tokens[1];
    const gchar* in_name  = tokens[2];

    MkModule* out_module = mk_module_lookup(modules, out_name);
    MkModule* in_module  = mk_module_lookup(modules, in_name);

    if (out_module == NULL || in_module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    if (!mk_module_binding_exists(out_module, in_module))
        mk_module_bind(out_module, in_module);
    else
        return COMMAND_BINDING_EXISTS;

    return NULL;    
}


/**
 * Remove a binding installed with mk_command_bind().
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_unbind(const gchar**    tokens,
                               const gsize      length,
                               MkModuleContext* modules)
{
    if (length != 3)
        return COMMAND_UNBIND_USAGE;

    const gchar* out_name = tokens[1];
    const gchar* in_name  = tokens[2];

    MkModule* out_module = mk_module_lookup(modules, out_name);
    MkModule* in_module  = mk_module_lookup(modules, in_name);

    if (out_module == NULL || in_module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    if (mk_module_binding_exists(out_module, in_module))
        mk_module_unbind(out_module, in_module);
    else
        return COMMAND_BINDING_NOT_EXISTS;

    return NULL;    
}


/**
 * Run a module defined with mk_command_define().
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_run(const gchar**    tokens,
                            const gsize      length,
                            MkModuleContext* modules)
{
    if (length != 2)
        return COMMAND_RUN_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    if (!mk_module_is_running(module))
        mk_module_run(module);
    else
        return COMMAND_MODULE_ALREADY_RUNNING;

    return NULL;    
}


/**
 * Stop a module previously started with mk_command_run(). If the module
 * is not running, this command has no effect.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_kill(const gchar**    tokens,
                             const gsize      length,
                             MkModuleContext* modules)
{
    if (length != 2)
        return COMMAND_KILL_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    if (mk_module_is_running(module))
        mk_module_kill(module);
    else
        return COMMAND_MODULE_NOT_RUNNING;

    return NULL;    
}


/**
 * Wait until a module has exited.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_wait(const gchar**    tokens,
                             const gsize      length,
                             MkModuleContext* modules)
{
    if (length != 2)
        return COMMAND_WAIT_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    if (mk_module_is_running(module))
        mk_module_wait(module);
    else
        return COMMAND_MODULE_NOT_RUNNING;

    return NULL;
}


/**
 * Listen to a module's standard output.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_listen(const gchar**    tokens,
                               const gsize      length,
                               MkModuleContext* modules)
{
    if (length != 2)
        return COMMAND_LISTEN_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    mk_module_listen(module);

    return NULL;
}


/**
 * Stop listening to a module's standard output.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_ignore(const gchar**    tokens,
                               const gsize      length,
                               MkModuleContext* modules)
{
    if (length != 2)
        return COMMAND_IGNORE_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    mk_module_ignore(module);

    return NULL;
}


/**
 * Send end-of-file to a module.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_eof(const gchar**    tokens,
                            const gsize      length,
                            MkModuleContext* modules)
{
    if (length != 2)
        return COMMAND_EOF_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    if (!mk_module_is_running(module))
        return COMMAND_MODULE_NOT_RUNNING;

    mk_module_eof(module);
    
    return NULL;
}


/**
 * Write to a module. All the tokens are space-separated and a newline is
 * appended.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_write(const gchar**    tokens,
                              const gsize      length,
                              MkModuleContext* modules)
{
    if (length < 3)
        return COMMAND_WRITE_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    if (!mk_module_is_running(module))
        return COMMAND_MODULE_NOT_RUNNING;

    for(gsize i = 2; i < length; ++i) {
        mk_module_write(module, tokens[i], -1);
        mk_module_write(module, " ", -1);
    }
    mk_module_write(module, "\n", -1);

    return NULL;
}


/**
 * Interpret a module's output as commands and execute them.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_obey(const gchar**    tokens,
                             const gsize      length,
                             MkModuleContext* modules)
{
    if (length != 2)
        return COMMAND_OBEY_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    mk_module_obey(module);

    return NULL;
}


/**
 * Stop interpreting commands from a module.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_disobey(const gchar**    tokens,
                                const gsize      length,
                                MkModuleContext* modules)
{
    if (length != 2)
        return COMMAND_DISOBEY_USAGE;

    const gchar* name = tokens[1];

    MkModule* module = mk_module_lookup(modules, name);
    if (module == NULL)
        return COMMAND_MODULE_NOT_FOUND;

    mk_module_disobey(module);

    return NULL;
}


/**
 * Exit.
 * @param tokens  the tokens that make up the command
 * @param length  number of tokens
 * @param modules module running context
 * @return        error string if any, or NULL
 */
const gchar* mk_command_exit(const gchar**    tokens,
                             const gsize      length,
                             MkModuleContext* modules)
{
    if (length == 1) {
        exit(EXIT_SUCCESS);

    } else if (length == 2) {
        const gchar* s_status = tokens[1];
        gint64 status = g_ascii_strtoll(s_status, NULL, 10);
        exit((int)status);
 
    } else {
        return COMMAND_EXIT_USAGE;
    }


    return NULL;
}
