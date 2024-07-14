#!/bin/bash

xhost +

docker run -itd --gpus all --ipc=host \
	-e DISPLAY=$DISPLAY \
	-e TZ=Asia/Bangkok \
	--name ds-practice-container \
	-v /tmp/.X11-unix:/tmp/.X11-unix \
	-v /home/witoon/programming-practice/ds-practice:/home/triton-server/ds-practice \
	-v /home/witoon/programming-practice/parking-lot-monitor:/home/triton-server/parking-lot-monitor \
	ds-workspace:latest /bin/bash
