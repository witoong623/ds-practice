#pragma once

#include <vector>
#include <unordered_map>

#include "opencv2/opencv.hpp"


enum class MemoryType {
  RGB,
  RGBA,
  NV12,
};


class MemoryBuffer {
  public:
    // empty memory buffer
    explicit MemoryBuffer();
    explicit MemoryBuffer(unsigned int width, unsigned int height, MemoryType frame_type);
    ~MemoryBuffer();

    // copy constructor
    MemoryBuffer(const MemoryBuffer& other);

    MemoryBuffer(MemoryBuffer&&) = delete;
    MemoryBuffer& operator=(const MemoryBuffer& other) = delete;
    MemoryBuffer& operator=(MemoryBuffer&&) = delete;

    [[nodiscard]] inline cv::Mat &get_memory() { return memory; }
    [[nodiscard]] inline int *get_ref_count() const { return ref_count; }
  private:
    cv::Mat memory;
    int *ref_count = nullptr;
};


class BufferLedger {
  public:
    explicit BufferLedger(int num_buffers, unsigned int width, unsigned int height, MemoryType frame_type);
    ~BufferLedger();

    [[nodiscard]] MemoryBuffer get_empty_buffer();
  private:
    std::vector<MemoryBuffer> ledger;

    unsigned int width;
    unsigned int height;
    MemoryType frame_type;
};
