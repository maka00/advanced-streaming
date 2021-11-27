#include <iostream>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <signal.h>
#include <fmt/format.h>
#include <gst/gst.h>
#include <thread>
#include <cxxopts.hpp>

void exit_handler(int s) {
    SPDLOG_INFO("Got Signal {}", s);
    std::exit(EXIT_SUCCESS);
}

void check_mem_usage(const std::string &info) {
    static double max_pm{0.0};
    int tSize = 0, resident = 0, share = 0;
    std::ifstream buffer("/proc/self/statm");
    buffer >> tSize >> resident >> share;
    buffer.close();

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    double rss = resident * page_size_kb;
    double shared_mem = share * page_size_kb;
    max_pm = std::max(max_pm, rss - shared_mem);
    SPDLOG_INFO("[{}] RSS: {}kB\t| SM: {}kB\t| PM: {}kB\t| Max: {}kB", info, rss, shared_mem, rss - shared_mem, max_pm);
}

gpointer main_loop_runner(gpointer ptr) {
    g_main_loop_run(reinterpret_cast<GMainLoop *>(ptr));
}

void start_stop_test(const std::string &launch_cmd) {
    GError *err{nullptr};
    auto pipeline2 = gst_parse_launch_full(launch_cmd.c_str(), nullptr, GST_PARSE_FLAG_FATAL_ERRORS, &err);
    if (err) {
        SPDLOG_ERROR("{}", err->message);
        g_error_free(err);
    }
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline2), GST_DEBUG_GRAPH_SHOW_STATES, "pipeline_two");
    std::thread([pipeline2] {
        while (true) {
            gst_element_set_state(pipeline2, GST_STATE_PLAYING);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            check_mem_usage("Started");
            gst_element_set_state(pipeline2, GST_STATE_NULL);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            check_mem_usage("Stopped");
        };
    }).detach();
}

void create_destroy_test(const std::string &launch_cmd) {
    GError *err{nullptr};
    std::thread([launch_cmd, &err] {
        while (true) {
            auto pipeline2 = gst_parse_launch_full(launch_cmd.c_str(), nullptr, GST_PARSE_FLAG_FATAL_ERRORS, &err);
            GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline2), GST_DEBUG_GRAPH_SHOW_STATES, "pipeline_two");
            if (err) {
                SPDLOG_ERROR("{}", err->message);
                g_error_free(err);
            }
            gst_element_set_state(pipeline2, GST_STATE_PLAYING);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            check_mem_usage("Started");
            gst_element_set_state(pipeline2, GST_STATE_NULL);
            gst_object_unref(pipeline2);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            check_mem_usage("Stopped");
        };
    }).detach();
}

void constant_running_test(const std::string &launch_cmd) {
    GError *err{nullptr};
    auto pipeline2 = gst_parse_launch_full(launch_cmd.c_str(), nullptr, GST_PARSE_FLAG_FATAL_ERRORS, &err);
    if (err) {
        SPDLOG_ERROR("{}", err->message);
        g_error_free(err);
    }
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline2), GST_DEBUG_GRAPH_SHOW_STATES, "pipeline_two");
    std::thread([] {
        while (true) {
            check_mem_usage("");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        };
    }).detach();
    std::thread([pipeline2] {
        gst_element_set_state(pipeline2, GST_STATE_PLAYING);
    }).detach();
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    static auto console = spdlog::stdout_color_mt("console");
    spdlog::set_pattern("[%H:%M:%S.%e][%s][%!][%#] %v");
    SPDLOG_INFO("Application started");

    cxxopts::Options options("multipipe", "Analyze memory consumption of gstreamer start/stop pipes");

    options.add_options()
            ("s,start", "starting and stopping")
            ("c,creating", "starting and stopping by destroying and creating pipelines")
            ("r,running", "constantly running pipelines")
            ("h,help", "Print usage");
    auto result = options.parse(argc, argv);
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);

    gst_init(nullptr, nullptr);
    auto main_loop = g_main_loop_new(nullptr, false);
    auto main_thread = g_thread_new("context-gmain", main_loop_runner, main_loop);

    GError *err{nullptr};
    auto pipeline = gst_parse_launch_full(
            "videotestsrc is-live=true pattern=11 ! videoconvert ! tee name=t t. ! queue ! fakesink",
            nullptr,
            GST_PARSE_FLAG_FATAL_ERRORS, &err);
    if (err) {
        SPDLOG_ERROR("{}", err->message);
        g_error_free(err);
        assert(false);
    }
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_STATES, "pipeline_one");

    const std::string launch_cmd = ""
                                   "videotestsrc is-live=true pattern=11 "
                                   //"! video/x-raw,width=1280,height=720 "
                                   "! video/x-raw,widht=3840,height=2160 "
                                   //"v4l2src device=/dev/video0 "
                                   "! videoconvert ! tee name=t "
                                   "t. ! queue ! x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream ! h264parse ! splitmuxsink muxer=mpegtsmux location=./test_0_%02d.ts max-size-time=10000000000 "
                                   //"t. ! queue ! x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream ! h264parse ! mpegtsmux ! fakesink"
                                   "t. ! queue ! x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream ! h264parse ! splitmuxsink muxer=mpegtsmux location=./test_1_%02d.ts max-size-time=10000000000 "
                                   //"t. ! queue ! x264enc tune=zerolatency ! video/x-h264, stream-format=byte-stream ! fakesink"
                                   "";

    if (result["s"].count()) {
        SPDLOG_INFO("Starting/Stopping without destroying the pipeline");
        start_stop_test(launch_cmd);
    } else if (result["c"].count()) {
        SPDLOG_INFO("Starting/Stopping with destroying the pipeline");
        create_destroy_test(launch_cmd);
    } else if (result["r"].count()) {
        SPDLOG_INFO("constant running the pipeline");
        constant_running_test(launch_cmd);
    } else {
        std::cout << options.help() << std::endl;
        std::cout << "use GST_DEBUG_DUMP_DOT_DIR=. to export the pipeline structure." << std::endl;
        std::cout << " and dot -Tpng filename.dot -o outfile.png" << std::endl;
        std::exit(0);
    }


    pause();
    g_main_loop_quit(main_loop);
    SPDLOG_INFO("Quiting GStreamer main loop.");
    main_loop = nullptr;
    g_thread_join(main_thread);
    SPDLOG_INFO("Joined GStreamer main thread.");
    main_thread = nullptr;
    SPDLOG_INFO("Application done");
    return 0;
}