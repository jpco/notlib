/* Copyright 2018 Jack Conger */

/*
 * This file is part of notlib.
 *
 * notlib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * notlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with notlib.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * The public API for notlib.
 */

#ifndef NOTLIB_H
#define NOTLIB_H

#define NL_VERSION 0.2

/*
 * Optional features
 */

#ifndef NL_ACTIONS
#define NL_ACTIONS 1
#endif

#ifndef NL_URGENCY
#define NL_URGENCY 1
#endif

// TODO: more optional features
//  - ICON

#if NL_ACTIONS
typedef struct {
    char **actions;
    size_t count;
} NLActions;
#endif

#if NL_URGENCY
enum NLUrgency {
    URG_NONE = -1,
    URG_MIN  =  0,
    URG_LOW  =  0,
    URG_NORM =  1,
    URG_CRIT =  2,
    URG_MAX  =  2
};
#endif

typedef struct hints NLHints;

enum NLHintType {
    HINT_TYPE_UNKNOWN = 0,
    HINT_TYPE_INT = 1,
    HINT_TYPE_BYTE = 2,
    HINT_TYPE_BOOLEAN = 3,
    HINT_TYPE_STRING = 4
};

typedef struct {
    enum NLHintType type;
    union {
        int i;
        unsigned char byte;
        int bl;
        const char *str;
    } d;
} NLHint;

typedef struct {
    unsigned int id;
    char *appname;
    char *summary;
    char *body;

    int timeout;
#if NL_ACTIONS
    NLActions *actions;
#endif

#if NL_URGENCY
    enum NLUrgency urgency;
#endif
    NLHints *hints;
} NLNote;

typedef struct {
    void (*notify)  (const NLNote *);
    void (*close)   (const NLNote *);  // Should this include CloseReason?
    void (*replace) (const NLNote *);
} NLNoteCallbacks;

typedef struct {
    char *app_name;
    char *author;
    char *version;
} NLServerInfo;

/* public functions */

/*
 * Hint accessors. These don't allow for every possible DBUS_TYPE_VARIANT, but
 * they do cover the common cases.
 */

// Type-generic hint accessors
extern enum NLHintType nl_get_hint_type(const NLNote *n, const char *key);
extern int nl_get_hint(const NLNote *n, const char *key, NLHint *out);

// Type-specific hint accessors
extern int nl_get_int_hint    (const NLNote *n, const char *key, int *out);
extern int nl_get_byte_hint   (const NLNote *n, const char *key, unsigned char *out);
extern int nl_get_boolean_hint(const NLNote *n, const char *key, int *out);
extern int nl_get_string_hint (const NLNote *n, const char *key, const char **out);

/*
 * Main entry point.
 *
 * TODO: describe semantics of how the callbacks get called
 * (e.g., concurrency? one-at-a-time? what's the deal?)
 *
 * TODO: allow client to provide information for GetServerInformation
 */
extern void notlib_run(NLNoteCallbacks, NLServerInfo*);

extern void nl_close_note(unsigned int);

#if NL_ACTIONS
// extern void nl_invoke_action(unsigned int, char *);
// char *nl_action_keys(NLNote *);
// char *nl_action_name(NLNote *, char *);
#endif

#endif
