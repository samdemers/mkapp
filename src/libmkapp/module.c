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
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "module.h"


#define BUFFER_LENGTH 2048

MkModuleContext* mk_module_context_new(GMainLoop* loop)
{
    MkModuleContext* mc = g_malloc(sizeof(MkModuleContext));

    mc->modules = g_hash_table_new_full(g_str_hash, g_str_equal,
                                        (GDestroyNotify)g_free,
                                        (GDestroyNotify)mk_module_delete);

    mc->eof_received = FALSE;
    mc->n_running    = 0;
    mc->loop         = loop;

    return mc;
}


void mk_module_context_free(MkModuleContext* mc)
{
    g_hash_table_unref(mc->modules);
    g_free(mc);

}


void mk_module_set_interpreter(MkModuleContext*    mc,
                               MkModuleInterpreter interpreter,
                               void*               data)
{
    mc->interpreter      = interpreter;
    mc->interpreter_data = data;    
}


MkModule* mk_module_lookup(MkModuleContext* mc, const gchar* name)
{
    return g_hash_table_lookup(mc->modules, name);
}


void mk_module_add(MkModuleContext* mc, MkModule* module)
{
    g_hash_table_insert(mc->modules, g_strdup(module->name), module);
}


void mk_module_remove(MkModuleContext* mc, MkModule* module)
{
    g_hash_table_remove(mc->modules, module->name);
}


void ptr_array_free_strings(GPtrArray* array)
{
    int i;
    for(i = 0; i < array->len; ++i)
        g_free(g_ptr_array_index(array, i));
    g_ptr_array_free(array, TRUE);
}


MkModule* mk_module_new(MkModuleContext* mc, const gchar* name, const gchar* cmd)
{
    // Allocate resources
    MkModule* module    = g_malloc(sizeof(MkModule));
    module->context   = mc;
    module->name      = g_strdup(name);
    module->listeners = g_ptr_array_new();
    module->pid       = -1;
    module->args      = g_ptr_array_new();
    module->writers   = 0;
    module->listen    = FALSE;
    module->zombie    = FALSE;
    module->obey      = FALSE;

    // Initialize the null-terminated argument list with argv[0]
    gchar* arg0 = g_strdup(cmd);
    g_ptr_array_add(module->args, arg0);
    g_ptr_array_add(module->args, NULL);
    return module;
}


void mk_module_delete(MkModule* module)
{
    g_debug("Deleting module %s\n", module->name);

    // If the module is still running, wait until it is dead and declare that
    // it scheduled for deletion. mk_module_delete() will have to be called
    // again when the module is really dead (see mk_module_on_exit()).
    if (module->pid > 0) {
        module->zombie = TRUE;
    } else {
        // Unbind the module from its listeners
        for (guint i = 0; i < module->listeners->len; ++i) {
            MkModule* dest_module = g_ptr_array_index(module->listeners, i);
            mk_module_unbind(module, dest_module);
        }

        // TODO: unbind from writers

        g_free(module->name);
        g_ptr_array_free(module->listeners, FALSE);
        ptr_array_free_strings(module->args);
        g_free(module);
    }
}


gboolean mk_module_binding_exists(MkModule* out_module, MkModule* in_module)
{
    for (gsize i = 0; i < out_module->listeners->len; ++i) {
        MkModule* listener = g_ptr_array_index(out_module->listeners, i);
        if (listener == in_module)
            return TRUE;
    }    

    return FALSE;
}


void mk_module_bind(MkModule* out_module, MkModule* in_module)
{
    if (!mk_module_binding_exists(out_module, in_module)) {
        g_ptr_array_add(out_module->listeners, in_module);
        ++(in_module->writers);
    }
}


void mk_module_unbind(MkModule* out_module, MkModule* in_module)
{
    if (mk_module_binding_exists(out_module, in_module)) {
        g_ptr_array_remove(out_module->listeners, in_module);
        --(in_module->writers);
    }
}


void mk_module_append_args(MkModule* module, const gsize argc, const gchar** argv)
{
    // Remove the NULL element, add the new elements and put back the NULL
    g_ptr_array_remove_index(module->args, module->args->len-1);
    for (gsize i = 0; i < argc; ++i) {
        g_ptr_array_add(module->args, g_strdup(argv[i]));
    }
    g_ptr_array_add(module->args, NULL);
}


