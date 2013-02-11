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
 * This file contains high level commands that can be performed on
 * GObjects. Most of these objects are GTK widgets.
 *
 * All functions are named after the following pattern :
 * mk_command_(class name)_(command name).
 *
 * Functions defined in this file are meant to be used with GModule instead
 * of being referenced directly. This is why no header file is provided.
 */


#include <glib.h>
#include <gtk/gtk.h>


/**
 * Display a widget.
 * @param widget widget to display
 */
void mk_command_GtkWidget_show(GtkWidget* widget, const gchar* unused)
{
    gtk_widget_show(widget);
}


/**
 * Hide a widget.
 * @param widget widget to hide
 */
void mk_command_GtkWidget_hide(GtkWidget* widget, const gchar* unused)
{
    gtk_widget_hide(widget);
}


/**
 * Set the tooltip of a widget.
 * @param widget widget to set the tooltip
 */
void mk_command_GtkWidget_tooltip(GtkWidget* widget, const gchar* markup)
{
    gtk_widget_set_tooltip_markup(widget, markup);
}


/*
 * GtkWindow
 */

/**
 * Set a window's title.
 * @param window window to change the title of
 * @param title  the new title
 */
void mk_command_GtkWindow_title(GtkWindow* window, const gchar* title)
{
    gtk_window_set_title(window, title);
}


/**
 * Set a window's icon.
 * @param window   window to change the icon of
 * @param filename path to the new icon file
 */
void mk_command_GtkWindow_icon(GtkWindow* window, const gchar* filename)
{
    gtk_window_set_icon_from_file(window, filename, NULL);
}


/**
 * Set the opacity of a window
 * @param window      window to set the opacity of
 * @param opacity_str new opacity as a character string, between 0.0 and 1.0.
 */
void mk_command_GtkWindow_opacity(GtkWindow* window, const gchar* opacity_str)
{
    gdouble opacity = g_ascii_strtod(opacity_str, NULL);
    if (gtk_widget_is_composited(GTK_WIDGET(window)))
        gtk_window_set_opacity(window, opacity);
    else
        g_warning
            ("Alpha values may not be drawn correctly for window \"%s\".",
             gtk_widget_get_name(GTK_WIDGET(window)));
}


/**
 * Set the urgency hint on a window.
 * @param window window that has urgent information in it
 */
void mk_command_GtkWindow_urgent(GtkWindow* window, const gchar* unused)
{
    gtk_window_set_urgency_hint(window, TRUE);
}


/**
 * Remove the urgency hint set with mk_command_GtkWindow_urgent().
 * @param window window to remove the hint from
 */
void mk_command_GtkWindow_not_urgent(GtkWindow* window, const gchar* unused)
{
    gtk_window_set_urgency_hint(window, FALSE);
}



/*
 * GtkWindow => GtkDialog => GtkMessageDialog
 */

/**
 * Set the main text on a message dialog window.
 * @param dialog dialog to change
 * @param markup new text to put on the dialog window
 */
void mk_command_GtkMessageDialog_text(GtkMessageDialog* dialog,
                                      const char* markup)
{
    gtk_message_dialog_set_markup(dialog, markup);
}


/**
 * Set the secondary text on a message dialog window.
 * @param dialog dialog to change
 * @param markup new text to put on the dialog window
 */
void mk_command_GtkMessageDialog_secondary(GtkMessageDialog* dialog,
                                           const char* markup)
{
    gtk_message_dialog_format_secondary_markup(dialog, "%s", markup);
}



/*
 * GtkWindow => GtkAssistant
 */

/**
 * Set the current page of an assistant.
 * @param assistant    assistant to change the current page
 * @param page_num_str new page number, as a string
 */
void mk_command_GtkAssistant_goto(GtkAssistant* assistant,
                                  const char* page_num_str)
{
    gint64 page_num = g_ascii_strtoll(page_num_str, NULL, 10);
    gtk_assistant_set_current_page(assistant, (gint)page_num);
}



/*
 * GtkMisc => GtkImage
 */

/**
 * Set the current image of an image widget.
 * @param image    image to set
 * @param filename path to the new image file
 */
void mk_command_GtkImage_set(GtkImage* image, const gchar* filename)
{
    gtk_image_set_from_file(image, filename);
}



/*
 * GtkMisc => GtkLabel
 */

/**
 * Set the text of a label widget.
 * @param label  label to set
 * @param markup new text for the label
 */
void mk_command_GtkLabel_set(GtkLabel* label, const gchar* markup)
{
    gtk_label_set_markup(label, markup);
}



