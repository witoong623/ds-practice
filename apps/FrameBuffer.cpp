#include "FrameBuffer.h"

#include <mutex>
#include <queue>
#include <unordered_map>

#include "opencv2/opencv.hpp"

#include "BufferLedger.h"


constexpr std::size_t NUM_CAMERA = 1;

FrameBuffer::FrameBuffer(std::size_t num_frames, bool enable):
    num_frames(num_frames), enable(enable), buffer_ledger(num_frames * NUM_CAMERA * 2, 1920, 1080, MemoryType::NV12) {}

void FrameBuffer::buffer_frame(unsigned int source_id, int frame_num, void *data, std::size_t size) {
  std::scoped_lock lock(guard);

  std::queue<int> &buffered_frame_nums = source_buffered_frame_nums[source_id];
  std::unordered_map<int, MemoryBuffer> &frames_buffer = source_buffer_frames[source_id];

  // remove oldest frame if buffer is full
  if (buffered_frame_nums.size() >= num_frames) {
    auto oldest_frame_number = buffered_frame_nums.front();
    frames_buffer.erase(oldest_frame_number);
    buffered_frame_nums.pop();
  }

  MemoryBuffer buffer = buffer_ledger.get_empty_buffer();
  buffer.replace_data(data, size);

  frames_buffer.insert({frame_num, buffer});
  buffered_frame_nums.push(frame_num);
}

ReturnFrameResult FrameBuffer::get_frames(unsigned int source_id, int frame_num,
                                          int num_frames, std::vector<MemoryBuffer> & frames) {
  std::scoped_lock lock(guard);

  if (source_buffer_frames.find(source_id) != source_buffer_frames.end()) {
    std::unordered_map<int, MemoryBuffer> &frames_buffer = source_buffer_frames[source_id];

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
