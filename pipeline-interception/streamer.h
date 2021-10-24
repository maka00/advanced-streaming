//
// Created by mk on 23.10.21.
//

#ifndef STREAMING_PROJECT_STREAMER_H
#define STREAMING_PROJECT_STREAMER_H

#include<gst/gst.h>

class Streamer {
public:
    Streamer();

    ~Streamer();

    void StartSource();

    void StartSink(GstCaps *capablities);

    void StartPlayback();

private:
    static int OnSourceMessage(GstBus *bus, GstMessage *msg, Streamer *self);

    static int OnSinkMessage(GstBus *bus, GstMessage *msg, Streamer *self);

    static GstFlowReturn OnNewSampleFromSink(GstElement *elt, Streamer *self);


    GstElement *source_{nullptr};
    GstElement *sink_{nullptr};
    GMainLoop *loop_{nullptr};
};


#endif //STREAMING_PROJECT_STREAMER_H
