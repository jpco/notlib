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
 * "Each notification displayed is allocated a unique ID by the server.
 * This is unique within the session. While the notification server is
 * running, the ID will not be recycled unless the capacity of a uint32 is
 * exceeded." - https://developer.gnome.org/notification-spec/#basic-design
 *
 * If a notification uses a replaces_id which has not been used before, the
 * server should avoid using that id again until it has exhausted the uint32
 * space.  This file makes that happen.
 */

#include <stdint.h>

#include "_notlib_internal.h"

typedef struct range {
    uint32_t min;
    uint32_t max;
    struct range *next;
} range;

const static uint32_t MAX = ~0;
static range *r = NULL;

// Claims the given ID, making it ineligible to be returned by get_unclaimed_id
// until the entire uint32 space has been returned.
extern void claim_id(uint32_t n) {
    if (n == 0)
        return;

    if (r == NULL) {
        r = ealloc(sizeof(range));
        r->min = n;
        r->max = n;
        r->next = NULL;
        return;
    }

    range *cr, *prevr = NULL;
    for (cr = r; cr != NULL; prevr = cr, cr = cr->next) {
        if (n < cr->min - 1) {
            range *nr = ealloc(sizeof(range));
            nr->min = n;
            nr->max = n;
            nr->next = cr;
            if (cr == r) {
                r = nr;
            } else {
                prevr->next = nr;
            }
            return;
        } else if (n == cr->min - 1) {
            cr->min--;
            return;
        } else if (n >= cr->min && n <= cr->max) {
            return;
        } else if (n == cr->max + 1) {
            cr->max++;
            if (cr->next != NULL && n == cr->next->min - 1) {
                range *vr = cr->next;
                cr->next = vr->next;
                cr->max = vr->max;
                free(vr);
            }
            return;
        }
    }
    range *nr = ealloc(sizeof(range));
    nr->min = n;
    nr->max = n;
    nr->next = NULL;
    prevr->next = nr;
}

// Claims and returns the lowest unclaimed ID.  If no such ID exists within
// the uint32 space, clears out all current claims and then claims/returns 1.
extern uint32_t get_unclaimed_id() {
    if (r == NULL || r->min > 1) {
        claim_id(1);
        return 1;
    }
    if (r->max == MAX) {
        free(r);
        r = NULL;
        claim_id(1);
        return 1;
    }
    uint32_t ret = r->max + 1;
    claim_id(ret);
    return ret;
}
