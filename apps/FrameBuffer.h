#pragma once

#include <vector>
#include <unordered_map>

#include "opencv2/opencv.hpp"


typedef std::unordered_map<unsigned int, std::unordered_map<int, cv::Mat>> SourceBufferFrames;

class FrameBuffer {
  public:
    explicit FrameBuffer(int num_frames);

    void buffer_frame(unsigned int source_id, int frame_num, cv::Mat frame);
  private:
    int num_frames;
    std::unordered_map<unsigned int, int> source_latest_frame_number;
    SourceBufferFrames source_buffer_frames;
};
