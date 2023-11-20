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
 * GDBus logic has been liberally adapted from
 * https://github.com/dunst-project/dunst/blob/master/src/dbus.c
 *
 * Desktop notifications API details can be found at
 * https://developer.gnome.org/notification-spec/
 */

#include <gio/gio.h>
#include <stdint.h>

#include "notlib.h"
#include "_notlib_internal.h"

static GDBusConnection *dbus_conn;
static GDBusNodeInfo *introspection_data = NULL;

static const char *dbus_introspection_xml =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<node name=\"" FDN_PATH "\">"
    "    <interface name=\"" FDN_IFAC "\">"

    "        <method name=\"GetCapabilities\">"
    "            <arg direction=\"out\" name=\"capabilities\"    type=\"as\"/>"
    "        </method>"

    "        <method name=\"Notify\">"
    "            <arg direction=\"in\"  name=\"app_name\"        type=\"s\"/>"
    "            <arg direction=\"in\"  name=\"replaces_id\"     type=\"u\"/>"
    "            <arg direction=\"in\"  name=\"app_icon\"        type=\"s\"/>"
    "            <arg direction=\"in\"  name=\"summary\"         type=\"s\"/>"
    "            <arg direction=\"in\"  name=\"body\"            type=\"s\"/>"
    "            <arg direction=\"in\"  name=\"actions\"         type=\"as\"/>"
    "            <arg direction=\"in\"  name=\"hints\"           type=\"a{sv}\"/>"
    "            <arg direction=\"in\"  name=\"expire_timeout\"  type=\"i\"/>"
    "            <arg direction=\"out\" name=\"id\"              type=\"u\"/>"
    "        </method>"

    "        <method name=\"CloseNotification\">"
    "            <arg direction=\"in\"  name=\"id\"              type=\"u\"/>"
    "        </method>"

    "        <method name=\"GetServerInformation\">"
    "            <arg direction=\"out\" name=\"name\"            type=\"s\"/>"
    "            <arg direction=\"out\" name=\"vendor\"          type=\"s\"/>"
    "            <arg direction=\"out\" name=\"version\"         type=\"s\"/>"
    "            <arg direction=\"out\" name=\"spec_version\"    type=\"s\"/>"
    "        </method>"

#if NL_ACTIONS && NL_REMOTE_ACTIONS

    "        <method name=\"InvokeAction\">"
    "            <arg direction=\"in\" name=\"id\"  type=\"u\"/>"
    "            <arg direction=\"in\" name=\"key\" type=\"s\"/>"
    "        </method>"

#endif

    "        <signal name=\"NotificationClosed\">"
    "            <arg name=\"id\"         type=\"u\"/>"
    "            <arg name=\"reason\"     type=\"u\"/>"
    "        </signal>"

    "        <signal name=\"ActionInvoked\">"
    "            <arg name=\"id\"         type=\"u\"/>"
    "            <arg name=\"action_key\" type=\"s\"/>"
    "        </signal>"

    "   </interface>"
    "</node>";

/**
 * DBus method call logic
 */

char **server_capabilities;

static void get_capabilities(GDBusConnection *conn, const char *sender,
                             const GVariant *params,
                             GDBusMethodInvocation *invocation) {
    GVariantBuilder *builder;
    GVariant *value;

    builder = g_variant_builder_new(G_VARIANT_TYPE("as"));

    char **cap;
    for (cap = server_capabilities;
            cap != NULL && *cap != NULL;
            cap++) {
        g_variant_builder_add(builder, "s", *cap);
    }

    value = g_variant_new("(as)", builder);
    g_variant_builder_unref(builder);
    g_dbus_method_invocation_return_value(invocation, value);

    g_dbus_connection_flush(conn, NULL, NULL, NULL);
}

static void close_notification(GDBusConnection *conn, const char *sender,
                               GVariant *params,
                               GDBusMethodInvocation *invocation) {
    guint32 id;
    g_variant_get(params, "(u)", &id);

    // TODO: return empty dbus error if note does not currently exist

    queue_close(id, CLOSE_REASON_CLOSED);

    g_dbus_method_invocation_return_value(invocation, NULL);
    g_dbus_connection_flush(conn, NULL, NULL, NULL);
}

