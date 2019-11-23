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

#ifndef _NOTLIB_INTERNAL_H
#define _NOTLIB_INTERNAL_H

#include <glib.h>
#include <stdio.h>
#include "notlib.h"

#define FDN_PATH "/org/freedesktop/Notifications"
#define FDN_IFAC "org.freedesktop.Notifications"
#define FDN_NAME "org.freedesktop.Notifications"

#define DBUS_VERSION "1.2"

extern void *ealloc(size_t);

enum CloseReason {
    CLOSE_REASON_MIN        = 1,
    CLOSE_REASON_EXPIRED    = 1,
    CLOSE_REASON_DISMISSED  = 2,
    CLOSE_REASON_CLOSED     = 3,
    CLOSE_REASON_UNKNOWN    = 4,
    CLOSE_REASON_MAX        = 4
};

extern NLNoteCallbacks callbacks;
extern NLServerInfo *server_info;
extern char **server_capabilities;

struct hints {
    GVariantDict *dict;
};

// queue.c

void enqueue_note(NLNote *note, char *sender);
void dequeue_note(uint32_t id, enum CloseReason);

// idrange.c

extern void claim_id(uint32_t);
extern uint32_t get_unclaimed_id();

// note.c

extern NLNote *new_note(uint32_t,     /* id */
                        char *,       /* app name */
                        char *,       /* summary */
                        char *,       /* body */
#if NL_ACTIONS
                        NLActions *,    /* actions */
#endif
#if NL_URGENCY
                        enum NLUrgency, /* urgency */
#endif
                        NLHints *,      /* hints */
                        int32_t       /* timeout */
                       );

#if NL_ACTIONS
extern void free_actions(NLActions *);
#endif
extern void free_note(NLNote *);
extern int32_t note_timeout(const NLNote *);

// dbus.c

extern void signal_notification_closed(NLNote *, const char *, enum CloseReason);
extern void signal_action_invoked(NLNote *, const char *, const char *);
extern void run_dbus_loop();

#endif  // _NOTLIB_INTERNAL_H
