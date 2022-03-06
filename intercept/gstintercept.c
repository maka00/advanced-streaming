/* GStreamer
 * Copyright (C) 2022 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstintercept
 *
 * The intercept element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! intercept ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include "gstintercept.h"

GST_DEBUG_CATEGORY_STATIC (gst_intercept_debug_category);
#define GST_CAT_DEFAULT gst_intercept_debug_category

/* prototypes */


static void gst_intercept_set_property(GObject *object,
                                       guint property_id, const GValue *value, GParamSpec *pspec);

static void gst_intercept_get_property(GObject *object,
                                       guint property_id, GValue *value, GParamSpec *pspec);

static void gst_intercept_dispose(GObject *object);

static void gst_intercept_finalize(GObject *object);

static GstCaps *gst_intercept_transform_caps(GstBaseTransform *trans,
                                             GstPadDirection direction, GstCaps *caps, GstCaps *filter);

static GstCaps *gst_intercept_fixate_caps(GstBaseTransform *trans,
                                          GstPadDirection direction, GstCaps *caps, GstCaps *othercaps);

static gboolean gst_intercept_accept_caps(GstBaseTransform *trans,
                                          GstPadDirection direction, GstCaps *caps);

static gboolean gst_intercept_set_caps(GstBaseTransform *trans,
                                       GstCaps *incaps, GstCaps *outcaps);

static gboolean gst_intercept_query(GstBaseTransform *trans,
                                    GstPadDirection direction, GstQuery *query);

static gboolean gst_intercept_decide_allocation(GstBaseTransform *trans,
                                                GstQuery *query);

static gboolean gst_intercept_filter_meta(GstBaseTransform *trans,
                                          GstQuery *query, GType api, const GstStructure *params);

static gboolean gst_intercept_propose_allocation(GstBaseTransform *trans,
                                                 GstQuery *decide_query, GstQuery *query);

static gboolean gst_intercept_transform_size(GstBaseTransform *trans,
                                             GstPadDirection direction, GstCaps *caps, gsize size, GstCaps *othercaps,
                                             gsize *othersize);

static gboolean gst_intercept_get_unit_size(GstBaseTransform *trans,
                                            GstCaps *caps, gsize *size);

static gboolean gst_intercept_start(GstBaseTransform *trans);

static gboolean gst_intercept_stop(GstBaseTransform *trans);

static gboolean gst_intercept_sink_event(GstBaseTransform *trans,
                                         GstEvent *event);

static gboolean gst_intercept_src_event(GstBaseTransform *trans,
                                        GstEvent *event);

static GstFlowReturn gst_intercept_prepare_output_buffer(GstBaseTransform *
trans, GstBuffer *input, GstBuffer **outbuf);

static gboolean gst_intercept_copy_metadata(GstBaseTransform *trans,
                                            GstBuffer *input, GstBuffer *outbuf);

static gboolean gst_intercept_transform_meta(GstBaseTransform *trans,
                                             GstBuffer *outbuf, GstMeta *meta, GstBuffer *inbuf);

static void gst_intercept_before_transform(GstBaseTransform *trans,
                                           GstBuffer *buffer);

static GstFlowReturn gst_intercept_transform(GstBaseTransform *trans,
                                             GstBuffer *inbuf, GstBuffer *outbuf);

static GstFlowReturn gst_intercept_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf);

enum {
    PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_intercept_src_template =
        GST_STATIC_PAD_TEMPLATE ("src",
                                 GST_PAD_SRC,
                                 GST_PAD_ALWAYS,
                                 GST_STATIC_CAPS("video/x-raw")
        //GST_STATIC_CAPS("application/unknown")
        );

static GstStaticPadTemplate gst_intercept_sink_template =
        GST_STATIC_PAD_TEMPLATE ("sink",
                                 GST_PAD_SINK,
                                 GST_PAD_ALWAYS,
                                 GST_STATIC_CAPS("video/x-raw")
        );

static GstStaticPadTemplate gst_intercept_sink_template2 =
        GST_STATIC_PAD_TEMPLATE ("sink2_%u",
                                 GST_PAD_SINK,
                                 GST_PAD_REQUEST,
                                 GST_STATIC_CAPS("video/x-raw")
        );

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstIntercept, gst_intercept, GST_TYPE_BASE_TRANSFORM,
                         GST_DEBUG_CATEGORY_INIT(gst_intercept_debug_category, "intercept", 0,
                                                 "debug category for intercept element"));

