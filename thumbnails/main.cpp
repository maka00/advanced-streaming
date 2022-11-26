#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <gst/gst.h>
#include <iostream>
#include <fmt/format.h>

#include "streamer.h"

static GMainLoop *loop;



const std::string pipeline_cmd = "videotestsrc is-live=true ! video/x-raw,framerate=30/1 ! timeoverlay ! queue ! tee name=t t. ! queue ! openh264enc ! h264parse ! queue ! splitmuxsink muxer=mpegtsmux location=./test%02d t. ! queue ! video/x-raw,framerate=1/10 ! appsink name=sink";

int main(int argc, char *argv[])
{
    // spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
    SPDLOG_INFO("HELLO");
    GstElement *pipeline, *appsrc, *conv, *videosink;
    gst_init(nullptr, nullptr);
    loop = g_main_loop_new(nullptr, false);
    auto streamer = new Streamer();
    streamer->pipeline_cmd = pipeline_cmd;
    streamer->Start();

    g_main_loop_run(loop);
    streamer->Stop();
    g_main_loop_unref(loop);
    gst_deinit();
    return 0;
}


