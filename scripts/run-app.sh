#!/bin/bash

set -e

config="config/dstest3_config.yml"

if [ "$1" == "Release" ]; then
    ./build-release/apps/ds-practice $config
else
    ./build-debug/apps/ds-practice $config
fi
