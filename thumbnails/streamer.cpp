//
// Created by mk on 20.11.22.
//

#include "streamer.h"
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <spdlog/spdlog.h>

Streamer::Streamer() {
    pipeline_cmd = std::string();
}
Streamer::~Streamer() {
}

void Streamer::Start() {
    GError* err{nullptr};
    pipeline = gst_parse_launch(pipeline_cmd.c_str(), &err);
    if ( err != nullptr) {
        SPDLOG_ERROR("error during launch: {}", err->message);
        throw std::exception();
    }
    if (!pipeline) {
        SPDLOG_ERROR("error during launch: {}", pipeline_cmd);
        throw std::exception();
    }
    auto sink = gst_bin_get_by_name(GST_BIN(pipeline),"sink");
    g_object_set(G_OBJECT(sink), "emit-signals", true, "sync", false, nullptr);
    g_signal_connect(sink, "new-sample", G_CALLBACK(Streamer::OnNewSample), this);
    gst_object_unref(sink);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

}
void Streamer::Stop() {
    gst_element_send_event(GST_ELEMENT(pipeline),gst_event_new_eos());
}
int Streamer::OnNewSample(GstElement *element, Streamer *self) {
    auto sample = gst_app_sink_pull_sample(GST_APP_SINK (element));
    auto buffer = gst_sample_get_buffer(sample);
    GstPad *pad = gst_element_get_static_pad(element, "sink");
    auto caps = gst_pad_get_current_caps(pad);
    gst_caps_unref(caps);
    gst_sample_unref(sample);
    return 0;
}
