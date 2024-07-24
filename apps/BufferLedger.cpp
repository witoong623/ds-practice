#include "BufferLedger.h"

#include <cstdlib>
#include <iostream>

#include "opencv2/opencv.hpp"
#include "unistd.h"


MemoryBuffer::MemoryBuffer(): ref_count(nullptr) {}

MemoryBuffer::MemoryBuffer(
    int width, int height, int type,
    void *data, std::size_t size, std::size_t step):
    data(data), ref_count(new int(0)),
    mat_view(height, width, type, data, step) {}

MemoryBuffer::~MemoryBuffer() {
  if (ref_count != nullptr) {
    if (*ref_count > 0) {
      (*ref_count)--;
    }
  }
}

MemoryBuffer::MemoryBuffer(const MemoryBuffer& other) {
  data = other.data;
  ref_count = other.ref_count;
  mat_view = other.mat_view;
  if (ref_count != nullptr) {
    (*ref_count)++;
  }
}


// TODO: calculate size of memory with alighment padding dynamically
static constexpr int NV12_GPU_BUFFER_SIZE = 3317760;

BufferLedger::BufferLedger(
    int num_buffers, int width, int height, MemoryType frame_type):
    num_buffers(num_buffers), width(width), height(height) {
  
  // find all necessary info for createing memory buffer
  switch (frame_type) {
    case MemoryType::NV12: {
      this->height = height * 3 / 2;
      type = CV_8UC1;
      step = 2048;
      break;
    }
    case MemoryType::RGB: {
      this->height = height;
      type = CV_8UC3;
      step = width + 3;
      break;
    }
    case MemoryType::RGBA: {
      this->height = height;
      type = CV_8UC4;
      step = width + 4;
      break;
    }
  }

  memory_pool = std::malloc(NV12_GPU_BUFFER_SIZE * num_buffers);

  ledger.reserve(num_buffers);
  auto data_ptr = memory_pool;

  for (int i = 0; i < num_buffers; i++) {
    ledger.emplace_back(width, height, type, data_ptr, NV12_GPU_BUFFER_SIZE, step);
    data_ptr = static_cast<unsigned char*>(data_ptr) + NV12_GPU_BUFFER_SIZE;
  }
}

BufferLedger::~BufferLedger() {
  for (auto &buffer : ledger) {
    auto *ref_count = buffer.get_ref_count();
    if (ref_count != nullptr) {
      delete ref_count;
    }
  }
}

MemoryBuffer BufferLedger::get_empty_buffer() {
  for (auto &buffer : ledger) {
    if (*buffer.get_ref_count() == 0) {
      return buffer;
    }
  }

  std::cout << "Error: no empty buffer found" << std::endl;
  // TODO: create new memory pool by calculating optimal size of buffer based on page size
  auto page_size = sysconf(_SC_PAGE_SIZE);

  throw std::runtime_error("Error: no empty buffer found. Page size is " + std::to_string(page_size));
}

void MemoryBuffer::replace_data(void *data, std::size_t size) {
  std::memcpy(this->data, data, size);
}
