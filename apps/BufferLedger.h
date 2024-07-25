#pragma once

#include <atomic>
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
    explicit MemoryBuffer(int width, int height, int type,
                          void *data, std::size_t size, std::size_t step);
    ~MemoryBuffer();

    // copy constructor
    MemoryBuffer(const MemoryBuffer& other);

    MemoryBuffer(MemoryBuffer&&) = delete;
    MemoryBuffer& operator=(const MemoryBuffer& other) = delete;
    MemoryBuffer& operator=(MemoryBuffer&&) = delete;

    void replace_data(void *data, std::size_t size);

    [[nodiscard]] inline cv::Mat &get_mat_view() { return mat_view; }
    [[nodiscard]] inline std::atomic_int *get_ref_count() const { return ref_count; }
    [[nodiscard]] inline void *get_data() const { return data; }
  private:
    void *data = nullptr;
    std::atomic_int *ref_count = nullptr;
    cv::Mat mat_view;
};


class BufferLedger {
  public:
    explicit BufferLedger(int num_buffers, int width, int height, MemoryType frame_type);
    ~BufferLedger();

    [[nodiscard]] MemoryBuffer get_empty_buffer();
  private:
    std::vector<MemoryBuffer> ledger;

    int num_buffers;
    int width;
    int height;
    int type;
    std::size_t step;

    // memory pool share by all memory buffers
    void *memory_pool;
};
