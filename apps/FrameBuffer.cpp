#include "FrameBuffer.h"

#include <unordered_map>


FrameBuffer::FrameBuffer(int num_frames): num_frames(num_frames) {}

void FrameBuffer::buffer_frame(unsigned int source_id, int frame_num, cv::Mat frame) {
  int latest_frame_number = -1;
  if (source_latest_frame_number.find(source_id) != source_latest_frame_number.end()) {
    int latest_frame_number = source_latest_frame_number[source_id];
  }

  std::unordered_map<int, cv::Mat> frames_buffer = source_buffer_frames[source_id];

  if (frames_buffer.size() == num_frames) {
    int oldest_frame_number = latest_frame_number - num_frames;
    frames_buffer.erase(oldest_frame_number);
  }

  frames_buffer.insert({frame_num, frame});
}
