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

#include <stdint.h>

#include "notlib.h"
#include "_notlib_internal.h"

extern NLNote *new_note(uint32_t id, char *appname,
                        char *summary, char *body,
#if NL_ACTIONS
                        NLActions *actions,
#endif
#if NL_URGENCY
                        enum NLUrgency urgency,
#endif
                        NLHints *hints,
                        int32_t timeout) {
    NLNote *n = ealloc(sizeof(NLNote));

    n->id      = id;
    n->appname = appname;
    n->summary = summary;
    n->body    = body;
    n->timeout = timeout;
#if NL_ACTIONS
    n->actions = actions;
#endif
#if NL_URGENCY
    n->urgency = urgency;
#endif
    n->hints = hints;
    return n;
}

extern int nl_get_hint(const NLNote *n, const char *key, NLHint *out) {
    if (n == NULL)
        return 0;

    GVariant *gv = g_variant_dict_lookup_value(n->hints->dict, key, NULL);
    if (gv == NULL) return 0;

    const GVariantType *t = g_variant_get_type(gv);

    if (g_variant_type_equal(t, G_VARIANT_TYPE_STRING)) {
        out->type = HINT_TYPE_STRING;
        size_t len;
        out->d.str = g_variant_get_string(gv, &len);
    } else if (g_variant_type_equal(t, G_VARIANT_TYPE_BOOLEAN)) {
        out->type = HINT_TYPE_BOOLEAN;
        out->d.bl = g_variant_get_boolean(gv);
    } else if (g_variant_type_equal(t, G_VARIANT_TYPE_BYTE)) {
        out->type = HINT_TYPE_BYTE;
        out->d.byte = g_variant_get_byte(gv);
    } else if (g_variant_type_equal(t, G_VARIANT_TYPE_INT32)) {
        out->type = HINT_TYPE_INT;
        out->d.i = g_variant_get_int32(gv);
    } else {
        g_variant_unref(gv);
        return 0;
    }

    g_variant_unref(gv);
    return 1;
}

extern char *nl_get_hint_as_string(const NLNote *n, const char *key) {
    char *out;
    if (n == NULL)
        return NULL;

    GVariant *gv = g_variant_dict_lookup_value(n->hints->dict,
            key, G_VARIANT_TYPE_STRING);

    if (gv != NULL) {
        out = g_variant_dup_string(gv, NULL);
    } else {
        gv = g_variant_dict_lookup_value(n->hints->dict, key, NULL);
        if (gv == NULL) return NULL;
        out = g_variant_print(gv, FALSE);
    }

    g_variant_unref(gv);
    return out;
}

extern enum NLHintType nl_get_hint_type(const NLNote *n, const char *key) {
    NLHint h;
    if (!nl_get_hint(n, key, &h))
        return HINT_TYPE_UNKNOWN;
    return h.type;
}

extern int nl_get_int_hint(const NLNote *n, const char *key, int *out) {
    if (n == NULL)
        return 0;
    GVariant *gv = g_variant_dict_lookup_value(n->hints->dict,
            key, G_VARIANT_TYPE_INT32);
    if (gv == NULL)
        return 0;
    *out = g_variant_get_int32(gv);
    g_variant_unref(gv);
    return 1;
}

extern int nl_get_byte_hint(const NLNote *n, const char *key, unsigned char *out) {
    if (n == NULL)
        return 0;
    GVariant *gv = g_variant_dict_lookup_value(n->hints->dict,
            key, G_VARIANT_TYPE_BYTE);
    if (gv == NULL)
        return 0;
    *out = g_variant_get_byte(gv);
    g_variant_unref(gv);
    return 1;
}

extern int nl_get_boolean_hint(const NLNote *n, const char *key, int *out) {
    if (n == NULL)
        return 0;
    GVariant *gv = g_variant_dict_lookup_value(n->hints->dict,
                key, G_VARIANT_TYPE_BOOLEAN);
    if (gv == NULL) {
        return 0;
    }
    *out = g_variant_get_boolean(gv);
    g_variant_unref(gv);
    return 1;
}

extern int nl_get_string_hint(const NLNote *n, const char *key, const char **out) {
    if (n == NULL)
        return 0;

    size_t l;
    GVariant *gv = g_variant_dict_lookup_value(n->hints->dict,
            key, G_VARIANT_TYPE_STRING);
    if (gv == NULL)
        return 0;
    *out = g_variant_get_string(gv, &l);
    g_variant_unref(gv);
    return 1;
}

#if NL_ACTIONS
static int invoke_action(const NLNote *n, void *v) {
    if (n == NULL)
        return 0;

    const char *key = (const char *)v;
    if (strcmp(key, "default") && !nl_action_name(n, key))
        return 0;

    signal_action_invoked(n->id, key);

    int resident;
    if (nl_get_boolean_hint(n, "resident", &resident) && resident)
        goto ret;

    nl_close_note(n->id);
ret:
    return 1;
}

extern int nl_invoke_action(unsigned int id, const char *key) {
    return queue_call(id, invoke_action, (void *)key);
}

static void init_keys_names(NLActions *a) {
    char **keys  = ealloc(sizeof(char *) * (a->count / 2));
    char **names = ealloc(sizeof(char *) * (a->count / 2));

    size_t i;
    for (i = 0; i < a->count; i++) {
        if (i % 2)
            names[i / 2] = a->actions[i];
        else
            keys[i / 2]  = a->actions[i];
    }
    a->keys  = keys;
    a->names = names;
}

extern const char **nl_action_keys(const NLNote *n) {
    if (!n) return NULL;
    if (n->actions->keys == NULL)
        init_keys_names(n->actions);
    return (const char **)n->actions->keys;
}

extern const char *nl_action_name(const NLNote *n, const char *key) {
    if (!n) return NULL;
    NLActions *a = n->actions;
    if (a == NULL)
        return NULL;

    if (a->keys == NULL)
        init_keys_names(a);

    size_t i;
    size_t nkeys = a->count / 2;
    for (i = 0; i < nkeys; i++) {
        if (!strcmp(key, a->keys[i]))
            return a->names[i];
    }
    return NULL;
}

extern void free_actions(NLActions *a) {
    if (!a) return;

    if (a->keys != NULL)
        free(a->keys);
    if (a->names != NULL)
        free(a->names);

    g_strfreev(a->actions);
    free(a);
}
#endif

extern void free_note(NLNote *n) {
    if (!n) return;

    g_free(n->appname);
    g_free(n->summary);
    g_free(n->body);

#if NL_ACTIONS
    free_actions(n->actions);
#endif

    g_variant_dict_unref(n->hints->dict);
    free(n->hints);
    free(n);
}

static int32_t dto = 5000;

extern void nl_set_default_timeout(unsigned int new) {
    dto = (int32_t) new;
}

extern int32_t note_timeout(const NLNote *n) {
    if (n->timeout >= 0)
        return n->timeout;

#if NL_URGENCY
    if (n->urgency == URG_CRIT)
        return 0;
#endif

    return dto;
}
