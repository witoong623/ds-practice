#!/bin/bash

set -e

config="config/dstest2_config.yml"

if [ "$1" == "Release" ]; then
    ./build-release/apps/person-monitor $config
else
    ./build-debug/apps/person-monitor $config
fi
