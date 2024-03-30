#!/bin/bash

xhost +

docker run -itd --gpus all --ipc=host \
	-e DISPLAY=$DISPLAY \
	--name ds-practice-container \
	-v /tmp/.X11-unix:/tmp/.X11-unix \
	-v /etc/localtime:/etc/localtime:ro \
	-v /etc/timezone:/etc/timezone:ro \
	-v $(pwd):/home/triton-server/ds-practice \
	ds-workspace:latest /bin/bash