static void
gst_intercept_class_init(GstInterceptClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);

    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass),
                                              &gst_intercept_src_template);
    gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass),
                                              &gst_intercept_sink_template);

    gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass),
                                              &gst_intercept_sink_template2);

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "FIXME Long name", "Generic", "FIXME Description",
                                          "FIXME <fixme@example.com>");

    gobject_class->set_property = gst_intercept_set_property;
    gobject_class->get_property = gst_intercept_get_property;
    gobject_class->dispose = gst_intercept_dispose;
    gobject_class->finalize = gst_intercept_finalize;
    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR (gst_intercept_transform_caps);
    /*
    base_transform_class->fixate_caps = GST_DEBUG_FUNCPTR (gst_intercept_fixate_caps);
     */
    base_transform_class->accept_caps = GST_DEBUG_FUNCPTR (gst_intercept_accept_caps);
    /*
    base_transform_class->set_caps = GST_DEBUG_FUNCPTR (gst_intercept_set_caps);
    base_transform_class->query = GST_DEBUG_FUNCPTR (gst_intercept_query);
    base_transform_class->decide_allocation = GST_DEBUG_FUNCPTR (gst_intercept_decide_allocation);
    base_transform_class->filter_meta = GST_DEBUG_FUNCPTR (gst_intercept_filter_meta);
    base_transform_class->propose_allocation = GST_DEBUG_FUNCPTR (gst_intercept_propose_allocation);
    base_transform_class->transform_size = GST_DEBUG_FUNCPTR (gst_intercept_transform_size);
     */
    base_transform_class->get_unit_size = GST_DEBUG_FUNCPTR (gst_intercept_get_unit_size);
    base_transform_class->start = GST_DEBUG_FUNCPTR (gst_intercept_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_intercept_stop);
    /*
    base_transform_class->sink_event = GST_DEBUG_FUNCPTR (gst_intercept_sink_event);
    base_transform_class->src_event = GST_DEBUG_FUNCPTR (gst_intercept_src_event);
    base_transform_class->prepare_output_buffer = GST_DEBUG_FUNCPTR (gst_intercept_prepare_output_buffer);
    base_transform_class->copy_metadata = GST_DEBUG_FUNCPTR (gst_intercept_copy_metadata);
    base_transform_class->transform_meta = GST_DEBUG_FUNCPTR (gst_intercept_transform_meta);
    base_transform_class->before_transform = GST_DEBUG_FUNCPTR (gst_intercept_before_transform);
    base_transform_class->transform = GST_DEBUG_FUNCPTR (gst_intercept_transform);
     */
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR (gst_intercept_transform_ip);

}

static void
gst_intercept_init(GstIntercept *intercept) {
}

void
gst_intercept_set_property(GObject *object, guint property_id,
                           const GValue *value, GParamSpec *pspec) {
    GstIntercept *intercept = GST_INTERCEPT (object);

    GST_DEBUG_OBJECT (intercept, "set_property");

    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

void
gst_intercept_get_property(GObject *object, guint property_id,
                           GValue *value, GParamSpec *pspec) {
    GstIntercept *intercept = GST_INTERCEPT (object);

    GST_DEBUG_OBJECT (intercept, "get_property");

    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

void
gst_intercept_dispose(GObject *object) {
    GstIntercept *intercept = GST_INTERCEPT (object);

    GST_DEBUG_OBJECT (intercept, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS (gst_intercept_parent_class)->dispose(object);
}

void
gst_intercept_finalize(GObject *object) {
    GstIntercept *intercept = GST_INTERCEPT (object);

    GST_DEBUG_OBJECT (intercept, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS (gst_intercept_parent_class)->finalize(object);
}

static GstCaps *
gst_intercept_transform_caps(GstBaseTransform *trans, GstPadDirection direction,
                             GstCaps *caps, GstCaps *filter) {
    GstIntercept *intercept = GST_INTERCEPT (trans);
    GstCaps *othercaps;

    GST_DEBUG_OBJECT (intercept, "transform_caps");

    othercaps = gst_caps_copy (caps);

    /* Copy other caps and modify as appropriate */
    /* This works for the simplest cases, where the transform modifies one
     * or more fields in the caps structure.  It does not work correctly
     * if passthrough caps are preferred. */
    if (direction == GST_PAD_SRC) {
        /* transform caps going upstream */
    } else {
        /* transform caps going downstream */
    }

    if (filter) {
        GstCaps *intersect;

        intersect = gst_caps_intersect(othercaps, filter);
        gst_caps_unref(othercaps);

        return intersect;
    } else {
        return othercaps;
    }
}

static GstCaps *
gst_intercept_fixate_caps(GstBaseTransform *trans, GstPadDirection direction,
                          GstCaps *caps, GstCaps *othercaps) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "fixate_caps");

    return NULL;
}

static gboolean
gst_intercept_accept_caps(GstBaseTransform *trans, GstPadDirection direction,
                          GstCaps *caps) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "accept_caps");
    GstPad *pad;

    // The source pad caps define all the caps downstream
    // we don't want to define them upstream
    pad = GST_BASE_TRANSFORM_SRC_PAD(intercept);

    return gst_pad_peer_query_accept_caps(pad, caps);
}

