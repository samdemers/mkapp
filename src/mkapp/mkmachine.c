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
 * State-machine-like scripts.
 */

#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "parser.h"
#include "mkmachine_parser.h"
#include "transition.h"


#define PACKAGE_NAME         "mkmachine"
#define PACKAGE_VERSION      "0.1"
#define PACKAGE_PARAM_STRING "FILE - start a state machine"


GHashTable* m_transitions = NULL; // Transitions between states 
                                  // (key: source State, value: Transition)
gchar*           m_current_state; // Current state
GMainLoop*       m_main_loop;     // Main glib loop
MkParserContext* m_parser = NULL; // Text file parser


/*
 * Command-line options.
 */

gchar**  m_files;   // Input files
gboolean m_version; // Obtain version information ?

static GOptionEntry entries[] = {
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &m_files,
      "State machine definition file", NULL },
    { "version", 'V', 0, G_OPTION_ARG_NONE,
      (gpointer)&m_version, "Print version information", NULL },
    { NULL }
};


/**
 * Respond to input by performing the appropriate transition and printing
 * the corresponding output.
 * @param line input string
 */
void command(const gchar* line)
{
    if (m_current_state == NULL)
        return; // ERROR

    gchar* output;

    MkTransition* t = mk_transition_lookup(m_transitions, m_current_state,
                                           line, &output);

    if (t != NULL) {

        // Change state
        g_free(m_current_state);
        m_current_state = g_strdup(t->dst_state);

        // Print specified output, if any
        if (output != NULL)
            g_printf("%s\n", output);
        fflush(stdout);
    }

    g_free(output);
}


/**
 * Read and handle the standard input.
 * @param source    IO channel to read from
 * @param condition condition that happened
 * @param data      unused
 * @return          whether the source must still be watched
 */
gboolean read_input(GIOChannel*  source,
                    GIOCondition condition,
                    gpointer     data)
{
    while(1) {
        gchar* line;
        gsize  length;
        GError* error = NULL;
        
        GIOStatus status = g_io_channel_read_line(source, &line, &length,
                                                  NULL, &error);
        
        if (error != NULL)
            g_critical("Input/output error: %s.", error->message);
        
        switch(status) {
        case G_IO_STATUS_ERROR:
            g_free(line);
            g_critical("Input/output error.");
            return FALSE;
            
        case G_IO_STATUS_NORMAL:
            // Success
            command(line);
            break;
            
        case G_IO_STATUS_EOF:
            // End of file
            g_free(line);
            g_main_loop_quit(m_main_loop);
            return FALSE;
            
        case G_IO_STATUS_AGAIN:
            // Resource temporarily unavailable
            g_free(line);
            return TRUE;
        }
        
        g_free(line);
    }

    return TRUE;
}



/**
 * Execute a command based on its token list.
 * @param tokens the tokens that make up the command
 * @param length number of tokens
 */
void parse_file(const char** tokens, const gsize length)
{
    static gchar* src_state = NULL;
    static gchar* signal    = NULL;
    static gchar* output    = NULL;

    // The first bracket level defines the source state. The second one
    // defines possible responses to signals when in that state.
    if (src_state == NULL) {
        src_state = g_strdup(tokens[0]);
        for (gsize i = 1; i < length; ++i) {
            if (g_strcmp0(tokens[i], "{")) {
                g_fprintf(stderr, "Syntax error.\n");
            }
        }

    } else if (signal == NULL) {
        signal = g_strdup(tokens[0]);
        for (gsize i = 1; i < length; ++i) {
            if (g_strcmp0(tokens[i], "{")) {
                g_fprintf(stderr, "Syntax error.\n");
            }
        }

    } else {

        for (gsize i = 1; i < length; ++i) {
            if (!g_strcmp0(tokens[i], "}")) {
                if (signal != NULL) {
                    g_free(signal);
                    signal = NULL;
                    g_fprintf(stderr, "Output: \"%s\"\n", output);
                    g_free(output);
                    output = NULL;
                } else if (src_state != NULL) {
                    g_free(src_state);
                    src_state = NULL;
                } else {
                    g_fprintf(stderr, "Syntax error.\n");
                }
            } else {
                if (output == NULL)
                    output = g_strdup(tokens[i]);
                else
                    output = g_strconcat(output, " ", tokens[i], NULL);
            }
        }
    }

    g_fprintf(stderr, "src_state=%s, signal=%s, output=%s\n",
              src_state, signal, output);
}


/**
 * Main.
 */
int main(int argc, char* argv[])
{
    // Make critical errors fatal to abort when they happen
    g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);

    // Read command-line arguments
    GError* error = NULL;
    GOptionContext* context;
    context = g_option_context_new(PACKAGE_PARAM_STRING);
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);

    if (error != NULL)
        g_critical("%s", error->message);

    if (m_version) {
        g_printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        exit(EXIT_SUCCESS);
    }

    if (m_files == NULL || m_files[0] == NULL)
        g_critical("No input file");

    if (m_files[1] != NULL)
        g_critical("Too many input files.");

    // Parse the state machine definition
    m_transitions = g_hash_table_new_full
        (g_str_hash, g_str_equal, (GDestroyNotify)g_free,
         (GDestroyNotify)mk_transition_list_delete);
    m_parser = mk_machine_parser_new(m_transitions);
    mk_parser_parse_file(m_parser, m_files[0]);
    m_current_state = g_strdup(mk_machine_parser_get_default_state(m_parser));

    g_free(m_files);

    // Create an IO channel for stdin and start a main loop to handle it.
    GIOChannel* chan = g_io_channel_unix_new(STDIN_FILENO);
    g_io_channel_set_flags(chan, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP, read_input, NULL); 
    m_main_loop = g_main_loop_new(NULL, TRUE);
    g_main_loop_run(m_main_loop);
}
