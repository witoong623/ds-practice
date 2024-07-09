#pragma once

#include <vector>
#include <unordered_map>

#include "opencv2/opencv.hpp"


enum ReturnFrameResult {
  RETURN_ALL,
  RETURN_PARTIAL,
  NOT_FOUND
};

typedef std::unordered_map<unsigned int, std::unordered_map<int, cv::Mat>> SourceBufferFrames;

class FrameBuffer {
  public:
    explicit FrameBuffer(int num_frames, bool enable = true);

    void buffer_frame(unsigned int source_id, int frame_num, cv::Mat frame);
    ReturnFrameResult get_frames(unsigned int source_id, int frame_num,
                                 int num_frames, std::vector<cv::Mat> & frames);
  private:
    int num_frames;
    bool enable;
    std::unordered_map<unsigned int, int> source_latest_frame_number;
    SourceBufferFrames source_buffer_frames;
};