/*
 * GtkProgress => GtkProgressBar
 */

/**
 * Set the text displayed inside a progress bar.
 * @param pbar progress bar to set the text of
 * @param text text to write on the progress bar
 */
void mk_command_GtkProgressBar_text(GtkProgressBar* pbar, const gchar* text)
{
    gtk_progress_bar_set_text(pbar, text);
}


/**
 * Set the percentage of a progress bar.
 * @param pbar    progress bar to set the percentage of
 * @param pct_str new progress, as a string, between 0.0 and 1.0
 */
void mk_command_GtkProgressBar_set(GtkProgressBar* pbar, const gchar* pct_str)
{
    gdouble pct = g_ascii_strtod(pct_str, NULL);
    gtk_progress_bar_set_fraction(pbar, pct);
}



/*
 * GtkContainer => GtkBox => GtkHBox => GtkStatusbar
 */

/**
 * Push a new message into a status bar.
 *
 * The command takes two arguments:
 *  - Context identifier string
 *  - Message to push into the status bar
 * @param sbar status bar to push a new message in
 * @param args whitespace-separated arguments for the command.
 */
void mk_command_GtkStatusbar_push(GtkStatusbar* sbar, const gchar* args)
{
    gchar** tokens;
    tokens = g_regex_split_simple("\\s+", args, 0, 0);

    if (tokens != NULL && tokens[0] != NULL && tokens[1] != NULL)
        gtk_statusbar_push(sbar,
                           gtk_statusbar_get_context_id(sbar, tokens[0]), 
                           tokens[1]);

    g_strfreev(tokens);
}


/**
 * Pop the last message put with mk_command_GtkStatusbar_push() with a
 * given context description.
 * @param sbar   status bar to pop a message from
 * @param source context identifier string
 */
void mk_command_GtkStatusbar_pop(GtkStatusbar* sbar, const gchar* source)
{
    gtk_statusbar_pop(sbar, gtk_statusbar_get_context_id(sbar, source));
}


/*
 * GtkStatusIcon
 */

/**
 * Set the image of a status icon.
 * @param icon     icon to set
 * @param filename path to the icon image file
 */
void mk_command_GtkStatusIcon_set(GtkStatusIcon* icon, const gchar* filename)
{
    gtk_status_icon_set_from_file(icon, filename);
}


/**
 * Set the tooltip of a status icon.
 * @param icon   icon to set the tooltip of
 * @param markup new text for the tooltip
 */
void mk_command_GtkStatusIcon_tooltip(GtkStatusIcon* icon, const gchar* markup)
{
#if GTK_CHECK_VERSION(2,14,0)
    gtk_status_icon_set_tooltip_markup(icon, markup);
#else
    gtk_status_icon_set_tooltip(icon, markup);    
#endif
}


/**
 * Display a status icon.
 * @param icon status icon to display
 */
void mk_command_GtkStatusIcon_show(GtkStatusIcon* icon, const gchar* unused)
{
    gtk_status_icon_set_visible(icon, TRUE);
}


/**
 * Hide a status icon displayed with mk_command_GtkStatusIcon_show().
 * @param icon status icon to hide
 */
void mk_command_GtkStatusIcon_hide(GtkStatusIcon* icon, const gchar* unused)
{
    gtk_status_icon_set_visible(icon, FALSE);
}


/**
 * Make a status icon blink.
 * @param icon status icon to start blinking
 */
void mk_command_GtkStatusIcon_blink(GtkStatusIcon* icon, const gchar* unused)
{
    gtk_status_icon_set_blinking(icon, TRUE);
}


/**
 * Stop a status icon from blinking.
 * @param icon status icon to stop from blinking.
 */
void mk_command_GtkStatusIcon_no_blink(GtkStatusIcon* icon, const gchar* unused)
{
    gtk_status_icon_set_blinking(icon, FALSE);
}



/*
 * GtkContainer => GtkBin => GtkButton
 */

/**
 * Set a button's text.
 * @param button button to set
 * @param text   text to write on the button
 */
void mk_command_GtkButton_text(GtkButton* button, const gchar* text)
{
    gtk_button_set_label(button, text);
}


/**
 * Set a button's image.
 * @param button   button to set
 * @param filename path to the icon image file
 */
void mk_command_GtkButton_image(GtkButton* button, const gchar* filename)
{
    GtkWidget* image = gtk_image_new_from_file(filename);
    gtk_button_set_image(button, image);
}



