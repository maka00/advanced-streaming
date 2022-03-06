#!/usr/bin/env bash

export GST_PLUGIN_PATH=.
export GST_DEBUG=3

function inspect() {
	gst-inspect-1.0 intercept
}

function fake() {
	gst-launch-1.0 -v fakesrc ! intercept ! fakesink
}

fake
#inspect

