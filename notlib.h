/* Copyright 2018 Jack Conger */

/*
 * This file is part of notlib.
 *
 * notlib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * notlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with notlib.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * The public API for notlib.
 */

#ifndef NOTLIB_H
#define NOTLIB_H

#define VERSION 0.2

// By default, say we support actions
#ifndef ACTIONS
#define ACTIONS 1
#endif

// TODO: more optional features
//  - ICON
//  - URGENCY, to treat URGENCY specially

#if ACTIONS
typedef struct {
    char **actions;
    size_t count;
} Actions;
#endif

typedef struct {
    unsigned int id;
    char *appname;
    char *summary;
    char *body;

    int timeout;
#if ACTIONS
    Actions *actions;
#endif

    // TODO: hints
} Note;

typedef struct {
    void (*notify)  (const Note *);
    void (*close)   (const Note *);  // Should this include CloseReason?
    void (*replace) (const Note *);
} NoteCallbacks;

/* public functions */

/*
 * Main entry point.
 *
 * TODO: describe semantics of how the callbacks get called
 * (e.g., concurrency? one-at-a-time? what's the deal?)
 *
 * TODO: allow client to provide information for GetServerInformation
 */
extern void notlib_run(NoteCallbacks);

// TODO: allow client to close notes (would the close callback get called?)

#if ACTIONS
// TODO: allow client to interact with actions
#endif

#endif
