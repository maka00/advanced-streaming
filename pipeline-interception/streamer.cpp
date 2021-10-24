//
// Created by mk on 23.10.21.
//

#include "streamer.h"
#include <spdlog/spdlog.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

Streamer::Streamer() {
    gst_init(nullptr, nullptr);
    loop_ = g_main_loop_new(nullptr, false);
}

Streamer::~Streamer() {
    gst_element_set_state(source_, GST_STATE_NULL);
    gst_element_set_state(sink_, GST_STATE_NULL);
    if (!source_)
        gst_object_unref(source_);
    if (!sink_)
        gst_object_unref(sink_);
    if (!loop_)
        g_main_loop_unref(loop_);
}

void Streamer::StartSource() {
    std::string cmd{};
    cmd = "videotestsrc pattern=ball is-live=true ! video/x-raw, width=640, height=480 ! appsink name=testsink";
    source_ = gst_parse_launch(cmd.c_str(), nullptr);
    if (!source_) {
        SPDLOG_ERROR("Error during: '{}'", cmd);
        throw std::exception();
    }
    auto bus = gst_element_get_bus(source_);
    gst_bus_add_watch(bus, (GstBusFunc) (Streamer::OnSourceMessage), this);
    gst_object_unref(bus);
    auto testsink = gst_bin_get_by_name(GST_BIN (source_), "testsink");
    g_object_set(G_OBJECT(testsink), "emit-signals", true, "sync", false, nullptr);
    g_signal_connect(testsink, "new-sample", G_CALLBACK(Streamer::OnNewSampleFromSink), this);
    gst_object_unref(testsink);
}

void Streamer::StartSink(GstCaps *capablities) {
    std::string cmd{};
    cmd = "appsrc name=testsource ! queue ! videoconvert ! x264enc key-int-max=10 ! h264parse ! splitmuxsink location=./tmp%02d.ts max-size-time=10000000000";
    sink_ = gst_parse_launch(cmd.c_str(), nullptr);
    if (!sink_) {
        SPDLOG_ERROR("Error during: '{}'", cmd);
        throw std::exception();
    }
    auto testsource = gst_bin_get_by_name(GST_BIN(sink_), "testsource");
    g_object_set(testsource, "caps", capablities, nullptr);
    g_object_set(testsource, "format", GST_FORMAT_TIME, nullptr);
    gst_object_unref(testsource);
    auto sink_bus = gst_element_get_bus(sink_);
    //g_object_set(testsource, "block", TRUE, NULL);
    g_object_set(testsource, "is-live", TRUE, NULL);
    gst_bus_add_watch(sink_bus, (GstBusFunc) (Streamer::OnSinkMessage), this);
    gst_object_unref(sink_bus);

}

int Streamer::OnSourceMessage([[maybe_unused]] GstBus *bus, GstMessage *msg, Streamer *self) {
    if (!msg)
        return 0;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS: {
            SPDLOG_INFO("Source got dry");
            auto source = gst_bin_get_by_name(GST_BIN(self->sink_), "testsource");
            gst_app_src_end_of_stream(GST_APP_SRC(source));
            gst_object_unref(source);
        }
            break;
        case GST_MESSAGE_ERROR:
            GError *err;
            gst_message_parse_error(msg, &err, nullptr);
            SPDLOG_ERROR("{}: {}", GST_OBJECT_NAME(msg->src), err->message);
            g_clear_error(&err);
            break;
        default:
            break;
    }
    return 0;
}

int Streamer::OnSinkMessage([[maybe_unused]] GstBus *bus, GstMessage *msg, [[maybe_unused]] Streamer *self) {
    if (!msg)
        return 0;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            SPDLOG_INFO("EOS");
            break;
        case GST_MESSAGE_ERROR:
            GError *err;
            gst_message_parse_error(msg, &err, nullptr);
            SPDLOG_ERROR("{}: {}", GST_OBJECT_NAME(msg->src), err->message);
            g_clear_error(&err);
            break;
        default:
            break;
    }
    return 0;
}

void Streamer::StartPlayback() {
    gst_element_set_state(source_, GST_STATE_PLAYING);
    g_main_loop_run(loop_);
}

static gboolean print_field(GQuark field, const GValue *value, gpointer pfx) {
    gchar *str = gst_value_serialize(value);

    g_print("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string(field), str);
    g_free(str);
    return TRUE;
}

static void print_caps(const GstCaps *caps, const gchar *pfx) {
    guint i;

    g_return_if_fail (caps != NULL);

    if (gst_caps_is_any(caps)) {
        g_print("%sANY\n", pfx);
        return;
    }
    if (gst_caps_is_empty(caps)) {
        g_print("%sEMPTY\n", pfx);
        return;
    }

    for (i = 0; i < gst_caps_get_size(caps); i++) {
        GstStructure *structure = gst_caps_get_structure(caps, i);

        g_print("%s%s\n", pfx, gst_structure_get_name(structure));
        gst_structure_foreach(structure, print_field, (gpointer) pfx);
    }
}

GstFlowReturn Streamer::OnNewSampleFromSink(GstElement *elt, Streamer *self) {
    GstSample *sample;
    GstBuffer *app_buffer, *buffer;
    GstElement *source;
    GstFlowReturn ret;
    // SPDLOG_INFO("Got data");

    /* get the sample from appsink */
    sample = gst_app_sink_pull_sample(GST_APP_SINK (elt));
    buffer = gst_sample_get_buffer(sample);
    GstPad *pad = gst_element_get_static_pad(elt, "sink");
    auto caps = gst_pad_get_current_caps(pad);
    // print_caps(caps, "    ");
    if (!self->sink_) {
        auto ccaps = gst_caps_copy(caps);
        self->StartSink(ccaps);
        gst_element_set_state(self->sink_, GST_STATE_PLAYING);
        gst_caps_unref(ccaps);
    }
    gst_caps_unref(caps);
    /* make a copy */
    app_buffer = gst_buffer_copy_deep(buffer);

    gst_sample_unref(sample);
    /* get source an push new buffer */
    source = gst_bin_get_by_name(GST_BIN (self->sink_), "testsource");
    // either push it via an event
    // g_signal_emit_by_name(source, "push-buffer", app_buffer, &ret);
    // or directly
    ret = gst_app_src_push_buffer(GST_APP_SRC (source), app_buffer);
    gst_object_unref(source);
    return ret;
}
