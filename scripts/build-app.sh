#!/bin/bash

set -e

# if argument is release, use release type, otherwise debug
if [ "$1" == "Release" ]; then
	cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
	cmake --build build-release

else
	cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
	cmake --build build-debug
fi
