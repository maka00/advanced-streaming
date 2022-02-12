#include<iostream>
#include <gst/gst.h>
#include <opencv2/opencv.hpp>

static GMainLoop *loop;

static void
cb_need_data(GstElement *appsrc,
             guint unused_size,
             gpointer user_data) {
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    guint size, depth, height, width, step, channels;
    GstFlowReturn ret;
    guchar *data1;
    GstMapInfo map;
    auto img = (cv::Mat *) user_data;
    height = img->size().height;
    width = img->size().width;
    step = img->step;
    channels = img->channels();
    depth = img->depth();
    data1 = (guchar *) img->data;
    size = height * width * channels;

    buffer = gst_buffer_new_allocate(NULL, size, NULL);
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    memcpy((guchar *) map.data, data1, gst_buffer_get_size(buffer));

    GST_BUFFER_PTS (buffer) = timestamp;
    const unsigned int fps = 5;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);

    timestamp += GST_BUFFER_DURATION (buffer);

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
    if (ret != GST_FLOW_OK) {
        g_main_loop_quit(loop);
    }
}

cv::Mat loadimage(const std::string &filename) {
    return cv::imread(filename, cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
}

const std::string pipeline_cmd = "\
                                  appsrc name=source \
                                  ! video/x-raw,format=BGR,width=3840,height=2160,framerate=5/1,interlace-mode=progressive\
                                  ! videoconvert \
                                  ! video/x-raw,format=YUY2,width=3840,height=2160,framerate=5/1\
                                  ! v4l2sink device=/dev/video4 \
                                 ";


gint
main(gint argc,
     gchar *argv[]) {
    GstElement *pipeline, *appsrc, *conv, *videosink;

    auto image = loadimage("./blue_red_example.png");
    /* init GStreamer */
    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    /* setup pipeline */
    GError *err{};
    pipeline = gst_parse_launch(pipeline_cmd.c_str(), &err);
    if (err) {
        g_printerr("Error: %s\n", err->message);
        return -1;
    }

    //g_object_set (videosink, "device", "/dev/video0", NULL);
    /* setup appsrc */
    appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "source");
    if (appsrc != nullptr) {
        g_object_set(G_OBJECT (appsrc),
                     "stream-type", 0,
                     "format", GST_FORMAT_TIME, NULL);
        g_signal_connect (appsrc, "need-data", G_CALLBACK(cb_need_data), (gpointer) &image);
    } else {
        std::cout << "source element not found! ignoring.\n";
    }
    /* play */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    /* clean up */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT (pipeline));
    g_main_loop_unref(loop);

    return 0;
}


