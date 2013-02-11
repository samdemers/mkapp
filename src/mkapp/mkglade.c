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
 * Generic, GtkBuilder-based user interface manager.
 */

#include <unistd.h>
#include <stdlib.h>

#include <glib/gprintf.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#define PACKAGE_NAME         "mkglade"
#define PACKAGE_VERSION      "0.1"
#define PACKAGE_PARAM_STRING "FILE - open a Glade GUI"

#define COMMAND_PREFIX       "mk_command_"
#define INFO_PREFIX          "mk_print_"
#define INFO_SUFFIX          "_info"


/**
 * Information printing functions called when signals are received.
 * @param object object that generated the signal
 */
typedef void (*PrintInfoFun)(GObject* object);

/**
 * Command functions called when user commands are received.
 * @param object target object
 * @param args   command arguments
 */
typedef void (*CommandFun)(GObject* object, gchar* args);


/**
 * Structure for the data passed to callbacks whenever a signal is
 * thrown.
 */
typedef struct {
    gchar*        signal_name;
    gchar*        handler_name;
    GObject*      object;
    PrintInfoFun* info_functions;
} callback_data;


/*
 * Command-line options.
 */
gchar**  m_files = NULL;    // Input files
gboolean m_version = FALSE; // Obtain version information ?

static GOptionEntry entries[] = {
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &m_files,
      "GtkBuilder file", NULL },
    { "version", 'V', 0, G_OPTION_ARG_NONE,
      (gpointer)&m_version, "Print version information", NULL },
    { NULL }
};


/**
 * Allocate and initialize callback data.
 * @param signal_name         name of the signal thrown
 * @param handler_name        name of the handler
 * @param object              object from which the signal originated
 * @param info_functions_size maximum number of info functions
 * @return                    callback data
 */
gpointer callback_data_new(const gchar* signal_name,
                           const gchar* handler_name,
                           GObject* object,
                           size_t info_functions_size)
{
    callback_data* cd = g_new(callback_data, 1);
    cd->signal_name = g_strdup(signal_name);
    cd->handler_name = g_strdup(handler_name);
    cd->object = object;
    cd->info_functions = g_new0(PrintInfoFun, info_functions_size+1);
    return cd;
}


/**
 * Deallocate callback data allocated using callback_data_new().
 * @param cd the data to free
 */
void callback_data_free(callback_data* cd)
{
    g_free(cd->signal_name);
    g_free(cd->handler_name);
    g_free(cd->info_functions);
    g_free(cd);
}


/**
 * Print information about a signal and the object that sent it to the
 * standard output.
 * @param data callback data, which include the handler name and the object
 */
void print_default_info(callback_data* data)
{

    // Print generic info
    g_printf("%s", data->handler_name);
    GObject* object = data->object;

    // Print specific info, if any.
    for(gsize i = 0; data->info_functions[i] != NULL; ++i) {
        PrintInfoFun fun = data->info_functions[i];
        fun(object);
    }

    g_printf("\n");
    fflush(stdout);
}


/**
 * Connect signals to the default callback. All signals are connected
 * to a generic callback function that writes information about the signal
 * and the object that sent it to the standard output.
 * @param builder        unused
 * @param object         object that will throw the signal
 * @param signal_name    name of that signal
 * @param handler_name   handler name, which is an arbitrary string
 * @param connect_object unused
 * @param flags          unused
 * @param user_data      unused
 */
void connect_signals(GtkBuilder    *builder,
                     GObject       *object,
                     const gchar   *signal_name,
                     const gchar   *handler_name,
                     GObject       *connect_object,
                     GConnectFlags flags,
                     gpointer      user_data)
{
    if (object == NULL)
        return;

    callback_data* cd;
    gsize info_functions_size = g_type_depth(G_OBJECT_TYPE(object));
    cd = callback_data_new(signal_name, handler_name, object,
                           info_functions_size);

    // If supported, find the specific handlers for this object type and
    // parent types.
    if (g_module_supported()) {

        GModule* module = g_module_open(NULL, 0); // Main program
        GType type = G_OBJECT_TYPE(object);

        for(gsize i = 0; type != 0; ++i) {
            gchar* symbol_name;
            symbol_name = g_strconcat(INFO_PREFIX, g_type_name(type),
                                      INFO_SUFFIX, NULL);

            g_module_symbol(module, symbol_name,
                            (gpointer*)&(cd->info_functions[i]));

            g_free(symbol_name);
            type = g_type_parent(type);
        }

        g_module_close(module);
    }

    // Connect the default handler.
    g_signal_connect_swapped(object,
                             signal_name,
                             G_CALLBACK(print_default_info),
                             cd);
}


