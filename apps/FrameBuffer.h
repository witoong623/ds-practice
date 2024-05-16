#pragma once

#include <vector>
#include <unordered_map>

#include "opencv2/opencv.hpp"


class FrameBuffer {
  public:
    explicit FrameBuffer(int num_frames);
    ~FrameBuffer();

    void buffer_frame(void *data, int size);
  private:
    std::unordered_map<int, std::vector<cv::Mat>> source_buffer_frames;
};