void mk_module_write(MkModule* module, const gchar* data, const gsize length)
{
    GError* error = NULL;
    
    // If the module is not running, do not try to write 
    if (!mk_module_is_running(module)) {
        g_warning("Could not write to %s: module not running\n",
                  module->name);
        return;
    }

    // Do not write if the module's input is not writeable. This can happen
    // the channel was shut down after a call to mk_module_eof() but the
    // process did notexit yet.
    if (!module->in) {
        g_warning("Could not write to %s: module not writeable\n",
                  module->name);
        return;
    }

    // Write data
    gsize written;
    g_io_channel_write_chars(module->in, data, length, &written, &error);
    if (error != NULL) {
        g_critical("Error writing to %s: %s", module->name, error->message);
        return;
    }

    // Flush
    g_io_channel_flush(module->in, &error);
    if (error != NULL) {
        g_critical("Error writing to %s: %s", module->name, error->message);
        return;
    }

}


void mk_module_write_to_listeners(MkModule*    module,
                                  const gchar* data, 
                                  const gsize  length)
{
    if (length == 0)
        return;

    // Send a copy of data to all the listeners
    for (guint i = 0; i < module->listeners->len; ++i) {

        MkModule* dest_module = g_ptr_array_index(module->listeners, i);

        if (dest_module != NULL)
            mk_module_write(dest_module, data, length);
    }

    // Write to our own standard output if listening has been requested
    if (module->listen) {
        g_printf("%s", data);
        fflush(stdout);
    }

    // Interpret the commands if obedience has been requested
    if (module->obey && module->context->interpreter != NULL) {
        gsize len = length;
        if (len < 0)
            len = strlen(data);
            
        for(gsize i = 0; i < len; ++i)
            module->context->interpreter
                (module->context->interpreter_data, data[i]);
    }
}


void mk_module_on_exit(GPid pid, gint status, MkModule* module)
{
    g_debug("MkModule %s exited with status %d.", module->name, status>>8);

    MkModuleContext* mc = module->context;

    // Write any remaining data from the module to its listeners
    mk_module_forward_out(module->out, 0, module);
    mk_module_forward_err(module->err, 0, module);

    // Remove the watch on stdout and stderr and shut down IO channels.
    // The module's standard input could be already closed (see
    // mk_module_eof() and mk_module_kill()).
    while (g_source_remove_by_user_data(module));

    if (module->in) {
        g_io_channel_shutdown(module->in, TRUE, NULL);
        g_io_channel_unref(module->in);
    }

    g_io_channel_shutdown(module->out, TRUE, NULL);
    g_io_channel_shutdown(module->err, TRUE, NULL);
    g_io_channel_unref(module->out);
    g_io_channel_unref(module->err);

    // Close the pid (does nothing under UNIX)
    g_spawn_close_pid(pid);

    // Delete the module if mk_module_delete has already been called
    // on it while it was running.
    module->pid = -1;
    if (module->zombie)
        mk_module_delete(module);

    // Quit if execution is finished and a main loop has been provided
    --(mc->n_running);
    g_debug("MkModules running: %d", mc->n_running);
    if (mk_module_finished(mc) && mc->loop)
        g_main_loop_quit(mc->loop);

}


gboolean mk_module_forward_out(GIOChannel*  source,
                               GIOCondition unused,
                               MkModule*    module)
{
    gchar     buf[BUFFER_LENGTH];
    gsize     length;
    GError*   error = NULL;
    GIOStatus status;

    // Read data
    status = g_io_channel_read_chars(source, buf, BUFFER_LENGTH-1,
                                     &length, &error);


    // Check for errors
    if (error != NULL)
        g_critical("Error reading from %s: %s", module->name, error->message);

    switch (status) {
    case G_IO_STATUS_ERROR:
        // An error occurred.
        g_critical("Error reading from %s", module->name);
        break;

    case G_IO_STATUS_NORMAL:
        buf[length] = '\0';
        mk_module_write_to_listeners(module, buf, length);
        break;

    case G_IO_STATUS_EOF:
        // End of file.
        return FALSE;

    case G_IO_STATUS_AGAIN:
        // Resource temporarily unavailable.
        return TRUE;
    }

    // Return TRUE to keep the source open
    return TRUE;
}


gboolean mk_module_forward_err(GIOChannel*  source,
                               GIOCondition unused,
                               MkModule*    module)
{
    while (1) {
        gchar*    line;
        gsize     length;
        GIOStatus status;
        GError*   error = NULL;

        status = g_io_channel_read_line(source, &line, &length, NULL, &error);
        
        if (error != NULL)
            g_critical("Error forwarding standard error: %s", error->message);
        
        switch (status) {
        case G_IO_STATUS_ERROR:
            // An error occurred.
            g_critical("Error reading from %s", module->name);
            break;
            
        case G_IO_STATUS_NORMAL:
            g_fprintf(stderr, "%s: %s", module->name, line);
            break;
            
        case G_IO_STATUS_EOF:
            // End of file.
            return FALSE;
            
        case G_IO_STATUS_AGAIN:
            // Resource temporarily unavailable.
            return TRUE;
        }
        
        g_free(line);

    }

    return TRUE;    
}