#if NL_ACTIONS && NL_REMOTE_ACTIONS
static void invoke_action(GDBusConnection *conn, const char *sender,
                          GVariant *params,
                          GDBusMethodInvocation *invocation) {
    guint32 id;
    gchar *key;
    g_variant_get(params, "(us)", &id, &key);

    // TODO: return empty dbus error if note does not currently exist
    // or note does not have invoked action
    nl_invoke_action(id, key);

    g_free(key);
    g_dbus_method_invocation_return_value(invocation, NULL);
    g_dbus_connection_flush(conn, NULL, NULL, NULL);
}
#endif

NLServerInfo *server_info;

static void get_server_information(GDBusConnection *conn, const char *sender,
                                   const GVariant *params,
                                   GDBusMethodInvocation *invocation) {
    GVariant *value;

    if (server_info) {
        value = g_variant_new("(ssss)",
                              server_info->app_name,
                              server_info->author,
                              server_info->version,
                              DBUS_VERSION);
    } else {
        value = g_variant_new("(ssss)",
                              "A notlib-based application",
                              "Anonymous",
                              "0.0",
                              DBUS_VERSION);
    }
    g_dbus_method_invocation_return_value(invocation, value);
    g_dbus_connection_flush(conn, NULL, NULL, NULL);
}

static void notify(GDBusConnection *conn, const char *sender,
                   GVariant *params,
                   GDBusMethodInvocation *invocation) {
    char *appname = NULL;
    uint32_t replaces_id = 0;
    char *summary = NULL;
    char *body = NULL;
#if NL_ACTIONS
    NLActions *actions = ealloc(sizeof(NLActions));
    actions->count = 0;
    actions->keys = NULL;
    actions->names = NULL;
#endif
    int32_t timeout = -1;
#if NL_URGENCY
    enum NLUrgency urgency = 1;
#endif
    NLHints *hints = NULL;

    {
        GVariantIter _iter;
        GVariantIter *iter = &_iter;
        g_variant_iter_init(iter, params);
        GVariant *content;
#if NL_URGENCY
        GVariant *dict_value;
#endif
        int idx = 0;
        while ((content = g_variant_iter_next_value(iter))) {
            switch (idx) {
            case 0:
                if (g_variant_is_of_type(content, G_VARIANT_TYPE_STRING))
                    appname = g_variant_dup_string(content, NULL);
                break;
            case 1:
                if (g_variant_is_of_type(content, G_VARIANT_TYPE_UINT32))
                    replaces_id = g_variant_get_uint32(content);
                break;
            case 2: break;  /* icon -- not supported */
            case 3:
                if (g_variant_is_of_type(content, G_VARIANT_TYPE_STRING))
                    summary = g_variant_dup_string(content, NULL);
                break;
            case 4:
                if (g_variant_is_of_type(content, G_VARIANT_TYPE_STRING))
                    body = g_variant_dup_string(content, NULL);
                break;
            case 5:
#if NL_ACTIONS
                if (g_variant_is_of_type(content, G_VARIANT_TYPE_STRING_ARRAY))
                    actions->actions = g_variant_dup_strv(content, &(actions->count));
#endif
                break;
            case 6:
                if (g_variant_is_of_type(content, G_VARIANT_TYPE_DICTIONARY)) {
                    hints = ealloc(sizeof(NLHints));
                    hints->dict = g_variant_dict_new(content);
#if NL_URGENCY
                    dict_value = g_variant_lookup_value(content, "urgency", G_VARIANT_TYPE_BYTE);
                    if (dict_value) {
                        urgency = g_variant_get_byte(dict_value);
                        g_variant_unref(dict_value);
                    }
#endif
                }
                break;
            case 7:
                if (g_variant_is_of_type(content, G_VARIANT_TYPE_INT32))
                    timeout = g_variant_get_int32(content);
                break;
            }
            g_variant_unref(content);
            ++idx;
        }
    }

    uint32_t n_id;
    if (replaces_id != 0) {
        claim_id(replaces_id);
        n_id = replaces_id;
    } else {
        n_id = get_unclaimed_id();
    }

#if NL_ACTIONS
    if (actions->count < 1) {
        free_actions(actions);
        actions = NULL;
    }
#endif

    NLNote *note = new_note(n_id, appname, summary, body,
#if NL_ACTIONS
                            actions,
#endif
#if NL_URGENCY
                            urgency,
#endif
                            hints,
                            timeout);

    queue_notify(note);

    GVariant *reply = g_variant_new("(u)", n_id);
    g_dbus_method_invocation_return_value(invocation, reply);
    g_dbus_connection_flush(conn, NULL, NULL, NULL);
}

