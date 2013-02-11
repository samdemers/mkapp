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
 * MkModule execution functions.
 *
 * A module is any program running in its own process and communicating
 * with other programs througn its standard input and output.
 */

#ifndef __MODULE_H__
#define __MODULE_H__

#include <glib.h>


/**
 * MkModule interpreter function type, called when a module's obey flag is set
 * to true, to interpred its output as commands.
 * @param data arbitrary pointer passed to mk_module_set_interpreter()
 * @param c    character received
 */
typedef void(*MkModuleInterpreter)(void* data, const char c);


/**
 * A module context is an environment within which to run modules. Modules
 * inside the same context can be connected together and their name must
 * be unique within that context.
 *
 * If a main loop is associated to the context, g_main_loop_quit() will
 * be called when all modules have finished running and end of file has
 * been reached.
 *
 * If an interpreter function is provided, it will be called to handle
 * all the characters received from a module that has its obey flag on.
 *
 * @brief MkModule running context.
 */
typedef struct {
    GHashTable*         modules;          /// All modules (key=name)
    gboolean            eof_received;     /// Was EOF received?
    gint                n_running;        /// Number of modules running
    GMainLoop*          loop;             /// Program's main loop
    MkModuleInterpreter interpreter;      /// MkModule command interpreter
    void*               interpreter_data; /// Data for interpreter
} MkModuleContext;


/**
 * A module is any executable file launched within its own process. Modules
 * must have a unique name and can have their standard inputs and output
 * connected freely.
 *
 * @brief Command launched in its own process.
 */
typedef struct {
    MkModuleContext* context;   /// Context the module belongs to
    gchar*           name;      /// Unique module name
    GPtrArray*       listeners; /// MkModules interested in a module's output
    GPid             pid;       /// Process ID
    GPtrArray*       args;      /// Executable file and arguments
    GIOChannel*      in;        /// Standard input
    GIOChannel*      out;       /// Standard output
    GIOChannel*      err;       /// Standard error
    guint            source;    /// Glib event source
    gint             writers;   /// Number of modules this one listens to
    gboolean         listen;    /// Are we listening to this module's output?
    gboolean         zombie;    /// Is this module supposed to be dead?
    gboolean         obey;      /// Shall we obey this module?
} MkModule;


/**
 * Create a new module running context.
 * @param loop program's main loop to quit after end-of-file, or NULL
 * @return     a newly-allocated module context
 */
MkModuleContext* mk_module_context_new(GMainLoop* loop);


/**
 * Free a module running context created with module_context_new().
 * @param mc context
 */
void mk_module_context_free(MkModuleContext* mc);


/**
 * Configure the function to call to parse the output of a module that has
 * its "obey" flag on.
 * @param mc          module context
 * @param interpreter parsing function
 * @param data        data for interpreter
 */
void mk_module_set_interpreter(MkModuleContext*    mc,
                               MkModuleInterpreter interpreter,
                               void*               data);


/**
 * Find a module within the context's module table.
 * @param mc   module context
 * @param name module name
 * @return     module found, or NULL if not found
 */
MkModule* mk_module_lookup(MkModuleContext* mc, const gchar* name);


/**
 * Add a module to a context's module table.
 * @param mc     module context
 * @param module module to add
 */
void mk_module_add(MkModuleContext* mc, MkModule* module);


/**
 * Remove a module from a context's module table.
 * @param mc     module context
 * @param module module to remove
 */
void mk_module_remove(MkModuleContext* mc, MkModule* module);


/**
 * Create a new MkModule.
 * @param name Unique module name
 * @param cmd  Executable file that will run in the module's process.
 * @param mc   Context the module will exist in
 * @return     A new MkModule that must be freed by module_delete().
 */
MkModule* mk_module_new(MkModuleContext* mc,
                        const gchar*     name,
                        const gchar*     cmd);

/**
 * Delete a MkModule.
 * @param module the module
 */
void mk_module_delete(MkModule* module);