/**
 * Interpret and execute a command.
 * @param line    the raw command line
 * @param builder GtkBuilder object to act on
 */
void command(GtkBuilder* builder, const gchar* line)
{
    gchar* widget_name = NULL;
    gchar* cmd = NULL;
    gchar* args = NULL;

    // Match the command line against a regular expression and extract
    // the target object, the command and the arguments.
    GError* error = NULL;

    GRegex* regex = g_regex_new
        ("\\s*(?:(?:\"(?<WID>\\w+?)\")|(?<WID>\\w+?))\\s+"
         "(?:(?:\"(?<CMD>.*?)\")|(?<CMD>.*?))"
         "(?:\\s+(?<ARG>.*?))?\\n?$",
         G_REGEX_DUPNAMES, 0, &error);
    if (error != NULL)
        g_critical("could not compile regular expression: %s",
                   error->message);
    
    GMatchInfo* match_info;
    g_regex_match(regex, line, 0, &match_info);
    if (g_match_info_matches(match_info)) {
	    widget_name = g_match_info_fetch_named(match_info, "WID");
	    cmd = g_match_info_fetch_named(match_info, "CMD");
	    args = g_match_info_fetch_named(match_info, "ARG");
    } else {
        g_match_info_free(match_info);
        g_regex_unref(regex);
        return;
    }

    g_match_info_free(match_info);
    g_regex_unref(regex);

    if (widget_name == NULL || cmd == NULL) {
        g_free(widget_name);
        g_free(cmd);
        g_free(args);
        return;
    }

    GObject* object = NULL;
    object = gtk_builder_get_object(builder, widget_name);
    if (object == NULL) {
        g_warning("Object \"%s\" not found.", widget_name);
        g_free(widget_name);
        g_free(cmd);
        g_free(args);
        return;
    }
    
    // Use GModule to find the handlers for the object's type and parent types
    gboolean command_is_supported = FALSE;

    if (g_module_supported()) {
        
        GModule* module = g_module_open(NULL, 0); // Main program
        GType type = G_OBJECT_TYPE(object);

        for(gsize i = 0; type != 0; ++i) {

            CommandFun fun;

            // Find the function
            gchar* symbol_name
                = g_strconcat(COMMAND_PREFIX,
                              g_type_name(type), "_", cmd, NULL);
            g_module_symbol(module, symbol_name, (gpointer*)&(fun));
            g_free(symbol_name);

            // Execute the function found
            if (fun != NULL) {
                command_is_supported = TRUE;
                fun(object, args);
            }            

            type = g_type_parent(type);
        }

        g_module_close(module);
    }

    if (!command_is_supported) {
        g_warning("Command \"%s\" unsupported for %s object.",
                  cmd, G_OBJECT_TYPE_NAME(object));
    }

    g_free(widget_name);
    g_free(cmd);
    g_free(args);
}


/**
 * Read the standard input and execute commands.
 * @param source    IO channel to read from
 * @param condition condition that happened
 * @param builder   GtkBuilder object to manage
 * @return          whether the source must still be watched
 */
gboolean read_input(GIOChannel*  source,
                    GIOCondition condition,
                    GtkBuilder*  builder)
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
            // Success
            command(builder, line);
            break;
            
        case G_IO_STATUS_EOF:
            // End of file
            g_free(line);
            gtk_main_quit();
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
 * Main.
 */
int main(int argc, char* argv[])
{
    GtkBuilder* builder;

    // Make critical errors fatal to abort when they happen
    g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);

    // Read command-line arguments
    GError* error = NULL;
    GOptionContext* context;
    context = g_option_context_new(PACKAGE_PARAM_STRING);
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));

    if (!g_option_context_parse(context, &argc, &argv, &error))
        g_critical("%s", error->message);

    if (m_version) {
        g_printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        exit(EXIT_SUCCESS);
    }

    if (m_files == NULL || m_files[0] == NULL)
        g_critical("No input file");
    if (m_files[1] != NULL)
        g_critical("Too many input files.");

    // Initialize the gui
    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file (builder, m_files[0], &error)) {
        g_critical("Couldn't load builder file: %s", error->message);
        g_error_free(error);
    }

    gtk_builder_connect_signals_full(builder, connect_signals, NULL);

    // Emit a warning about GModule
    if (!g_module_supported())
        g_warning("GModule not supported. Most functions will not work.");

    // Start the main loop and watch stdin.
    GIOChannel* chan = g_io_channel_unix_new(STDIN_FILENO);
    g_io_channel_set_flags(chan, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP,
                   (GIOFunc)read_input, builder);

    gtk_main();
}

