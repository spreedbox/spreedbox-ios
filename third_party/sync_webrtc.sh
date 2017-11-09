#!/bin/bash -x


cd webrtc
gclient sync
echo "gclient sync"


gn gen out/ios_64 --args='target_os="ios" target_cpu="arm64"'