/**
 * Test the existence of a binding between two modules.
 * @param out_module module that would provide the output
 * @param in_module  module that would listen to out_module's output
 * @return           whether out_module's stdout is bound to in_module's stdin
 */
gboolean mk_module_binding_exists(MkModule* out_module, MkModule* in_module);

/**
 * Bind a module's standard output to another module's standard input.
 * @param out_module module that will provide the output
 * @param in_module  module that will listen to out_module's output
 */
void mk_module_bind(MkModule* out_module, MkModule* in_module);

/**
 * Remove the binding between a module's output and another's input.
 * @param out_module module that will provide the output
 * @param in_module  module that will listen to out_module's output
 */
void mk_module_unbind(MkModule* out_module, MkModule* in_module);

/**
 * Append arguments to a module's argument list.
 * @param module the module
 * @param argc   argument count
 * @param argv   arguments
 */
void mk_module_append_args(MkModule* module,
                           const gsize argc,
                           const gchar** argv);

/**
 * Write data to a module's standard input.
 * @param module module to write to
 * @param data   what to write
 * @param length number of data bytes
 */
void mk_module_write(MkModule* module, const gchar* data, const gsize length);

/**
 * Write data to all the listeners of a module.
 * @param data   the data
 * @param length size of data, or -1 if data is a nul-terminated string
 * @param module module the data is coming from
 */
void mk_module_write_to_listeners(MkModule*    module,
                                  const gchar* data,
                                  const gsize  length);

/**
 * Cleanup after a module has exited.
 * @param pid    process ID
 * @param status exit status
 * @param module the module itself
 */
void mk_module_on_exit(GPid      pid,
                       gint      status,
                       MkModule* module);

/**
 * Forward a module's standard output to the standard input of all its
 * listeners.
 * @param source    IO channel to read from
 * @param module    module to read from
 * @return          whether the source must still be watched
 */
gboolean mk_module_forward_out(GIOChannel*  source,
                               GIOCondition unused,
                               MkModule*    module);

/**
 * Forward a module's standard error to the console. The module name
 * is added to the beginning of each line.
 * @param source    IO channel to read from
 * @param module    module to read from
 * @return          whether the source must still be watched
 */
gboolean mk_module_forward_err(GIOChannel*  source,
                               GIOCondition unused,
                               MkModule*    module);

/**
 * Launch a module. A new process is spawned and the module's command
 * is executed. Redirections are set up to forward the module's output
 * to its listeners.
 * @param module the module
 */
void mk_module_run(MkModule* module);

/**
 * Kill a module's process.
 * @param module the module
 */
void mk_module_kill(MkModule* module);

/**
 * Wait for a module to exit.
 * @param module the module
 */
void mk_module_wait(MkModule* module);

/**
 * Check whether a module is running.
 * @param  module the module
 * @return        whether module is running
 */
gboolean mk_module_is_running(MkModule* module);

/**
 * Start listening to a module.
 * @param module the module
 */
void mk_module_listen(MkModule* module);

/**
 * Stop listening to a module.
 * @param module the module
 */
void mk_module_ignore(MkModule* module);

/**
 * Tell the system that EOF has been received and that the main loop
 * must quit as soon as all the modules have finished running.
 * @param mc module context
 * @return   TRUE if no module is running anymore
 */
gboolean mk_module_eof_received(MkModuleContext* mc);

/**
 * Check whether the module context has finished executing. Execution
 * is finished when EOF was received and all modules have exited.
 * @param mc module context
 * @return   whether execution has ended
 */
gboolean mk_module_finished(MkModuleContext* mc);

/**
 * Close a module's standard input (end-of-file).
 * @param module the module
 */
void mk_module_eof(MkModule* module);

/**
 * Start obeying a module. All its output will be interpreted using the
 * interpreter set using module_set_interpreter.
 * @param module the module to start obeying
 */
void mk_module_obey(MkModule* module);

/**
 * Stop obeying a module.
 * @param module the module to stop obeying
 */
void mk_module_disobey(MkModule* module);

#endif // __MODULE_H__