/**
 * DBus signal logic
 */

void signal_notification_closed(uint32_t id, enum CloseReason reason) {
    if (reason < CLOSE_REASON_MIN || reason > CLOSE_REASON_MAX)
        reason = CLOSE_REASON_UNKNOWN;

    GVariant *body = g_variant_new("(uu)", id, reason);
    GError *err = NULL;
    g_dbus_connection_emit_signal(dbus_conn, NULL, FDN_PATH, FDN_IFAC,
            "NotificationClosed", body, &err);

    if (err != NULL) {
        fprintf(stderr, "Could not emit NotificationClosed signal: %s\n", err->message);
        g_error_free(err);
    }
}

#if NL_ACTIONS
void signal_action_invoked(uint32_t id, const char *key) {
    GVariant *body = g_variant_new("(us)", id, key);
    GError *err = NULL;
    g_dbus_connection_emit_signal(dbus_conn, NULL, FDN_PATH, FDN_IFAC,
            "ActionInvoked", body, &err);

    if (err != NULL) {
        fprintf(stderr, "Could not emit ActionInvoked signal: %s\n", err->message);
        g_error_free(err);
    }
}
#endif

/**
 * Scaffolding.
 */

void handle_method_call(GDBusConnection *conn,
                        const char *sender,
                        const char *object_path,
                        const char *interface_name,
                        const char *method_name,
                        GVariant *params,
                        GDBusMethodInvocation *invocation,
                        gpointer user_data) {
    if (g_strcmp0(method_name, "GetCapabilities") == 0) {
        get_capabilities(conn, sender, params, invocation);
    } else if (g_strcmp0(method_name, "Notify") == 0) {
        notify(conn, sender, params, invocation);
    } else if (g_strcmp0(method_name, "CloseNotification") == 0) {
        close_notification(conn, sender, params, invocation);
    } else if (g_strcmp0(method_name, "GetServerInformation") == 0) {
        get_server_information(conn, sender, params, invocation);
#if NL_ACTIONS && NL_REMOTE_ACTIONS
    } else if (g_strcmp0(method_name, "InvokeAction") == 0) {
        invoke_action(conn, sender, params, invocation);
#endif
    } else {
        g_printerr("Unknown method name '%s' received from '%s'\n",
                   method_name, sender);
    }
}

static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call
};

static void on_bus_acquired(GDBusConnection *conn, const char *name,
                            gpointer user_data) {
    guint reg_id;
    GError *err = NULL;

    reg_id = g_dbus_connection_register_object(conn, FDN_PATH,
                                               introspection_data->interfaces[0],
                                               &interface_vtable,
                                               NULL, NULL, &err);

    if (reg_id == 0) {
        g_printerr("Failed to register object: %s\n", err->message);
    }
}

static void on_name_acquired(GDBusConnection *conn, const char *name,
                             gpointer user_data) {
    g_printerr("Got name %s on the session bus\n", name);
    dbus_conn = conn;
}

static void on_name_lost(GDBusConnection *conn, const char *name,
                         gpointer user_data) {
    g_printerr("Lost name %s on the session bus\n", name);
}

/**
 * The entry point to notlib.
 */

extern void run_dbus_loop(void) {
    GMainLoop *loop;
    guint owner_id;

    introspection_data = g_dbus_node_info_new_for_xml(dbus_introspection_xml,
                                                      NULL);

    owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                              FDN_NAME,
                              G_BUS_NAME_OWNER_FLAGS_NONE,
                              on_bus_acquired,
                              on_name_acquired,
                              on_name_lost,
                              NULL, NULL);

    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    g_bus_unown_name(owner_id);
}
