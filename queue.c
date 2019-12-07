/* Copyright 2019 Jack Conger */

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

#include <stdint.h>

#include "notlib.h"
#include "_notlib_internal.h"


/**
 * CORE DATA STRUCTURE -- qnode
 */

#define QUEUE_NOTIFY (CLOSE_REASON_MAX + 1)

#define LOCKED(queue, expr) do { \
    pthread_mutex_lock(&(queue).lock); \
    (expr); \
    pthread_mutex_unlock(&(queue).lock); \
} while (0);

typedef struct qn {
    NLNote *n;
    uint32_t id;

    int64_t exp;
    int action;
    struct qn *prev;
    struct qn *next;
} qnode;

typedef struct {
    pthread_mutex_t lock;
    qnode *start;
    qnode *end;
} queue;

/**
 * QUEUES
 *  - notify queue: changes that need to be propagated
 *  - timeout queue: notifications waiting to expire
 */

pthread_cond_t nq_cond = PTHREAD_COND_INITIALIZER;

queue notify_queue = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .start = NULL,
    .end = NULL
};
queue timeout_queue = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .start = NULL,
    .end = NULL
};


NLNoteCallbacks callbacks;

/* Callers MUST lock the queue's mutex before calling!! */
static void queue_insert(queue *q, qnode *qn) {
    qn->next = NULL;
    if (q->start == NULL) {
        qn->prev = NULL;
        q->start = qn;
        q->end = qn;
    } else {
        q->end->next = qn;
        qn->prev = q->end;
        q->end = qn;
    }
}

static qnode *queue_find_id(queue *q, uint32_t id) {
    qnode *qn;
    for (qn = q->start; qn; qn = qn->next) {
        if (qn->id == id)
            return qn;
    }
    return NULL;
}

static void queue_yank(queue *q, qnode *qn) {
    if (qn->prev != NULL) {
        qn->prev->next = qn->next;
    } else {
        q->start = qn->next;
    }
    if (qn->next != NULL) {
        qn->next->prev = qn->prev;
    } else {
        q->end = qn->prev;
    }
}

/* Callers MUST lock the queue's mutex before calling!! */
static qnode *queue_yank_first(queue *q) {
    qnode *qn = q->start;
    if (qn == NULL)
        return NULL;
    queue_yank(q, qn);
    return qn;
}

/* Callers MUST lock the queue's mutex before calling!! */
static qnode *queue_yank_id(queue *q, uint32_t id) {
    qnode *qn = queue_find_id(q, id);
    if (qn == NULL)
        return NULL;
    queue_yank(q, qn);
    return qn;
}


/**
 * CALLBACK THREADS
 */

static int scan_for_timeout(gpointer p);
static void enqueue(qnode *qn, int action);

static void do_notify(qnode *qn) {
    qnode *replaced = NULL;
    LOCKED(timeout_queue, {
        replaced = queue_yank_id(&timeout_queue, qn->id);
    });

    if (replaced != NULL && callbacks.replace != NULL) {
        callbacks.replace(qn->n);
    } else if (replaced == NULL && callbacks.notify != NULL) {
        callbacks.notify(qn->n);
    }

    LOCKED(timeout_queue, queue_insert(&timeout_queue, qn));

    if (replaced != NULL) {
        free_note(replaced->n);
        free(replaced);
    }

    int32_t timeout_ms = note_timeout(qn->n);
    if (timeout_ms == 0) {
        qn->exp = 0;
    } else {
        qn->exp = (g_get_monotonic_time() / 1000) + timeout_ms;
        g_timeout_add(timeout_ms, scan_for_timeout, NULL);
    }
}

static void do_close(qnode *qn) {
    qnode *closed = NULL;
    if (qn->n != NULL) {
        closed = qn;
    } else {
        LOCKED(notify_queue, {
            closed = queue_yank_id(&notify_queue, qn->id);
        });
        if (closed == NULL) {
            LOCKED(timeout_queue, {
                closed = queue_yank_id(&timeout_queue, qn->id);
            });
        }
    }
    if (closed != NULL) {
        if (callbacks.close != NULL)
            callbacks.close(closed->n);
        signal_notification_closed(closed->n->id, qn->action);
        if (closed != qn) {
            free_note(closed->n);
            free(closed);
        }
    }

    if (qn->n != NULL)
        free_note(qn->n);
    free(qn);
}

extern void *queue_listen(void *_) {
    while (1) {
        qnode *qn;
        LOCKED(notify_queue, {
            while (notify_queue.start == NULL)
                pthread_cond_wait(&nq_cond, &notify_queue.lock);
            qn = queue_yank_first(&notify_queue);
        });

        if (qn->action == QUEUE_NOTIFY) {
            /* fresh notification! */
            do_notify(qn);
        } else {
            /* closed ... probably */
            do_close(qn);
        }
    }
    return NULL;
}


/**
 * D-BUS THREAD
 */

static int scan_for_timeout(gpointer p) {
    int64_t current_time = g_get_monotonic_time() / 1000;
    qnode *qn, *qnext;

    LOCKED(timeout_queue, {
        for (qn = timeout_queue.start; qn; qn = qnext) {
            qnext = qn->next;
            if (!qn->exp || qn->exp > current_time)
                continue;

            queue_yank(&timeout_queue, qn);
            enqueue(qn, CLOSE_REASON_EXPIRED);
        }
    });

    /* keeps this function from being called over and over */
    return G_SOURCE_REMOVE;
}

static void enqueue(qnode *qn, int action) {
    qn->action = action;

    LOCKED(notify_queue, {
        queue_insert(&notify_queue, qn);
        pthread_cond_broadcast(&nq_cond);
    });
}

extern void queue_notify(NLNote *n) {
    qnode *qn = ealloc(sizeof(qnode));
    qn->n = n;
    qn->id = n->id;
    qn->exp = 0;
    enqueue(qn, QUEUE_NOTIFY);
}

extern void queue_close(uint32_t id, enum CloseReason reason) {
    qnode *qn = ealloc(sizeof(qnode));
    qn->n = NULL;
    qn->id = id;
    qn->exp = 0;
    enqueue(qn, reason);
}

extern int queue_call(uint32_t id, int (*callback)(const NLNote *, void *), void *data) {
    qnode *qn = NULL;
    pthread_mutex_t *m = NULL;

    pthread_mutex_lock(&notify_queue.lock);
    qn = queue_find_id(&notify_queue, id);
    if (qn != NULL) {
        m = &notify_queue.lock;
    } else {
        pthread_mutex_unlock(&notify_queue.lock);

        pthread_mutex_lock(&timeout_queue.lock);
        qn = queue_find_id(&timeout_queue, id);
        if (qn != NULL) {
            m = &timeout_queue.lock;
        } else {
            pthread_mutex_unlock(&timeout_queue.lock);
        }
    }

    int result;
    if (qn != NULL) {
        result = callback(qn->n, data);
        pthread_mutex_unlock(m);
    } else {
        result = callback(NULL, data);
    }
    return result;
}
