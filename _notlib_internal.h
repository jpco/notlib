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

#ifndef _NOTLIB_INTERNAL_H
#define _NOTLIB_INTERNAL_H

#define FDN_PATH "/org/freedesktop/Notifications"
#define FDN_IFAC "org.freedesktop.Notifications"
#define FDN_NAME "org.freedesktop.Notifications"

#define DBUS_VERSION "1.2"

enum CloseReason {
    CLOSE_REASON_MIN        = 1,
    CLOSE_REASON_EXPIRED    = 1,
    CLOSE_REASON_DISMISSED  = 2,
    CLOSE_REASON_CLOSED     = 3,
    CLOSE_REASON_UNKNOWN    = 4,
    CLOSE_REASON_MAX        = 4
};

extern NoteCallbacks callbacks;
extern ServerInfo *server_info;

// queue.c

void enqueue_note(Note *note, gchar *sender);
void dequeue_note(uint32_t id, enum CloseReason);

// note.c

extern Note *new_note(uint32_t,     /* id */
                      char *,       /* app name */
                      char *,       /* summary */
                      char *,       /* body */
#if ACTIONS
                      Actions *,    /* actions */
#endif
#if URGENCY
                      enum Urgency, /* urgency */
#endif
                      int32_t       /* timeout */
                      /* TODO: hints */
                     );

#if ACTIONS
extern void free_actions(Actions *actions);
#endif
extern void free_note(Note *);
extern int32_t note_timeout(const Note *);

// dbus.c

extern void signal_notification_closed(Note *, const gchar *, enum CloseReason);
extern void signal_action_invoked(Note *, const gchar *, const char *);
extern void run_dbus_loop();

#endif  // _NOTLIB_INTERNAL_H