/*
 * GtkContainer => GtkBin => GtkButton => GtkToggleButton
 */

/**
 * Set a toggle button on.
 * @param button toggle button to set
 */
void mk_command_GtkToggleButton_on(GtkToggleButton* button,
                                   const gchar* unused)
{
    gtk_toggle_button_set_active(button, TRUE);
}


/**
 * Set a toggle button off.
 * @param button toggle button to set
 */
void mk_command_GtkToggleButton_off(GtkToggleButton* button,
                                    const gchar* unused)
{
    gtk_toggle_button_set_active(button, FALSE);
}



/*
 * GtkContainer => GtkBin => GtkButton => GtkLinkButton
 */

/**
 * Set the link of a link button.
 * @param link_button link button to set
 * @param uri         target of the link
 */
void mk_command_GtkLinkButton_link(GtkLinkButton* link_button, const gchar* uri)
{
    gtk_link_button_set_uri(link_button, uri);
}



/*
 * GtkContainer => GtkBin => GtkButton => GtkScaleButton
 */

/**
 * Set the current value of a scale button.
 * @param scale_button scale button to set
 * @param value_str    new value, as a string
 */
void mk_command_GtkScaleButton_set(GtkScaleButton* scale_button,
                                   const gchar* value_str)
{
    gdouble value = g_ascii_strtod(value_str, NULL);
    gtk_scale_button_set_value(scale_button, value);
}



/*
 * GtkEntry
 */

/**
 * Append new text to the end of an entry.
 * @param entry entry to add to
 * @param text  new text to append
 */
void mk_command_GtkEntry_insert(GtkEntry* entry, const gchar* text)
{
    gint position;
    gtk_editable_insert_text(GTK_EDITABLE(entry), text, -1, &position);
}


/**
 * Set the current contents of an entry.
 * @param entry entry to set
 * @param text  new contents of the entry
 */
void mk_command_GtkEntry_set(GtkEntry* entry, const gchar* text)
{
    gtk_entry_set_text(entry, text);
    gtk_editable_set_position(GTK_EDITABLE(entry), -1); // End
}


/**
 * Remove all the contents of an entry.
 * @param entry entry to clear
 */
void mk_command_GtkEntry_clear(GtkEntry* entry, const gchar* unused)
{
    gtk_entry_set_text(entry, "");
}


/**
 * Set the progress of an entry.
 * @param entry        entry to set the progress
 * @param fraction_str new progress, as a string, between 0.0 and 1.0
 */
void mk_command_GtkEntry_fraction(GtkEntry* entry, const gchar* fraction_str)
{
#if GTK_CHECK_VERSION(2,16,0)
    gdouble fraction = g_ascii_strtod(fraction_str, NULL);
    gtk_entry_set_progress_fraction(entry, fraction);
#endif
}



/*
 * GtkRange
 */

/**
 * Set the current value of a range widget.
 * @param range     range to set
 * @param value_str new value, as a string
 */
void mk_command_GtkRange_set(GtkRange* range, const gchar* value_str)
{
    gdouble value = g_ascii_strtod(value_str, NULL);
    gtk_range_set_value(range, value);
}



/*
 * GtkTextBuffer
 */

/**
 * Append text to the end of a text buffer.
 * @param buffer buffer to add text to
 * @param text   text to append
 */
void mk_command_GtkTextBuffer_insert(GtkTextBuffer* buffer, const gchar* text)
{
    gtk_text_buffer_insert_at_cursor(buffer, text, -1);
}


/**
 * Set the contents of a text buffer.
 * @param buffer buffer to set
 * @param text   new text to put into the buffer
 */
void mk_command_GtkTextBuffer_set(GtkTextBuffer* buffer, const gchar* text)
{
    gtk_text_buffer_set_text(buffer, text, -1);
}


/**
 * Remove all contents of a text buffer.
 * @param buffer text buffer to clear
 */
void mk_command_GtkTextBuffer_clear(GtkTextBuffer* buffer, const gchar* unused)
{
    gtk_text_buffer_set_text(buffer, "", -1);
}



/*
 * GtkContainer => GtkTextView
 */

/**
 * Append text to the end of a text view.
 * @param text_view text view to add text to
 * @param text      text to append
 */
void mk_command_GtkTextView_insert(GtkTextView* text_view, const gchar* text)
{
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(text_view);
    mk_command_GtkTextBuffer_insert(buffer, text);
}


/**
 * Set the contents of a text view.
 * @param text_view text view to set
 * @param text      new text to put into the text view
 */
