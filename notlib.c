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
 *
 * GDBus logic has been liberally adapted from
 * https://github.com/dunst-project/dunst/blob/master/src/dbus.c
 *
 * Desktop notifications API details can be found at
 * https://developer.gnome.org/notification-spec/
 */

#include <glib.h>
#include <gio/gio.h>
#include <stdint.h>
#include <stdio.h>

#include "notlib.h"
#include "_notlib_internal.h"

extern void nl_close_note(unsigned int id) {
    dequeue_note(id, CLOSE_REASON_DISMISSED);
}

extern void notlib_run(NLNoteCallbacks cbs, NLServerInfo *info) {
    callbacks = cbs;
    server_info = info;
    run_dbus_loop();
}