void mk_module_run(MkModule* module)
{
    GError* error = NULL;
    gint in_fd, out_fd, err_fd;

    if (module->pid > 0) {
        g_debug("MkModule %s already running.", module->name);
        return;
    }

    g_debug("Starting module %s...", module->name);

    // Spawn the process
    g_spawn_async_with_pipes(NULL,
                             (gchar**)(module->args->pdata),
                             NULL,
                             G_SPAWN_SEARCH_PATH
                             | G_SPAWN_DO_NOT_REAP_CHILD,
                             NULL,
                             NULL,
                             &(module->pid),
                             &in_fd,
                             &out_fd,
                             &err_fd,
                             &error);
        
    if (error != NULL) {
        g_warning("Could not run %s: %s", module->name, error->message);
        return;
    }
        
    // Connect file descriptors. Output and error must be non-blocking.
    // Otherwise, g_io_channel_read_chars will block trying to fill the
    // buffer completely, which is kind of a strange and unpleasant
    // behavior.
    module->in = g_io_channel_unix_new(in_fd);
    module->out = g_io_channel_unix_new(out_fd);
    module->err = g_io_channel_unix_new(err_fd);

    g_io_channel_set_flags(module->out, G_IO_FLAG_NONBLOCK, NULL);
    g_io_channel_set_flags(module->err, G_IO_FLAG_NONBLOCK, NULL);

    // Start forwarding stdout to listeners and stderr to stderr
    g_io_add_watch(module->out, G_IO_IN | G_IO_ERR | G_IO_HUP,
                   (GIOFunc)mk_module_forward_out, module);
    g_io_add_watch(module->err, G_IO_IN | G_IO_ERR | G_IO_HUP,
                   (GIOFunc)mk_module_forward_err, module);

    // Cleanup when the child exits. Glib catches SIGCHLD to know when
    // to call the child watch function. If the process has already exited,
    // though, we must react by ourselves.
    module->source = g_child_watch_add(module->pid,
                                       (GChildWatchFunc)mk_module_on_exit,
                                       module);
    ++module->context->n_running;
    g_debug("MkModules running: %d", module->context->n_running);

    gint status;
    if (waitpid(module->pid, &status, WNOHANG) > 0) {
        mk_module_on_exit(module->pid, status, module);
    }

}


void mk_module_kill(MkModule* module)
{
    if (module->pid > 0) {
        // Kill the process
        if (kill(module->pid, SIGTERM))
            g_warning("Could not kill child process: %s", g_strerror(errno));

        // Close stdin to stop writing to the module
        g_io_channel_shutdown(module->in, TRUE, NULL);
        g_io_channel_unref(module->in);
        module->in = NULL;
    }
}


void mk_module_wait(MkModule* module)
{
    // If the module is running, wait until it exits. Then, call
    // mk_module_on_exit() manually: since we called waitpid() ourselves,
    // glib won't take care of it anymore.
    if (module->pid > 0) {
        int status;
        pid_t pid = waitpid(module->pid, &status, 0);
        mk_module_on_exit(pid, status, module);
        g_source_remove(module->source);
    }
}


gboolean mk_module_is_running(MkModule* module)
{
    return module->pid > 0;
}


void mk_module_listen(MkModule* module)
{
    module->listen = TRUE;
}


void mk_module_ignore(MkModule* module)
{
    module->listen = FALSE;
}


gboolean mk_module_eof_received(MkModuleContext* mc)
{
    mc->eof_received = TRUE;

    g_debug("End of file. MkModules running: %d.", mc->n_running);

    if (mc->n_running == 0) {
        if (mc->loop)
            g_main_loop_quit(mc->loop);
        return FALSE;
    }

    return TRUE;
}


gboolean mk_module_finished(MkModuleContext* mc)
{
    return (mc->n_running == 0 && mc->eof_received);
}


void mk_module_eof(MkModule* module)
{
    GError* error = NULL;

    g_io_channel_shutdown(module->in, TRUE, &error);
    g_io_channel_unref(module->in);
    module->in = NULL;

    if (error != NULL)
        g_warning("Could not close %s's standard input: %s",
                  module->name, error->message);
}


void mk_module_obey(MkModule* module)
{
    module->obey = TRUE;
}


void mk_module_disobey(MkModule* module)
{
    module->obey = FALSE;
}