void mk_command_GtkTextView_set(GtkTextView* text_view, const gchar* text)
{
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(text_view);
    mk_command_GtkTextBuffer_set(buffer, text);
}


/**
 * Remove all contents of a text view.
 * @param text_view text view to clear
 */
void mk_command_GtkTextView_clear(GtkTextView* text_view, const gchar* unused)
{
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(text_view);
    mk_command_GtkTextBuffer_clear(buffer, unused);
}



/*
 * GtkTreeStore
 */

void tree_model_split_args(GtkTreeModel* tree_model, const gchar* args,
                           gint* n_columns, gchar*** values)
{
    *n_columns = gtk_tree_model_get_n_columns(tree_model);
    *values = g_regex_split_simple("\\t+", args, 0, 0);
}


/**
 * Add an item to a tree store.
 * @param tree_store tree store to add to
 * @param args       parent path and whitespace-separated list of column values
 */
void mk_command_GtkTreeStore_add(GtkTreeStore* tree_store, const gchar* args)
{
    gint n_columns;
    gchar** values;
    tree_model_split_args(GTK_TREE_MODEL(tree_store), args,
                          &n_columns, &values);
    
    GtkTreeIter iter;
    GtkTreeIter parent;
    gboolean parent_valid = FALSE;

    // Try to resolve the first argument as a parent path in the tree. If it
    // is found, then the new row is appended under it.
    if (g_regex_match_simple("^\\d+(:\\d+)*$", values[0], 0, 0)) {

        gchar* parent_str = values[0];
        GtkTreePath* parent_path
            = gtk_tree_path_new_from_string(parent_str);

        if (parent_path != NULL) {
            parent_valid = gtk_tree_model_get_iter
                (GTK_TREE_MODEL(tree_store), &parent, parent_path);
        }

    }

    if (parent_valid)
        gtk_tree_store_append(tree_store, &iter, &parent);
    else
        gtk_tree_store_append(tree_store, &iter, NULL);
    
    // Set values
    for (gint i = 0; i < n_columns; ++i) {
        if (values[i+1] == NULL)
            break;
        
        GValue value = {0};
        g_value_init(&value, G_TYPE_STRING);
        g_value_set_string(&value, values[i+1]);
        gtk_tree_store_set_value(tree_store, &iter, i, &value);
    }

    g_strfreev(values);
}


/**
 * Remove all items from a tree store.
 * @param tree_store tree store to clear
 */
void mk_command_GtkTreeStore_clear(GtkTreeStore* tree_store,
                                   const gchar* unused)
{
    gtk_tree_store_clear(tree_store);
}



/*
 * GtkListStore
 */

/**
 * Add an item to a list store.
 * @param list_store list store to add to
 * @param args       whitespace-separated list of column values
 */
void mk_command_GtkListStore_add(GtkListStore* list_store, const gchar* args)
{
    gint n_columns;
    gchar** values;
    tree_model_split_args(GTK_TREE_MODEL(list_store), args,
                          &n_columns, &values);

    // Append
    GtkTreeIter iter;
    gtk_list_store_append(list_store, &iter);

    // Set values
    for (gint i = 0; i < n_columns; ++i) {
        if (values[i] == NULL)
            break;
        
        GValue value = {0};
        g_value_init(&value, G_TYPE_STRING);
        g_value_set_string(&value, values[i]);
        gtk_list_store_set_value(list_store, &iter, i, &value);
    }

    g_strfreev(values);
}


/**
 * Remove all items from a list store.
 * @param list_store list store to clear
 */
void mk_command_GtkListStore_clear(GtkListStore* list_store,
                                   const gchar* unused)
{
    gtk_list_store_clear(list_store);
}



/*
 * GtkAdjustment
 */

/**
 * Set the value of an adjustment.
 * @param adjustment adjustment to set
 * @param value_str  new value, as a string
 */
void mk_command_GtkAdjustment_set(GtkAdjustment* adjustment,
                                  const gchar* value_str)
{
    gdouble value = g_ascii_strtod(value_str, NULL);
    gtk_adjustment_set_value(adjustment, value);
}



/*
 * GtkContainer => GtkBin => GtkComboBox
 */

/**
 * Select an item inside a combo box.
 * @param combo_box combo box to set
 * @param index_str index of the item to select, as a string
 */
void mk_command_GtkComboBox_set(GtkComboBox* combo_box, const gchar* index_str)
{
	gint index = g_ascii_strtoll(index_str, NULL, 10);
	gtk_combo_box_set_active(combo_box, index);
}

