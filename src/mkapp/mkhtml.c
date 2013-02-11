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
#include <unistd.h>

#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#define PACKAGE_NAME         "mkhtml"
#define PACKAGE_VERSION      "0.1"
#define PACKAGE_PARAM_STRING "URI - open URI in a window"

#define DEFAULT_WIDTH       800
#define DEFAULT_HEIGHT      600
#define DEFAULT_BORDERLESS  FALSE
#define DEFAULT_TRANSPARENT FALSE


/**
 * Structure for mkhtml windows holding the all the widgets of a web
 * window.
 */
typedef struct {
    GtkWidget* window;
    GtkWidget* scrolled_window;
    GtkWidget* web_view;
} MkhtmlWindow;


static void create_window(const gchar* uri,
                          const gint width,
                          const gint height,
                          const gboolean decorated,
                          const gboolean transparent,
                          MkhtmlWindow* mh_window);

/*
 * Command-line options.
 */

gchar**  m_files = NULL;    // Input files
gboolean m_version = FALSE; // Obtain version information ?
gint     m_n_windows = 0;   // Number of running windows

gint     m_width       = DEFAULT_WIDTH;
gint     m_height      = DEFAULT_HEIGHT;
gboolean m_borderless  = DEFAULT_BORDERLESS;
gboolean m_transparent = DEFAULT_TRANSPARENT;

static GOptionEntry entries[] = {
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &m_files,
      "URI", NULL },
    { "width", 'w', 0, G_OPTION_ARG_INT, &m_width, "Window width", NULL },
    { "height", 'h', 0, G_OPTION_ARG_INT, &m_height, "Window height", NULL },
    { "borderless", 'b', 0, G_OPTION_ARG_NONE, &m_borderless,
      "Do not show window decorations", NULL },
    { "transparent", 't', 0, G_OPTION_ARG_NONE, &m_transparent,
      "Make the background transparent", NULL },
    { "version", 'V', 0, G_OPTION_ARG_NONE,
      (gpointer)&m_version, "Print version information", NULL },
    { NULL }
};


/**
 * Window destroyed callback. Free resources associated with a web
 * window and exit when all windows have been destroyed.
 * @param web_view  Webkit web view that triggered the event
 * @param mh_window web window destroyed
 */
static void destroy_cb(GtkWidget* web_view, MkhtmlWindow* mh_window)
{
    g_free(mh_window);
    if (--m_n_windows == 0)
        gtk_main_quit();
}


/**
 * Document title changed callback. Changes the window title to match
 * the document title.
 * @param web_view Webkit web view that triggered the event
 * @param window   window that must be updated
 */
static void notify_title_cb(WebKitWebView* web_view,
                            GParamSpec*    unused,
                            GtkWindow*     window)
{
    const gchar* title = webkit_web_view_get_title(web_view);
    if (title != NULL)
        gtk_window_set_title(window, title);
}

/**
 * Width or height changed callback. This is called the contents of a
 * WebKit web view requests that a window be resized.
 * @param web_view Webkit web view that triggered the event
 * @param window   window that must be updated
 */
static void notify_width_height_cb(WebKitWebWindowFeatures* features,
                                   GParamSpec*              unused,
                                   GtkWindow*               window)
{
    gint width, height;
    g_object_get(G_OBJECT(features), "width", &width, NULL);
    g_object_get(G_OBJECT(features), "height", &height, NULL);

    gtk_window_resize(window, width, height);
    g_debug("resize (%d, %d)", width, height);
}


/**
 * X or Y position changed callback. This is called the contents of a
 * WebKit web view requests that a window be moved.
 * @param web_view Webkit web view that triggered the event
 * @param window   window that must be updated
 */
static void notify_x_y_cb(WebKitWebWindowFeatures* features,
                          GParamSpec*              unused,
                          GtkWindow*               window)
{
    gint x, y;
    g_object_get(G_OBJECT(features), "x", &x, NULL);
    g_object_get(G_OBJECT(features), "y", &y, NULL);
    gtk_window_move(window, x, y);
    g_debug("move (%d, %d)", x, y);
}