static gboolean
gst_intercept_set_caps(GstBaseTransform *trans, GstCaps *incaps,
                       GstCaps *outcaps) {
    GstIntercept *intercept = GST_INTERCEPT (trans);
    GST_DEBUG_OBJECT (intercept, "set_caps");

    return TRUE;
}

static gboolean
gst_intercept_query(GstBaseTransform *trans, GstPadDirection direction,
                    GstQuery *query) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "query");

    return TRUE;
}

/* decide allocation query for output buffers */
static gboolean
gst_intercept_decide_allocation(GstBaseTransform *trans, GstQuery *query) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "decide_allocation");

    return TRUE;
}

static gboolean
gst_intercept_filter_meta(GstBaseTransform *trans, GstQuery *query, GType api,
                          const GstStructure *params) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "filter_meta");

    return TRUE;
}

/* propose allocation query parameters for input buffers */
static gboolean
gst_intercept_propose_allocation(GstBaseTransform *trans,
                                 GstQuery *decide_query, GstQuery *query) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "propose_allocation");

    return TRUE;
}

/* transform size */
static gboolean
gst_intercept_transform_size(GstBaseTransform *trans, GstPadDirection direction,
                             GstCaps *caps, gsize size, GstCaps *othercaps, gsize *othersize) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "transform_size");

    return TRUE;
}

static gboolean
gst_intercept_get_unit_size(GstBaseTransform *trans, GstCaps *caps,
                            gsize *size) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "get_unit_size");

    return TRUE;
}

/* states */
static gboolean
gst_intercept_start(GstBaseTransform *trans) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "start");

    return TRUE;
}

static gboolean
gst_intercept_stop(GstBaseTransform *trans) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "stop");

    return TRUE;
}

/* sink and src pad event handlers */
static gboolean
gst_intercept_sink_event(GstBaseTransform *trans, GstEvent *event) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "sink_event");

    return GST_BASE_TRANSFORM_CLASS (gst_intercept_parent_class)->sink_event(
            trans, event);
}

static gboolean
gst_intercept_src_event(GstBaseTransform *trans, GstEvent *event) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "src_event");

    return GST_BASE_TRANSFORM_CLASS (gst_intercept_parent_class)->src_event(
            trans, event);
}

static GstFlowReturn
gst_intercept_prepare_output_buffer(GstBaseTransform *trans, GstBuffer *input,
                                    GstBuffer **outbuf) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "prepare_output_buffer");

    return GST_FLOW_OK;
}

/* metadata */
static gboolean
gst_intercept_copy_metadata(GstBaseTransform *trans, GstBuffer *input,
                            GstBuffer *outbuf) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "copy_metadata");

    return TRUE;
}

static gboolean
gst_intercept_transform_meta(GstBaseTransform *trans, GstBuffer *outbuf,
                             GstMeta *meta, GstBuffer *inbuf) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "transform_meta");

    return TRUE;
}

static void
gst_intercept_before_transform(GstBaseTransform *trans, GstBuffer *buffer) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "before_transform");

}

/* transform */
static GstFlowReturn
gst_intercept_transform(GstBaseTransform *trans, GstBuffer *inbuf,
                        GstBuffer *outbuf) {
    GstIntercept *intercept = GST_INTERCEPT (trans);

    GST_DEBUG_OBJECT (intercept, "transform");

    return GST_FLOW_OK;
}

static GstFlowReturn
gst_intercept_transform_ip(GstBaseTransform *trans, GstBuffer *buf) {
    GstIntercept *intercept = GST_INTERCEPT (trans);
    GstMapInfo *info;
    GST_DEBUG_OBJECT (intercept, "transform_ip");
    //gst_buffer_map(buf, info, GST_MAP_READWRITE);

    //gst_buffer_unmap(buf, info);
    return GST_FLOW_OK;
}

static gboolean
plugin_init(GstPlugin *plugin) {

    /* FIXME Remember to set the rank if it's an element that is meant
       to be autoplugged by decodebin. */
    return gst_element_register(plugin, "intercept", GST_RANK_NONE,
                                GST_TYPE_INTERCEPT);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   intercept,
                   "FIXME plugin description",
                   plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

