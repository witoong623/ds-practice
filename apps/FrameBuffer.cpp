#include "FrameBuffer.h"

#include <unordered_map>

#include "opencv2/opencv.hpp"


FrameBuffer::FrameBuffer(int num_frames, bool enable): num_frames(num_frames), enable(enable) {}

void FrameBuffer::buffer_frame(unsigned int source_id, int frame_num, cv::Mat frame) {
  int latest_frame_number = -1;
  if (source_latest_frame_number.find(source_id) != source_latest_frame_number.end()) {
    latest_frame_number = source_latest_frame_number[source_id];
  }

  std::unordered_map<int, cv::Mat> &frames_buffer = source_buffer_frames[source_id];

  // remove oldest frame if buffer is full
  if (frames_buffer.size() == num_frames) {
    // + 1 because frame_num is 0-indexed
    int oldest_frame_number = latest_frame_number - num_frames + 1;
    frames_buffer.erase(oldest_frame_number);
  }

  frames_buffer.insert({frame_num, frame});
  source_latest_frame_number[source_id] = frame_num;
}

ReturnFrameResult FrameBuffer::get_frames(unsigned int source_id, int frame_num,
                                          int num_frames, std::vector<cv::Mat> & frames) {
  if (source_buffer_frames.find(source_id) != source_buffer_frames.end()) {
    std::unordered_map<int, cv::Mat> &frames_buffer = source_buffer_frames[source_id];

    for (int i = frame_num - num_frames; i < frame_num; i++) {
      if (frames_buffer.find(i) != frames_buffer.end()) {
        frames.push_back(frames_buffer[i]);
      }
    }
    if (frames.size() == num_frames) {
      return ReturnFrameResult::RETURN_ALL;
    } else if (frames.size() > 0) {
      return ReturnFrameResult::RETURN_PARTIAL;
    } else {
      return ReturnFrameResult::NOT_FOUND;
    }
  }
  return ReturnFrameResult::NOT_FOUND;
}