/**
 * Window features changed callback. Connect signal handlers for
 * WindowFeature-related actions.
 * @param web_view Webkit web view that triggered the event
 * @param window   window that must be updated
 */
static void notify_window_features_cb(WebKitWebView* web_view,
                                      GParamSpec*    unused,
                                      GtkWindow*     window)
{
    WebKitWebWindowFeatures* features;
    features = webkit_web_view_get_window_features(web_view);

    g_signal_connect(G_OBJECT(features), "notify::x",
                     G_CALLBACK(notify_x_y_cb), window);
    g_signal_connect(G_OBJECT(features), "notify::y",
                     G_CALLBACK(notify_x_y_cb), window);
    g_signal_connect(G_OBJECT(features), "notify::width",
                     G_CALLBACK(notify_width_height_cb), window);
    g_signal_connect(G_OBJECT(features), "notify::height",
                     G_CALLBACK(notify_width_height_cb), window);
}


/**
 * Script alert callback. Prints Javascript alert messages to standard
 * output.
 * @param web_view Webkit web view that triggered the event
 * @param message  test string written
 */
static gboolean script_alert_cb(WebKitWebView*  web_view,
                                WebKitWebFrame* unused,
                                gchar*          message,
                                gpointer        unused2)
{
    g_printf("%s\n", message);
    fflush(stdout);
    return TRUE;
}


/**
 * Create new web view callback. This is called when javascript code
 * tries to open a new window.
 * @param web_view the object on which the signal is emitted
 * @param frame    the WebKitWebFrame
 * @return         a newly allocated WebKitWebView
 */
static WebKitWebView* create_web_view_cb(WebKitWebView*  web_view,
                                         WebKitWebFrame* frame,
                                         gpointer        unused)
{
    MkhtmlWindow* mh_window = g_malloc(sizeof(MkhtmlWindow));
    create_window("", 1, 1,
                  !m_borderless, m_transparent,
                  mh_window);

    return WEBKIT_WEB_VIEW(mh_window->web_view);
}


/**
 * Callback called when javascripts requests that the window be
 * closed. The program exits.
 * @param web_view Webkit web view that triggered the event
 * @param window   window to hide
 */
static gboolean close_web_view_cb(WebKitWebView* web_view,
                                  GtkWidget*     window)
{
    gtk_widget_hide(window);
    gtk_main_quit();
    return TRUE;
}


/**
 * Read the standard input and execute any Javascript code, line by line.
 * @param source    IO channel to read from
 * @param condition condition that happened
 * @param web_view  Webkit web view to execute the command on
 * @return          whether the source must still be watched
 */
gboolean read_input(GIOChannel*    source,
                    GIOCondition   condition,
                    WebKitWebView* web_view)
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
            webkit_web_view_execute_script(web_view, line);
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
 * Web view ready callback. Display the web view.
 * @param web_view  Webkit web view that triggered the event
 * @param mh_window web window containing the web view
 * @return          FALSE to propagate the event further
 */
static gboolean web_view_ready_cb(WebKitWebView* web_view,
                                  MkhtmlWindow*  mh_window)
{
    gtk_widget_show(mh_window->web_view);
    gtk_widget_show(mh_window->scrolled_window);
    gtk_widget_show(mh_window->window);
    return FALSE;
}


/**
 * Button press callback. Start dragging the window clicked.
 * @param web_view  Webkit web view that triggered the event
 * @param mh_window web window containing the web view
 * @return          TRUE if the window starts being dragged
 */
static gboolean button_press_event_cb(WebKitWebView*  web_view,
                                      GdkEventButton* event,
                                      MkhtmlWindow*   mh_window)
{
    WebKitHitTestResult* test_result;
    test_result = webkit_web_view_get_hit_test_result(web_view, event);

    WebKitHitTestResultContext ctx;
    g_object_get(G_OBJECT(test_result), "context", &ctx, NULL);

    if (ctx == WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT) {

        gtk_window_begin_move_drag(GTK_WINDOW(mh_window->window),
                                   event->button,
                                   event->x_root,
                                   event->y_root,
                                   event->time);

        return TRUE;
    }

    g_object_unref(test_result);
    return FALSE;
}


