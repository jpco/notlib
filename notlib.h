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

/*
 * Optional features
 */

#ifndef ACTIONS
#define ACTIONS 1
#endif

#ifndef URGENCY
#define URGENCY 1
#endif

// TODO: more optional features
//  - ICON

#if ACTIONS
typedef struct {
    char **actions;
    size_t count;
} Actions;
#endif

#if URGENCY
enum Urgency {
    URG_NONE = -1,
    URG_MIN  =  0,
    URG_LOW  =  0,
    URG_NORM =  1,
    URG_CRIT =  2,
    URG_MAX  =  2
};
#endif

typedef struct hints Hints;

typedef struct {
    unsigned int id;
    char *appname;
    char *summary;
    char *body;

    int timeout;
#if ACTIONS
    Actions *actions;
#endif

#if URGENCY
    enum Urgency urgency;
#endif
    Hints *hints;
} Note;

typedef struct {
    void (*notify)  (const Note *);
    void (*close)   (const Note *);  // Should this include CloseReason?
    void (*replace) (const Note *);
} NoteCallbacks;

typedef struct {
    char *app_name;
    char *author;
    char *version;
} ServerInfo;

/* public functions */

/*
 * Hint accessors.
 *
 * Doesn't allow for every possible DBUS_TYPE_VARIANT, but covers
 * the common cases.  If the hint value associated with "key" is not of
 * the requested type, the type's obvious 'zero value' will be returned.
 *
 * For safety, the return value of get_string_hint() is a copy and must
 * be freed by the caller.
 */

enum HintType {
    HINT_TYPE_UNKNOWN = 0,
    HINT_TYPE_INT = 1,
    HINT_TYPE_BYTE = 2,
    HINT_TYPE_BOOLEAN = 3,
    HINT_TYPE_STRING = 4
};

extern enum HintType get_hint_type(const Note *n, char *key);

extern int get_int_hint     (const Note *n, char *key, int *out);
extern int get_byte_hint    (const Note *n, char *key, unsigned char *out);
extern int get_boolean_hint (const Note *n, char *key, int *out);
extern int get_string_hint  (const Note *n, char *key, char **out);

/*
 * Main entry point.
 *
 * TODO: describe semantics of how the callbacks get called
 * (e.g., concurrency? one-at-a-time? what's the deal?)
 *
 * TODO: allow client to provide information for GetServerInformation
 */
extern void notlib_run(NoteCallbacks, ServerInfo*);

extern void close_note(unsigned int);

#if ACTIONS
// extern void invoke_action(unsigned int, char *);
// char *action_keys(Note *);
// char *action_name(Note *, char *);
#endif

#endif
