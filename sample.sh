#!/usr/bin/env bash


# This function streams an image to the v4l2loopback device with 5 fps (higher rates result in higher CPU usage)
function streamImage() {
gst-launch-1.0 \
	filesrc location=./blue_red_example.png \
	! decodebin \
	! imagefreeze \
	! videoconvert \
	! 'video/x-raw,width=(int)3840, height=(int)2160, format=(string)YUY2, framerate=(fraction)5/1' \
	! v4l2sink device=/dev/video4
}

streamImage