/**
 * Create a new window with a scrolled window and a WebKit webview
 * displaying web contents.
 * @param uri          web URI to load into the window
 * @param width        window width
 * @param height       window height
 * @param decorated    show window decorations ?
 * @param transparent  make the background transparent ?
 * @param mh_window    return location for the new widgets
 */
static void create_window(const gchar* uri,
                          const gint width,
                          const gint height,
                          const gboolean decorated,
                          const gboolean transparent,
                          MkhtmlWindow* mh_window)
{

    // Create widgets
    GtkWidget* window          = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget* web_view        = webkit_web_view_new();

    // Configure widgets
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(web_view), uri);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    gtk_window_set_decorated(GTK_WINDOW(window), decorated);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    // If requested and supported, set the window background transparent.
    if (transparent) {
        GdkScreen*   screen = gtk_widget_get_screen(window);
        GdkColormap* colormap = gdk_screen_get_rgba_colormap(screen);

        if (colormap) {
            gtk_widget_set_default_colormap(colormap);
            webkit_web_view_set_transparent(WEBKIT_WEB_VIEW(web_view), TRUE);
        } else {
            g_warning("The screen does not support alpha channels.");
        }
    }

    // Pack the widgets
    gtk_container_add(GTK_CONTAINER(scrolled_window), web_view);
    gtk_container_add(GTK_CONTAINER(window),          scrolled_window);

    // Configure signal handlers
    g_signal_connect(window, "destroy", G_CALLBACK(destroy_cb), mh_window);

    g_signal_connect(G_OBJECT(web_view), "close-web-view",
                     G_CALLBACK(close_web_view_cb), window);
    g_signal_connect(G_OBJECT(web_view), "create-web-view",
                     G_CALLBACK(create_web_view_cb), NULL);
    g_signal_connect(G_OBJECT(web_view), "web-view-ready",
                     G_CALLBACK(web_view_ready_cb), mh_window);
    g_signal_connect(G_OBJECT(web_view), "script-alert",
                     G_CALLBACK(script_alert_cb), NULL);

    g_signal_connect(G_OBJECT(web_view), "notify::title",
                     G_CALLBACK(notify_title_cb), window);
    g_signal_connect(G_OBJECT(web_view), "notify::window-features",
                     G_CALLBACK(notify_window_features_cb), window);

    // Windows without decorations must be moveable. With this callback,
    // we start dragging the window when the web view is clicked.
    if (!decorated) {
        g_signal_connect(G_OBJECT(web_view), "button-press-event",
                         G_CALLBACK(button_press_event_cb), mh_window);
    }

    ++m_n_windows;

    if (mh_window != NULL) {
        mh_window->window = window;
        mh_window->scrolled_window = scrolled_window;
        mh_window->web_view = web_view;
    }
}


/**
 * Main.
 */
int main(int argc, char* argv[])
{
    // Initialize the gui
    gtk_init(&argc, &argv);
    if (!g_thread_supported())
        g_thread_init(NULL);

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

    if (m_files == NULL || m_files[0] == NULL) {
        g_fprintf(stderr, "No URI given\n");
        exit(1);
    }
    if (m_files[1] != NULL) {
        g_fprintf(stderr, "Too many URIs\n");
        exit(2);
    }

    // Create the main window and show it.
    MkhtmlWindow* mh_window = g_malloc(sizeof(MkhtmlWindow));
    create_window(m_files[0], m_width, m_height,
                  !m_borderless, m_transparent,
                  mh_window);
    gtk_widget_show(mh_window->web_view);
    gtk_widget_show(mh_window->scrolled_window);
    gtk_widget_show(mh_window->window);

    // Start the main loop and watch stdin.
    GIOChannel* chan = g_io_channel_unix_new(STDIN_FILENO);
    g_io_channel_set_flags(chan, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP,
                   (GIOFunc)read_input, mh_window->web_view);

    gtk_main();
}
