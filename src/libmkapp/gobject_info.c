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
 * This file contains information-printing functions for GObjects. Most
 * of them are GTK widgets.
 *
 * All functions are named after the following pattern :
 * mk_print_(class name)_info.
 *
 * Functions defined in this file are meant to be used with GModule instead
 * of being referenced directly. This is why no header file is provided.
 */


#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>


/*
 * GtkWindow
 */

/**
 * Print a window's position and size.
 * @param window window to print information about
 */
void mk_print_GtkWindow_info(GtkWindow* window)
{
    gint x, y, w, h;
    gtk_window_get_position(window, &x, &y);
    gtk_window_get_size(window, &w, &h);
    g_printf("\t%d,%d\t%d,%d", x, y, w, h);
}



/*
 * GtkWindow => GtkAssistant
 */

/**
 * Print the current page and page count of an assistant.
 * @param assistant assistant to print information about
 */
void mk_print_GtkAssistant_info(GtkAssistant* assistant)
{
    gint current_page = gtk_assistant_get_current_page(assistant);
    gint n_pages = gtk_assistant_get_n_pages(assistant);
    g_printf("\t%d/%d", current_page, n_pages);
}



/*
 * GtkButton => GtkToggleButton
 */

/**
 * Print "on" or "off" to reflect a toggle button's state.
 * @param toggle_button toggle button to print information about
 */
void mk_print_GtkToggleButton_info(GtkToggleButton* toggle_button)
{
    if (gtk_toggle_button_get_active(toggle_button))
        g_printf("\ton");
    else
        g_printf("\toff");
}



/*
 * GtkButton => GtkScaleButton
 */

/**
 * Print the current value of a scale button.
 * @param scale_button scale button to print information about
 */
void mk_print_GtkScaleButton_info(GtkScaleButton* scale_button)
{
    gdouble value = gtk_scale_button_get_value(scale_button);
    g_printf("\t%f", value);
}



/*
 * GtkEntry
 */

/**
 * Print the current contents of an entry.
 * @param entry entry to print information about
 */
void mk_print_GtkEntry_info(GtkEntry* entry)
{
    const gchar* text = gtk_entry_get_text(entry);
    g_printf("\t%s", text);
}



/*
 * GtkEntry => GtkSpinButton
 */

/**
 * Print the current value of a spin button.
 * @param spin spin button to print information about
 */
void mk_print_GtkSpinButton_info(GtkSpinButton* spin)
{
    if (gtk_spin_button_get_digits(spin) == 0) {
        gint value = gtk_spin_button_get_value_as_int(spin);
        g_printf("\t%d", value);
    } else {
        gdouble value = gtk_spin_button_get_value(spin);
        g_printf("\t%f", value);
    }
}



/*
 * GtkRange
 */

/**
 * Print the current value of a range.
 * @param range range to print information about
 */
void mk_print_GtkRange_info(GtkRange* range)
{
    gdouble value = gtk_range_get_value(range);
    gdouble fill = gtk_range_get_fill_level(range);
    g_printf("\t%f\t%f", value, fill);
}



/*
 * GtkTextBuffer
 */

/**
 * Print the current contents of a text buffer.
 * @param buffer text buffer to print information about
 */
void mk_print_GtkTextBuffer_info(GtkTextBuffer* buffer)
{
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    const gchar* text = gtk_text_buffer_get_text(buffer, &start, &end, 0);

    gint lines = gtk_text_buffer_get_line_count(buffer);
    g_printf("\t%d\n%s", lines, text);
}



/*
 * GtkContainer => GtkTextView
 */

/**
 * Print the current contents of a text view.
 * @param text_view text view to print information about
 */
void mk_print_GtkTextView_info(GtkTextView* text_view)
{
    GtkTextBuffer* text_buffer = gtk_text_view_get_buffer(text_view);
    mk_print_GtkTextBuffer_info(text_buffer);
}



/*
 * GtkContainer => GtkTreeView
 */

void print_tree_selection(GtkTreeModel *model,
                          GtkTreePath *path,
                          GtkTreeIter *iter,
                          gpointer data)
{
    gint columns = gtk_tree_model_get_n_columns(model);

    // Print the path of the selection
    g_printf("\t%s", gtk_tree_path_to_string(path));

    // Print all the text columns
    for (gint i = 0; i < columns; ++i) {
        GValue value = {0};
        GValue str_value = {0};

        gtk_tree_model_get_value(model, iter, i, &value);
        g_value_init(&str_value, G_TYPE_STRING);
        if (g_value_type_transformable(G_VALUE_TYPE(&value), G_TYPE_STRING)) {
            g_value_transform(&value, &str_value);
            g_printf("\t%s", g_value_get_string(&str_value));
        }

        g_value_unset(&value);
    }
}


/**
 * Print the selected items in a tree view.
 * @param tree_view tree view to print information about
 */
