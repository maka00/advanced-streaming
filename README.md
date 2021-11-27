# GStreamer concepts and examples

-----------------

## multipipe

Analyse the memory usage of various ways to start/stop a pipeline. Context: Two pipelines are running at the same time
(which is ok for gstreamer)

### Results

There is a memory leak when one pipeline is stopped and then freed (via gst_object_unref). The expectation was that the
memory is freed completely after the gst_object_unref. Looks like the memory is still held in the mainloop.

-----------------

## pipeline-interception

A simple appsrc/appsink example.