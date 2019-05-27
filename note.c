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

#include <stdlib.h>
#include <glib.h>
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
    NLNote *n = g_malloc(sizeof(NLNote));

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

extern enum NLHintType nl_get_hint_type(const NLNote *n, const char *key) {
    NLHint h;
    nl_get_hint(n, key, &h);
    return h.type;
}

extern int nl_get_int_hint(const NLNote *n, const char *key, int *out) {
    GVariant *gv = g_variant_dict_lookup_value(n->hints->dict,
            key, G_VARIANT_TYPE_INT32);
    if (gv == NULL)
        return 0;
    *out = g_variant_get_int32(gv);
    g_variant_unref(gv);
    return 1;
}

extern int nl_get_byte_hint(const NLNote *n, const char *key, unsigned char *out) {
    GVariant *gv = g_variant_dict_lookup_value(n->hints->dict,
            key, G_VARIANT_TYPE_BYTE);
    if (gv == NULL)
        return 0;
    *out = g_variant_get_byte(gv);
    g_variant_unref(gv);
    return 1;
}

extern int nl_get_boolean_hint(const NLNote *n, const char *key, int *out) {
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
extern void free_actions(NLActions *a) {
    if (!a) return;

    g_strfreev(a->actions);
    g_free(a);
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
    g_free(n->hints);
    g_free(n);
}

static int32_t dto = 5000;

extern void nl_set_default_timeout(unsigned int new) {
    dto = (int32_t) new;
}

extern int32_t note_timeout(const NLNote *n) {
    if (n->timeout >= 0)
        return n->timeout;

    return dto;
}
