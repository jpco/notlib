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

#include <stdlib.h>
#include <glib.h>
#include <stdint.h>

#include "notlib.h"
#include "_notlib_internal.h"

extern Note *new_note(uint32_t id, char *appname,
                      char *summary, char *body,
#if ACTIONS
                      Actions *actions,
#endif
                      int32_t timeout /* TODO: hints */) {
    Note *n = g_malloc(sizeof(Note));

    n->id      = id;
    n->appname = appname;
    n->summary = summary;
    n->body    = body;
    n->timeout = timeout;
#if ACTIONS
    n->actions = actions;
#endif
    // TODO: hints
    return n;
}

#if ACTIONS
extern void free_actions(Actions *a) {
    if (!a) return;

    g_strfreev(a->actions);
    g_free(a);
}
#endif

extern void free_note(Note *n) {
    if (!n) return;

    g_free(n->appname);
    g_free(n->summary);
    g_free(n->body);

#if ACTIONS
    free_actions(n->actions);
#endif

    // TODO: hints
    g_free(n);
}

extern int32_t note_timeout(const Note *n) {
    if (n->timeout >= 0)
        return n->timeout;

    return 5000;  // TODO: make this configurable
}
