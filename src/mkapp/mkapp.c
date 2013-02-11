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
 * Process management shell.
 */

#include <unistd.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "parser.h"
#include "mkapp_parser.h"
#include "module.h"

#define PACKAGE_NAME         "mkapp"
#define PACKAGE_VERSION      "0.1"
#define PACKAGE_PARAM_STRING "[FILE...] - start the mkapp shell"


GMainLoop*       m_main_loop = NULL; // Glib main loop
MkModuleContext* m_modules   = NULL; // Modules that make up the app
MkParserContext* m_parser    = NULL; // Text file parser


/*
 * Command-line options.
 */

gchar**  m_files    = NULL;  // Input files
gboolean m_version  = FALSE; // Obtain version information ?
gboolean m_verbose  = FALSE; // Be verbose ?
gchar*   m_commands = NULL;  // Commands from the command line

static GOptionEntry m_options[] = {
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
      (gpointer)&m_files, "Module definitions file", NULL },
    { "version", 'V', 0, G_OPTION_ARG_NONE,
      (gpointer)&m_version, "Print version information", NULL },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE,
      (gpointer)&m_verbose, "Be verbose", NULL },
    { "command", 'c', 0, G_OPTION_ARG_STRING,
      (gpointer)&m_commands, "Process commands from a string", NULL },
    { NULL }
};


/**
 * Do nothing.
 */
static void do_nothing() {}

/**
 * Main.
 */
int main(int argc, char* argv[])
{
    // Initialize the thread system. We don't use threads: this is a
    // workaround to make glib's g_child_watch_add more reliable when
    // SIGCHLD is caught between the moment when the main loop checks
    // all its sources and the moment when it poll()s its FDs.
    g_thread_init(NULL);

    // Make critical errors fatal to abort when they happen
    g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);

    // Read command-line arguments
    GError* error = NULL;
    GOptionContext* context;
    context = g_option_context_new(PACKAGE_PARAM_STRING);
    g_option_context_add_main_entries(context, m_options, NULL);
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);
    if (error != NULL)
        g_critical("%s", error->message);

    // If verbosity was not requested, block debug messages.
    if (!m_verbose)
        g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG,
                          (GLogFunc)do_nothing, NULL);

    // Print version info and exit?
    if (m_version) {
        g_printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        exit(EXIT_SUCCESS);
    }

    // Initialize the main loop, the module context and the parser
    m_main_loop = g_main_loop_new(NULL, TRUE);
    m_modules   = mk_module_context_new(m_main_loop);
    m_parser    = mk_app_parser_new(m_modules);
    mk_module_set_interpreter(m_modules,
                              (MkModuleInterpreter)mk_parser_parse_character,
                              m_parser);

    // Choose where to read commands from.
    if (m_commands != NULL) {
        // Parse commands from the command line?
        for (gsize i = 0; m_commands[i] != 0; ++i)
            mk_parser_parse_character(m_parser, m_commands[i]);

    } else if (m_files != NULL) {
        // Parse input files?
        for (gsize i = 0; m_files[i] != NULL; ++i)
            mk_parser_parse_file(m_parser, m_files[i]);

    } else {
        // Read standard input.
        GIOChannel* chan = g_io_channel_unix_new(STDIN_FILENO);
        g_io_channel_set_flags(chan, G_IO_FLAG_NONBLOCK, NULL);
        g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP,
                       (GIOFunc)mk_parser_parse_channel, m_parser);
    }

    // Start the main lo
    if (!mk_module_finished(m_modules)) {
        g_debug("Starting main loop...");
        g_main_loop_run(m_main_loop);
    }
}
