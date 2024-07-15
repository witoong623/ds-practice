#include "BufferLedger.h"

#include "opencv2/opencv.hpp"


MemoryBuffer::MemoryBuffer(): ref_count(nullptr) {}

MemoryBuffer::MemoryBuffer(unsigned int width, unsigned int height,
                           MemoryType frame_type):
                           ref_count(new int(0)) {
  switch (frame_type) {
    case MemoryType::RGB:
      memory = cv::Mat::zeros(height, width, CV_8UC3);
      break;
    case MemoryType::RGBA:
      memory = cv::Mat::zeros(height, width, CV_8UC4);
      break;
    case MemoryType::NV12:
      memory = cv::Mat::zeros(height * 3 / 2, width, CV_8UC1);
      break;
  }
}

MemoryBuffer::~MemoryBuffer() {
  if (ref_count != nullptr) {
    if (*ref_count > 0) {
      (*ref_count)--;
    }
  }
}

MemoryBuffer::MemoryBuffer(const MemoryBuffer& other) {
  memory = other.memory;
  ref_count = other.ref_count;
  if (ref_count != nullptr) {
    (*ref_count)++;
  }
}


BufferLedger::BufferLedger(int num_buffers, unsigned int width, unsigned int height, MemoryType frame_type):
                           width(width), height(height), frame_type(frame_type) {
  ledger.reserve(num_buffers);
  for (int i = 0; i < num_buffers; i++) {
    ledger.emplace_back(width, height, frame_type);
  }
}

BufferLedger::~BufferLedger() {
  for (auto &buffer : ledger) {
    delete buffer.get_ref_count();
  }
}

MemoryBuffer BufferLedger::get_empty_buffer() {
  for (auto &buffer : ledger) {
    if (*buffer.get_ref_count() == 0) {
      return buffer;
    }
  }

  // TODO: warning message about use more than allocated buffers
  return MemoryBuffer(width, height, frame_type);
}