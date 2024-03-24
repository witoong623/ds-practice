#!/bin/bash

set -e

gstreamer_cache=$HOME/.cache/gstreamer-1.0/registry.x86_64.bin

if [ -f $gstreamer_cache ]; then
    echo "Gstreamer cache exists"
    echo "Removing Gstreamer cache"
    rm $gstreamer_cache
fi

config="config/dstest2_config.yml"

if [ "$1" == "Release" ]; then
    ./build-release/apps/person-monitor $config
else
    ./build-debug/apps/person-monitor $config
fi