void mk_print_GtkTreeView_info(GtkTreeView* tree_view)
{
    GtkTreeSelection* selection = gtk_tree_view_get_selection(tree_view);
    g_printf("\t");
    gtk_tree_selection_selected_foreach(selection, print_tree_selection, NULL);
}


/**
 * Print a column's title.
 * @param column tree view column to print information about
 */
void mk_print_GtkTreeViewColumn_info(GtkTreeViewColumn* column)
{
    const gchar* title = gtk_tree_view_column_get_title(column);
    gint sort_column_id = gtk_tree_view_column_get_sort_column_id(column);
    g_printf("\t%d\t%s", sort_column_id, title);
}



/*
 * GtkContainer => GtkIconView
 */

void print_icon_selection(GtkIconView *icon_view,
                          GtkTreePath *path,
                          gpointer data)
{
    g_printf("%s ", gtk_tree_path_to_string(path));    
}


/**
 * Print the selected items in an icon view.
 * @param icon_view icon view to print information about.
 */
void mk_print_GtkIconView_info(GtkIconView* icon_view)
{
    g_printf("\t");
    gtk_icon_view_selected_foreach(icon_view, print_icon_selection, NULL);
}



/*
 * GtkContainer => GtkBin => GtkComboBox
 */

/**
 * Print the selected item in a combo box.
 * @param combo_box combo box to print information about
 */
void mk_print_GtkComboBox_info(GtkComboBox* combo_box)
{
    gint          active = gtk_combo_box_get_active(combo_box);
    GtkTreeModel* model  = gtk_combo_box_get_model(combo_box);
    GtkTreePath*  path   = gtk_tree_path_new_from_indices(active, -1);
    GtkTreeIter   iter;
    if (gtk_combo_box_get_active_iter(combo_box, &iter)) {
        print_tree_selection(model, path, &iter, NULL);
    }
}



/*
 * GtkContainer => GtkBox => GtkVBox => GtkColorSelection
 */

/**
 * Print the color currently selected in a color selection widget.
 * @param color_selection color selection widget to print information about
 */
void mk_print_GtkColorSelection_info(GtkColorSelection* color_selection)
{
    GdkColor color;
    gtk_color_selection_get_current_color(color_selection, &color);
    guint16 alpha = gtk_color_selection_get_current_alpha(color_selection);

    g_printf("\t%d,%d,%d,%d",
             color.red, color.green, color.blue, alpha);    
}



/*
 * GtkButton => GtkColorButton
 */

/**
 * Print the color currently selected by a color button.
 * @param color_button color button to print information about
 */
void mk_print_GtkColorButton_info(GtkColorButton* color_button)
{
    GdkColor color;
    gtk_color_button_get_color(color_button, &color);
    guint16 alpha = gtk_color_button_get_alpha(color_button);

    g_printf("\t%d,%d,%d,%d",
             color.red, color.green, color.blue, alpha);
}



/*
 * GtkWindow => GtkDialog => GtkColorSelectionDialog
 */

/**
 * Print the color currently selected by a color selection dialog.
 * @param dialog selection dialog to print information about
 */
void mk_print_GtkColorSelectionDialog_info(GtkColorSelectionDialog* dialog)
{
#if GTK_CHECK_VERSION(2,14,0)
    GtkColorSelection* color_sel;
    color_sel = (GtkColorSelection*)
        gtk_color_selection_dialog_get_color_selection(dialog);
    mk_print_GtkColorSelection_info(color_sel);
#endif
}



/*
 * GtkButton => GtkFontButton
 */

/**
 * Print the font currently selected by a font button.
 * @param font_button font button to print information about
 */
void mk_print_GtkFontButton_info(GtkFontButton* font_button)
{
    const gchar* font_name = gtk_font_button_get_font_name(font_button);
    g_printf("\t%s", font_name);
}



/*
 * GtkContainer => GtkBox => GtkVBox => GtkFontSelection
 */

/**
 * Print the font currently selected by a font selection widget.
 * @param font_selection font selection widget to print information about
 */
void mk_print_GtkFontSelection_info(GtkFontSelection* font_selection)
{
    const gchar* font_name = gtk_font_selection_get_font_name(font_selection);
    g_printf("\t%s", font_name);
}



/*
 * GtkWindow => GtkDialog => GtkFontSelectionDialog
 */

/**
 * Print the font currently selected by a font selection dialog.
 * @param dialog font selection dialog to print information about
 */
void mk_print_GtkFontSelectionDialog_info(GtkFontSelectionDialog* dialog)
{
    gchar* font_name = gtk_font_selection_dialog_get_font_name(dialog);
    g_printf("\t%s", font_name);
    g_free(font_name);
}



/*
 * GtkButton => GtkLinkButton
 */

/**
 * Print the target uri of a link button.
 * @param link_button link button to print information about
 */
void mk_print_GtkLinkButton_info(GtkLinkButton* link_button)
{
    const gchar* uri = gtk_link_button_get_uri(link_button);
    g_printf("\t%s", uri);
}



