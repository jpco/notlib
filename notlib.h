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
 *
 * The public API for notlib.
 */

#ifndef NOTLIB_H
#define NOTLIB_H

#include <stdlib.h>

#define NL_VERSION 0.2

/*
 * Optional features
 */

#ifndef NL_ACTIONS
#define NL_ACTIONS 1
#endif

#ifndef NL_REMOTE_ACTIONS
#define NL_REMOTE_ACTIONS 0
#endif

#ifndef NL_URGENCY
#define NL_URGENCY 1
#endif

#if NL_ACTIONS
typedef struct {
    char **actions;
    char **keys;
    char **names;
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
    char **capabilities;
} NLServerInfo;

/* public functions */

/*
 * Hint accessors. These don't allow for every possible DBUS_TYPE_VARIANT, but
 * they do cover the common cases.
 */

// Type-generic hint accessors
extern enum NLHintType nl_get_hint_type(const NLNote *n, const char *key);
extern int nl_get_hint(const NLNote *n, const char *key, NLHint *out);
extern char *nl_get_hint_as_string(const NLNote *n, const char *key);

// Type-specific hint accessors
extern int nl_get_int_hint     (const NLNote *n, const char *key, int *out);
extern int nl_get_byte_hint    (const NLNote *n, const char *key, unsigned char *out);
extern int nl_get_boolean_hint (const NLNote *n, const char *key, int *out);
extern int nl_get_string_hint  (const NLNote *n, const char *key, const char **out);

/*
 * Interacting with actions.
 */

#if NL_ACTIONS
// Invokes an action only if the given ID refers to an open notification, and
// the key is "default" or was specified by the notification.  Returns truthy
// if an action was, indeed, invoked.
extern int nl_invoke_action        (unsigned int id, const char *key);

extern const char **nl_action_keys (const NLNote *);
extern const char *nl_action_name  (const NLNote *, const char *);
#endif

/*
 * Main entry point(s).
 *
 * Notlib generates a new pthread to listen for D-Bus messages and queue events.
 * Callbacks are made synchronously in the same thread which invokes notlib_run.
 */
extern void notlib_run(NLNoteCallbacks, char **, NLServerInfo*);

extern void nl_close_note(unsigned int);
extern void nl_set_default_timeout(unsigned int);

#endif
