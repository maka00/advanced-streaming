#ifndef ADVANCED_STREAMING_STREAMER_H
#define ADVANCED_STREAMING_STREAMER_H
#include <string>
#include <gst/gstbus.h>

class Streamer {
public:
    Streamer();
    ~Streamer();

    std::string pipeline_cmd;

    void Start();
    void Stop();

private:
    GstElement* pipeline;
    /// Event on each new frame in app sink element.
    static int OnNewSample(GstElement *element, Streamer *self);
};

#endif//ADVANCED_STREAMING_STREAMER_H