/*
 * GtkFileChooser
 */

/**
 * Print the file selected by a file chooser.
 * @param file_chooser file chooser to print information about
 */
void mk_print_GtkFileChooser_info(GtkFileChooser* file_chooser)
{
    gchar* filename;
    filename = gtk_file_chooser_get_filename(file_chooser);
    if (filename != NULL) {
        g_printf("\t%s", filename);
        g_free(filename);
    }
}



/*
 * GtkContainer => GtkBox => GtkVBox => GtkFileChooserButton
 */

/**
 * Print the file selected by a file chooser button.
 * @param button file chooser button to print information about
 */
void mk_print_GtkFileChooserButton_info(GtkFileChooserButton* button)
{
    mk_print_GtkFileChooser_info(GTK_FILE_CHOOSER(button));
}



/*
 * GtkContainer => GtkBox => GtkVBox => GtkFileChooserWidget
 */

/**
 * Print the file selected by a file chooser widget.
 * @param widget file chooser widget to print information about
 */
void mk_print_GtkFileChooserWidget_info(GtkFileChooserWidget* widget)
{
    mk_print_GtkFileChooser_info(GTK_FILE_CHOOSER(widget));
}



/*
 * GtkWindow => GtkDialog => GtkFileChooserDialog
 */

/**
 * Print the file selected by a file chooser dialog.
 * @param dialog file chooser dialog to print information about
 */
void mk_print_GtkFileChooserDialog_info(GtkFileChooserDialog* dialog)
{
    mk_print_GtkFileChooser_info(GTK_FILE_CHOOSER(dialog));
}



/*
 * GtkContainer => GtkNotebook
 */

/**
 * Print the current page and page count of a notebook.
 * @param notebook notebook to print information about
 */
void mk_print_GtkNotebook_info(GtkNotebook* notebook)
{
    gint current_page = gtk_notebook_get_current_page(notebook);
    gint n_pages = gtk_notebook_get_n_pages(notebook);
    g_printf("\t%d/%d", current_page, n_pages);
}



/*
 * GtkAdjustment
 */

/**
 * Print the current value and the range of an adjustment.
 * @param adjustment adjustment to print information about
 */
void mk_print_GtkAdjustment_info(GtkAdjustment* adjustment)
{
    gdouble value = gtk_adjustment_get_value(adjustment);

#if GTK_CHECK_VERSION(2,14,0)
    gdouble lower = gtk_adjustment_get_lower(adjustment);
    gdouble upper = gtk_adjustment_get_upper(adjustment);
    g_printf("\t%f\t%f-%f", value, lower, upper);
#else
    g_printf("\t%f", value);
#endif
}



/*
 * GtkCalendar
 */

/**
 * Print the selected date on a calendar.
 * @param calendar calendar to print information about
 */
void mk_print_GtkCalendar_info(GtkCalendar* calendar)
{
    guint year;
    guint month;
    guint day;
    gtk_calendar_get_date(calendar, &year, &month, &day);
    g_printf("\t%d-%02d-%02d", year, month+1, day); // YYYY-MM-DD
}



/*
 * GtkRecentChooser
 */

/**
 * Print the uri of the selected item in a recent file chooser.
 * @param recent_chooser recent file chooser to print information about
 */
void mk_print_GtkRecentChooser_info(GtkRecentChooser* recent_chooser)
{
    GtkRecentInfo* info;
    info = gtk_recent_chooser_get_current_item(recent_chooser);
    if (info != NULL) {
        const gchar* uri = gtk_recent_info_get_uri(info);
        g_printf("\t%s", uri);
        gtk_recent_info_unref(info);
    }
}



/*
 * GtkWindow => GtkDialog => GtkRecentChooserDialog
 */

/**
 * Print the uri of the selected item in a recent file dialog.
 * @param dialog recent file dialog to print information about
 */
void mk_print_GtkRecentChooserDialog_info(GtkRecentChooserDialog* dialog)
{
    mk_print_GtkRecentChooser_info(GTK_RECENT_CHOOSER(dialog));
}



/*
 * GtkContainer => GtkMenuShell => GtkMenu => GtkRecentChooserMenu
 */


/**
 * Print the uri of the selected item in a recent file menu.
 * @param menu recent file menu to print information about
 */
void mk_print_GtkRecentChooserMenu_info(GtkRecentChooserMenu* menu)
{
    mk_print_GtkRecentChooser_info(GTK_RECENT_CHOOSER(menu));
}



/*
 * GtkContainer => GtkBox => GtkVBox => GtkRecentChooserWidget
 */

/**
 * Print the uri of the selected item in a recent file widget.
 * @param widget recent file menu to print information about
 */
void mk_print_GtkRecentChooserWidget_info(GtkRecentChooserWidget* widget)
{
    mk_print_GtkRecentChooser_info(GTK_RECENT_CHOOSER(widget));
}
