#pragma once

#include <mutex>
#include <vector>
#include <unordered_map>

#include "opencv2/opencv.hpp"

#include "BufferLedger.h"


enum ReturnFrameResult {
  RETURN_ALL,
  RETURN_PARTIAL,
  NOT_FOUND
};

typedef std::unordered_map<unsigned int, std::unordered_map<int, MemoryBuffer>> SourceBufferFrames;

class FrameBuffer {
  public:
    explicit FrameBuffer(int num_frames, bool enable = true);

    // TODO: protect buffer and get function with mutex
    // frame is data from pipeline, need to copy it
    void buffer_frame(unsigned int source_id, int frame_num, void *data, std::size_t size);

    ReturnFrameResult get_frames(unsigned int source_id, int frame_num,
                                 int num_frames, std::vector<MemoryBuffer> & frames);
  private:
    bool enable;
    int num_frames;
    std::mutex guard;

    std::unordered_map<unsigned int, int> source_latest_frame_number;
    SourceBufferFrames source_buffer_frames;
    BufferLedger buffer_ledger;
};
