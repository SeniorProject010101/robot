#!/usr/bin/env bash
# Starts a virtual desktop (viewable at http://localhost:6080) when no real
# X display is available, e.g. on a Mac. On a Linux host with X forwarding
# the display is already live, so this does nothing.

n="${DISPLAY#:}"; n="${n%%.*}"
display_live() {
  python3 -c 'import socket,sys; socket.socket(socket.AF_UNIX).connect(sys.argv[1])' \
    "/tmp/.X11-unix/X$n" 2>/dev/null
}
if [ -n "$n" ] && ! display_live; then
  rm -f "/tmp/.X11-unix/X$n" "/tmp/.X$n-lock"
  Xvfb "$DISPLAY" -screen 0 1600x900x24 >/tmp/xvfb.log 2>&1 &
  sleep 1
  fluxbox >/tmp/fluxbox.log 2>&1 &
  x11vnc -display "$DISPLAY" -forever -shared -nopw -quiet >/tmp/x11vnc.log 2>&1 &
  websockify --web /usr/share/novnc 6080 localhost:5900 >/tmp/novnc.log 2>&1 &
fi

exec "$@"
