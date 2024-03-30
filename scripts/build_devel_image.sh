#!/bin/bash

echo "Building devel image"

docker build -t ds-workspace:latest \
	--target devel \
	-f docker/Dockerfile .
