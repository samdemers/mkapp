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
 * Data store manager.
 */

#include <unistd.h>
#include <stdlib.h>

#include <glib/gprintf.h>
#include <glib.h>

#include "store_node.h"
#include "store_key_value.h"

#define PACKAGE_NAME         "mkstore"
#define PACKAGE_VERSION      "0.1"
#define PACKAGE_PARAM_STRING "[FILE]"


GNode*     m_tree = NULL;      // Data
GMainLoop* m_main_loop = NULL; // Glib main loop


/*
 * Command-line options.
 */

gchar**  m_files       = NULL;  // Input files
gboolean m_read_only   = FALSE; // Don't update the input file
gboolean m_auto_save   = FALSE; // Auto-save the file
gchar*   m_translation = NULL;  // Translation mode expression
gboolean m_echo        = FALSE; // Echo mode?
gboolean m_version     = FALSE; // Obtain version information?

static GOptionEntry entries[] = {
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &m_files,
      "Data file", NULL },
    { "version", 'V', 0, G_OPTION_ARG_NONE,
      (gpointer)&m_version, "Print version information", NULL },

    { "read-only", 'r', 0, G_OPTION_ARG_NONE, &m_read_only,
      "Don't update the file", NULL },
    { "auto-save", 'a', 0, G_OPTION_ARG_NONE, &m_auto_save,
      "Automatically save the file during edition", NULL },
    { "echo", 'e', 0, G_OPTION_ARG_NONE, &m_echo,
      "Echo mode: echo the value of the variable after each line", NULL },

    { "translate", 't', 0, G_OPTION_ARG_STRING, &m_translation,
      "Translate inputs using a regular expression", NULL },
    { NULL }
};


/**
 * Do variable substitution for a match of the translation regular
 * expression inside an input string.
 * @param match_indo the GMatchInfo generated by the match
 * @param result     a GString containing the new string
 * @param matches    whether the variable was found
 * @return           FALSE to continue the replacement process
 */
gboolean translate_match(const GMatchInfo* match_info,
                         GString*          result,
                         gboolean*         matches)
{
    // If a string was captured, assume it contains the variable
    // name. Otherwise, use the whole match.
    gchar* key = g_match_info_fetch(match_info, 1);

    if (key == NULL)
        key = g_match_info_fetch(match_info, 0);

    g_assert(key != NULL);

    GNode* node = mk_store_node_get_by_name(m_tree, key, FALSE);

    if (node != NULL && node->data != NULL) {
        MkNodeInfo* info = (MkNodeInfo*)(node->data);
        if (info->value != NULL) {
            *matches = TRUE;
            g_string_append(result, info->value);
        }
    }

    return FALSE;
}


/**
 * Translate a whole line, replacing all occurences of the translation
 * regular expression with the value of the variable it references.
 * @param line the line to translate
 * @return     the translated line
 */
gchar* translate(const gchar* line)
{
    GError*  error = NULL;
    GRegex*  regex = g_regex_new(m_translation, 0, 0, &error);
    gchar*   result = NULL;
    gboolean matches = FALSE;

    if (error != NULL)
        g_critical("Regular expression error: %s", error->message);

    result = g_regex_replace_eval(regex, line, -1, 0, 0,
                                  (GRegexEvalCallback)translate_match,
                                  &matches, &error);

    if (error != NULL)
        g_critical("Substitution error: %s", error->message);

    g_regex_unref(regex);

    if (matches) {
        return result;
    } else {
        g_free(result);
        return NULL;
    }
}


/**
 * Interpret and execute a command.
 * @param line the raw command line
 * @return     FALSE if we must quit. TRUE otherwise
 */
gboolean command(const gchar* line)
{
    gboolean tree_changed = FALSE;

    // Exit upon request
    if (g_regex_match_simple("^\\s*exit\\s*$", line, 0, 0))
        return FALSE;
    
    // Read the line. Update the file if necessary.
    gchar* key;
    gchar* value;
    tree_changed = mk_key_value_read_line(m_tree, line, &key, &value);
    if (tree_changed && !m_read_only && m_auto_save)
        mk_key_value_write_file(m_tree, m_files[0]);
    
    // In translation mode, translate the line unless it was an assignation.
    if (key == NULL && m_translation != NULL) {

        gchar* result = translate(line);
        if (result != NULL) {
            g_printf("%s", result);
            g_free(result);
        } else {
            g_printf("%s", line);            
        }
        fflush(stdout);
    }

    // In echo mode, echo the key=value pair.
    if (m_echo && m_translation == NULL) {
        if (value == NULL) {
            key = g_strdup(line);
            g_strchomp(key);
            GNode* node = mk_store_node_get_by_name(m_tree, key, FALSE);
            if (node != NULL) {
                MkNodeInfo* info = (MkNodeInfo*)(node->data);
                value = g_strdup(info->value);
            }
        }
        
        if (value != NULL) {
            g_printf("%s=%s\n", key, value);
            fflush(stdout);
        }
    }

    g_free(key);
    g_free(value);

    return TRUE;
}


/**
 * Update the file if not in read-only mode.
 */
void update_file()
{
    if (!m_read_only) {
        g_assert(m_files[0] != NULL);
        mk_key_value_write_file(m_tree, m_files[0]);
    }
}


/**
 * Read the standard input and execute commands.
 * @param source    IO channel to read from
 * @param condition condition that happened
 * @return          whether the source must still be watched
 */
gboolean read_input(GIOChannel*  source,
                    GIOCondition condition,
                    gpointer     unused)
{
    while(1) {
        gchar*    line;
        gsize     length;
        GError*   error = NULL;
        GIOStatus status;
        
        status = g_io_channel_read_line(source, &line, &length, NULL, &error);

        switch(status) {
        case G_IO_STATUS_ERROR:
            // An error occurred
            g_free(line);
            g_critical("Input/output error.");
            return FALSE;
            
        case G_IO_STATUS_NORMAL:
            // Success. If the command returns false, it means that it
            // is time to exit.
            if(!command(line)) {
                update_file();
                g_free(line);
                g_main_loop_quit(m_main_loop);
                return FALSE;
            }
            break;
            
        case G_IO_STATUS_EOF:
            // End of file
            update_file();
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
 * Main
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
    if (!g_option_context_parse(context, &argc, &argv, &error))
        g_critical("%s", error->message);

    if (m_version) {
        g_printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        exit(EXIT_SUCCESS);
    }

    if (m_files != NULL && m_files[1] != NULL)
        g_critical("Too many input files.");

    // Initialize the tree
    m_tree = mk_store_node_new("");
    
    // Read the input file. If no file name was provided, assume
    // read-only mode.
    if (m_files != NULL && m_files[0] != NULL)
        mk_key_value_read_file(m_tree, m_files[0], m_read_only);
    else
        m_read_only = TRUE;

    // Start the main loop and watch stdin.
    GIOChannel* chan = g_io_channel_unix_new(STDIN_FILENO);
    g_io_channel_set_flags(chan, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP, read_input, NULL);

    m_main_loop = g_main_loop_new(NULL, TRUE);

    g_main_loop_run(m_main_loop);
}
