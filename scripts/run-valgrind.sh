#!/bin/bash

set -e

config="config/dstest3_config.yml"

valgrind ./build-debug/apps/ds-practice $config
