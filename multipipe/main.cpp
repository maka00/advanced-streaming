#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <signal.h>
#include <fmt/format.h>
#include <gst/gst.h>
#include <thread>

void exit_handler(int s) {
    SPDLOG_INFO("Got Signal {}", s);
    std::exit(EXIT_SUCCESS);
}

void check_mem_usage() {
    static double max_pm{0.0};
    int tSize = 0, resident = 0, share = 0;
    std::ifstream buffer("/proc/self/statm");
    buffer >> tSize >> resident >> share;
    buffer.close();

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    double rss = resident * page_size_kb;
    double shared_mem = share * page_size_kb;
    max_pm = std::max(max_pm, rss - shared_mem);
    SPDLOG_INFO("RSS: {}kB | SM: {}kB | PM: {}kB | Max: {}kB", rss, shared_mem, rss - shared_mem, max_pm);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    static auto console = spdlog::stdout_color_mt("console");
    spdlog::set_pattern("[%H:%M:%S.%e][%s][%!][%#] %v");
    SPDLOG_INFO("Application started");
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);

    gst_init(nullptr, nullptr);
    GError *err{nullptr};
    auto pipeline = gst_parse_launch_full(
            "videotestsrc is-live=true pattern=11 ! videoconvert ! tee name=t t. ! queue ! fakesink",
            nullptr,
            GST_PARSE_FLAG_FATAL_ERRORS, &err);
    if (err) {
        SPDLOG_ERROR("{}", err->message);
        g_error_free(err);
    }
    auto pipeline2 = gst_parse_launch_full(//
            "videotestsrc is-live=true pattern=11 "
            //"! video/x-raw,width=1280,height=720 "
            "! video/x-raw,widht=3840,height=2160 "
            //"v4l2src device=/dev/video0 "
            "! videoconvert ! tee name=t "
            "t. ! queue ! fakesink "
            "t. ! queue ! x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream ! h264parse ! mpegtsmux ! fakesink"
            //"t. ! queue ! x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream ! fakesink"
            "",
            nullptr,
            GST_PARSE_FLAG_FATAL_ERRORS, &err);
    if (err) {
        SPDLOG_ERROR("{}", err->message);
        g_error_free(err);
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::thread([] {
        while (true) {
            check_mem_usage();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        };
    }).detach();
//#define START_STOP_TEST
#ifdef START_STOP_TEST
    std::thread([pipeline2] {
        while (true) {
            gst_element_set_state(pipeline2, GST_STATE_PLAYING);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            gst_element_set_state(pipeline2, GST_STATE_NULL);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        };
    }).detach();
#else

    std::thread([pipeline2] {
        gst_element_set_state(pipeline2, GST_STATE_PLAYING);
    }).detach();
#endif
    pause();
    SPDLOG_INFO("Application done");
    return 0;
}