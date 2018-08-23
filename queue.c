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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "notlib.h"
#include "_notlib_internal.h"

typedef struct qn {
    Note *n;
    gint64 exp;
    gchar *client;

    struct qn *prev;
    struct qn *next;
} qnode;

static qnode *note_queue_start = NULL;
static qnode *note_queue_end = NULL;

static void dequeue_note_by_node(qnode *qn, enum CloseReason reason) {
    if (qn->prev != NULL) {
        qn->prev->next = qn->next;
    } else {
        note_queue_start = qn->next;
    }

    if (qn->next != NULL) {
        qn->next->prev = qn->prev;
    } else {
        note_queue_end = qn->prev;
    }

    signal_notification_closed(qn->n, qn->client, reason);
    if (callbacks.close != NULL)
        callbacks.close(qn->n);

    free_note(qn->n);
    g_free(qn->client);
    free(qn);
}

extern void dequeue_note(uint32_t id, enum CloseReason reason) {
    qnode *qn;
    for (qn = note_queue_start; qn; qn = qn->next) {
        if (qn->n->id == id) {
            dequeue_note_by_node(qn, reason);
            return;
        }
    }
}

static gboolean wakeup_queue(gpointer p) {
    gint64 curr_time = g_get_monotonic_time() / 1000;

    qnode *qn, *qnext;
    for (qn = note_queue_start; qn; qn = qnext) {
        qnext = qn->next;
        if (qn->exp && qn->exp <= curr_time)
            dequeue_note_by_node(qn, CLOSE_REASON_EXPIRED);
    }

    return FALSE;
}

static void setup_qnode(qnode *qn, Note *n, gchar *client) {
    qn->n = n;
    qn->client = client;

    int32_t timeout_millis = note_timeout(n);
    if (!timeout_millis) {
        qn->exp = 0;
        return;
    }

    qn->exp = (g_get_monotonic_time() / 1000) + timeout_millis;
    g_timeout_add(timeout_millis, wakeup_queue, NULL);
}

static int replace_note(Note *n, gchar *client) {
    qnode *qn;
    for (qn = note_queue_start; qn; qn = qn->next) {
        if (qn->n->id == n->id) {
            free_note(qn->n);
            if (callbacks.replace != NULL)
                callbacks.replace(n);
            setup_qnode(qn, n, client);
            return TRUE;
        }
    }
    return FALSE;
}

extern void enqueue_note(Note *n, gchar *client) {
    if (n->id && replace_note(n, client))
        return;

    // literally DO we care about running out of memory?
    qnode *new_qn = malloc(sizeof(qnode));

    new_qn->next = NULL;
    if (note_queue_start == NULL) {
        note_queue_start = new_qn;
        new_qn->prev = NULL;
        note_queue_end = new_qn;
    } else {
        note_queue_end->next = new_qn;
        new_qn->prev = note_queue_end;
        note_queue_end = new_qn;
    }

    callbacks.notify(n);
    setup_qnode(new_qn, n, client);
}
