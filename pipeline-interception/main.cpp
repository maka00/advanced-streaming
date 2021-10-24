#include <iostream>
#include <future>
#include <boost/scope_exit.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <signal.h>
#include "streamer.h"
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>


void exit_handler(int s) {
    SPDLOG_INFO("Got Signal {}", s);
    std::exit(EXIT_SUCCESS);
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

    std::future<void> done;
    std::shared_future<void> sd = done.share();
    Streamer stream;
    stream.StartSource();
    //GSMain main_object;
    stream.StopSink();
    pause();
    SPDLOG_INFO("Application done");
    return 0;
}