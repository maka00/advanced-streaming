//
// Created by mk on 30.08.21.
//

#include "gstreamer.h"
#include "spdlog/spdlog.h"
#include <stdexcept>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <boost/scope_exit.hpp>

gstreamer::gstreamer(const std::string &pipeline_name) :
        _stopped{true} {
    SPDLOG_DEBUG(_logger_name, "created");
    int argc{0};
    gst_init(&argc, nullptr);
    _pipeline = gst_pipeline_new(pipeline_name.c_str());
}

gstreamer::~gstreamer() {
    gst_object_unref(_pipeline);
}

void
gstreamer::add_element(const std::string &bin_name, const std::string &plugin_name, const std::string &element_name) {
    auto element = gst_element_factory_make(plugin_name.c_str(), element_name.c_str());
    if (!element) {
        SPDLOG_ERROR("console", "unable to create factory for {}", plugin_name);
        throw std::logic_error("unable to create factory");
    }
    if (_pipelines.count(bin_name) == 0) {
        std::vector<std::pair<std::string, GstElement *>> new_item{std::make_pair(element_name, element)};
        _pipelines.emplace(bin_name, new_item);
    } else {
        _pipelines[bin_name].emplace_back(element_name, element);
    }
}

void gstreamer::link_pipeline() {
    for (const auto &item: _pipelines) {
        std::for_each(std::begin(item.second), std::end(item.second), [this](auto v) {
            gst_bin_add(GST_BIN(_pipeline), v.second);
            _elements.emplace(v.first, v.second);
        });
    }
    for (const auto &item: _pipelines) {
        if (item.second.empty())
            continue;
        std::accumulate(std::begin(item.second), std::end(item.second), item.second.begin()->second,
                        [](auto previous, auto current) {
                            if (previous == current.second)
                                return current.second;
                            gst_element_link(previous, current.second);
                            return current.second;
                        });
    }

}

void gstreamer::run() {
    SPDLOG_INFO("GStreamer started");
    _stopped = false;
    post_fix();
    auto ret = gst_element_set_state(_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        SPDLOG_ERROR("console", "Unable to set pipeline to playing state");
        throw std::logic_error("error during gstreamer state change");
    }
    GstBus *bus = gst_element_get_bus(_pipeline);
    do {
        auto msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                              static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED |
                                                                          GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        parse_message(msg);
    } while (!_stopped);
    gst_object_unref(bus);
    gst_element_set_state(_pipeline, GST_STATE_NULL);
    SPDLOG_INFO("GStreamer stopped");

}

void gstreamer::stop() {
    _stopped = true;
}

void gstreamer::parse_message(GstMessage *msg) {
    if (msg != nullptr) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE (msg)) {
            case GST_MESSAGE_ERROR: {
                gst_message_parse_error(msg, &err, &debug_info);
                SPDLOG_ERROR("Error received from element {}: {}", GST_OBJECT_NAME(msg->src), err->message);
                if (std::string(err->message) == std::string("Output window was closed")) {
                    _stopped = true;
                    return;
                }
                SPDLOG_ERROR("Debugging information: {}", debug_info ? debug_info : "none");
                g_clear_error(&err);
                g_free(debug_info);
            }
                break;
            case GST_MESSAGE_EOS:
                SPDLOG_INFO("End-Of-Stream reached.");
                _stopped = true;
                break;
            default:
                SPDLOG_ERROR("Unexpected message received.");
                break;
        }
        gst_message_unref(msg);
    }
}

void gstreamer::set_property(const std::string &element, const std::string &property,
                             std::variant<int, float, std::string> value) {
    auto it = _elements.find(element);
    if (it == _elements.end()) {
        SPDLOG_ERROR("Gst Element '{}' not known", element);
        throw std::logic_error("gst element unknown");
    }
    if (std::holds_alternative<int>(value)) {
        g_object_set(it->second, property.c_str(), std::get<int>(value), nullptr);
    } else if (std::holds_alternative<float>(value)) {
        g_object_set(it->second, property.c_str(), std::get<float>(value), nullptr);
    } else
        g_object_set(it->second, property.c_str(), std::get<std::string>(value).c_str(), nullptr);

}

void gstreamer::add_pad_added_handler(std::string element,
                                      std::function<std::optional<std::string>(const std::string &)> matcher) {
    auto matching = _signal_matcher.filter_mapper;
    if (matching.contains(element)) {
        matching[element].push_back(matcher);
    } else
        matching.insert({std::move(element), {matcher}});
    _signal_matcher = {_elements, matching};
}

void gstreamer::post_fix() {
    for (const auto & matching: _signal_matcher.filter_mapper) {
        auto it = _elements.find(matching.first);
        if (it == _elements.end()) {
            SPDLOG_ERROR("Gst Element '{}' not known", matching.first);
            throw std::logic_error("gst element unknown");
        }
        g_signal_connect(it->second, "pad_added", G_CALLBACK(gstreamer::pad_added_handler), &_signal_matcher);
    }
}

void gstreamer::pad_added_handler(GstElement *src, GstPad *new_pad,
                                  pad_handler_struct *data) {
    SPDLOG_INFO("pad_added_handler");
    GstPad *sink_pad = nullptr;
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = nullptr;
    GstStructure *new_pad_struct = nullptr;
    const gchar *new_pad_type = nullptr;
    BOOST_SCOPE_EXIT_ALL(new_pad_caps, sink_pad) {
                                                     if (new_pad_caps != nullptr)
                                                         gst_caps_unref(new_pad_caps);
                                                     if (sink_pad != nullptr)
                                                         gst_object_unref(sink_pad);
                                                 };

    SPDLOG_INFO("Received new pad '{}' from '{}'.", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    new_pad_type = gst_structure_get_name(new_pad_struct);
    auto[mapper, functions] = *data;
    for (const auto &fun: functions[std::string(GST_ELEMENT_NAME(src))]) {
        auto link = fun(new_pad_type);
        if (link) {
            sink_pad = gst_element_get_static_pad(mapper[*link], "sink");
        } else {
            SPDLOG_INFO("It has type '{}' which is not mapped for {}. Ignoring.", new_pad_type, *link);
            continue;
        }
        /* If our converter is already linked, we have nothing to do here */
        if (gst_pad_is_linked(sink_pad)) {
            SPDLOG_ERROR("We are already linked. Ignoring.");
            continue;
        }

        /* Attempt the link */
        ret = gst_pad_link(new_pad, sink_pad);
        if (GST_PAD_LINK_FAILED (ret)) {
            SPDLOG_ERROR("Type is '{}' but link failed.", new_pad_type);
        } else {
            SPDLOG_ERROR("Link succeeded (type '{}').", new_pad_type);
        }
    }
}
