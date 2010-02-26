/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* Copyright 2010 litl, LLC. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <config.h>

#include <gjs/gjs.h>
#include <cairo.h>
#include "cairo-private.h"

GJS_DEFINE_PROTO_ABSTRACT("CairoPattern", gjs_cairo_pattern)

GJS_DEFINE_PRIV_FROM_JS(GjsCairoPattern, gjs_cairo_pattern_class)

static void
gjs_cairo_pattern_finalize(JSContext *context,
                           JSObject  *obj)
{
    GjsCairoPattern *priv;
    priv = JS_GetPrivate(context, obj);
    if (priv == NULL)
        return;
    cairo_pattern_destroy(priv->pattern);
    g_slice_free(GjsCairoPattern, priv);
}

/* Properties */
static JSPropertySpec gjs_cairo_pattern_proto_props[] = {
    { NULL }
};

/* Methods */

static JSBool
getType_func(JSContext *context,
             JSObject  *object,
             uintN      argc,
             jsval     *argv,
             jsval     *retval)
{
    cairo_pattern_t *pattern;
    cairo_pattern_type_t type;

    if (argc > 1) {
        gjs_throw(context, "FIXME");
        return JS_FALSE;
    }

    pattern = gjs_cairo_pattern_get_pattern(context, object);
    type = cairo_pattern_get_type(pattern);

    if (!gjs_cairo_check_status(context, cairo_pattern_status(pattern), "pattern"))
        return JS_FALSE;

    *retval = INT_TO_JSVAL(type);
    return JS_TRUE;
}

static JSFunctionSpec gjs_cairo_pattern_proto_funcs[] = {
    // getMatrix
    { "getType", getType_func, 0, 0 },
    // setMatrix
    { NULL }
};

/* Public API */

/**
 * gjs_cairo_pattern_construct:
 * @context: the context
 * @object: object to construct
 * @pattern: cairo_pattern to attach to the object
 *
 * Constructs a pattern wrapper giving an empty JSObject and a
 * cairo pattern. A reference to @pattern will be taken.
 *
 * This is mainly used for subclasses where object is already created.
 */
void
gjs_cairo_pattern_construct(JSContext       *context,
                            JSObject        *object,
                            cairo_pattern_t *pattern)
{
    GjsCairoPattern *priv;

    g_return_if_fail(context != NULL);
    g_return_if_fail(object != NULL);
    g_return_if_fail(pattern != NULL);

    priv = g_slice_new0(GjsCairoPattern);

    g_assert(priv_from_js(context, object) == NULL);
    JS_SetPrivate(context, object, priv);

    priv->context = context;
    priv->object = object;
    priv->pattern = cairo_pattern_reference(pattern);
}

/**
 * gjs_cairo_pattern_finalize:
 * @context: the context
 * @object: object to finalize
 *
 * Destroys the resources assoicated with a pattern wrapper.
 *
 * This is mainly used for subclasses.
 */

void
gjs_cairo_pattern_finalize_pattern(JSContext *context,
                                   JSObject  *object)
{
    g_return_if_fail(context != NULL);
    g_return_if_fail(object != NULL);

    gjs_cairo_pattern_finalize(context, object);
}

/**
 * gjs_cairo_pattern_from_pattern:
 * @context: the context
 * @pattern: cairo_pattern to attach to the object
 *
 * Constructs a pattern wrapper given cairo pattern.
 * A reference to @pattern will be taken.
 *
 */
JSObject *
gjs_cairo_pattern_from_pattern(JSContext       *context,
                               cairo_pattern_t *pattern)
{
    JSObject *object;

    g_return_val_if_fail(context != NULL, NULL);
    g_return_val_if_fail(pattern != NULL, NULL);

    object = JS_NewObject(context, &gjs_cairo_pattern_class, NULL, NULL);
    if (!object) {
        gjs_throw(context, "failed to create surface");
        return NULL;
    }

    gjs_cairo_pattern_construct(context, object, pattern);

    return object;
}

/**
 * gjs_cairo_pattern_get_pattern:
 * @context: the context
 * @object: pattern wrapper
 *
 * Returns: the pattern attaches to the wrapper.
 *
 */
cairo_pattern_t *
gjs_cairo_pattern_get_pattern(JSContext *context,
                              JSObject  *object)
{
    GjsCairoPattern *priv;

    g_return_val_if_fail(context != NULL, NULL);
    g_return_val_if_fail(object != NULL, NULL);

    priv = JS_GetPrivate(context, object);
    if (priv == NULL)
        return NULL;

    return priv->pattern;
}

