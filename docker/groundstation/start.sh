#!/bin/bash

# Start virtual display on :99
Xvfb :99 -screen 0 1024x768x24 &

# Start VNC server- nopw since we are not wanting a password for quicker development purposes
x11vnc -display :99 -nopw -forever &

/usr/share/novnc/utils/novnc_proxy --listen 6080 --vnc localhost:5900 &

# Start Python GUI using display :99
DISPLAY=:99 python3 ground_station/gui.py