#!/usr/bin/env sh
killall -q sslocal
sslocal -c ~/.config/shadowsocks/shadowsocks.json
