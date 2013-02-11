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
 * State machine transitions.
 * 
 * A transition is a way to pass from one state to another one when a
 * signal is received. An optional output can be associated with a
 * transition.
 */

#ifndef __TRANSITION_H__
#define __TRANSITION_H__

#include <glib.h>

/**
 * A transition is a way to change the current state from one state to
 * another. For a transition to happen, the current state must be its
 * source state and the transition's signal (a string) must be received.
 * When this happens, the transition's output is written and the current
 * state becomes the transition's destination state.
 *
 * @brief Transition transition from one state to another.
 */
typedef struct {
    gchar* src_state; /// Source state
    gchar* signal;    /// Expression that triggers the transition
    gchar* dst_state; /// Destination state
    gchar* output;    /// Output text when the transition occurs
} MkTransition;


/**
 * Create a new Transition.
 * @param src_state source state
 * @param signal    signal (a regex) that, if received while in src_state,
 *                  will bring us to dst_state
 * @param dst_state destination state
 * @param output    text that must be output when the transition happens
 * @return          a new Transition that must be freed by
 *                  mk_transition_delete().
 */
MkTransition* mk_transition_new(const gchar* src_state,
                                const gchar* signal,
                                const gchar* dst_state,
                                const gchar* output);

/**
 * Delete a transition.
 * @param t the transition
 */
void mk_transition_delete(MkTransition* t);

/**
 * Add a transition to the transition table.
 * @param table transition table
 * @param t     the transition
 */
void mk_transition_add(GHashTable* table, MkTransition* t);

/**
 * Find a transition in the list.
 * @param table  transition table
 * @param src_state source state
 * @param signal signal (a regex) that, if received while in src_state,
 *               will bring us to dst_state
 * @param output return value of the output string, with occurences of \n
 *               replaced by their corresponding captured strings in the
 *               regular expression (signal)
 * @return       the transition, or NULL if not found
 */
MkTransition* mk_transition_lookup(GHashTable* table,
                                   const gchar* src_state,
                                   const gchar* signal,
                                   gchar** output);

/**
 * Delete the list of transitions.
 * @param array a GPtrArray full of Transitions.
 */
void mk_transition_list_delete(GPtrArray* array);

#endif // __TRANSITION_H__
