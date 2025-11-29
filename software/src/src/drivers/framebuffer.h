// framebuffer.h - Double-buffered frame buffer for display
//
// Copyright (c) 2016 Patrick Dowling

#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include <stdint.h>
#include <string.h>

template <size_t frame_size, size_t num_frames>
class FrameBuffer {
public:
  static constexpr size_t kFrameSize = frame_size;
  static constexpr size_t kNumFrames = num_frames;

  void Init() {
    memset(frames_, 0, sizeof(frames_));
    write_frame_ = 0;
    read_frame_ = 0;
    readable_count_ = 0;
  }

  uint8_t *writeable_frame() {
    if (readable_count_ >= kNumFrames)
      return nullptr;
    return frames_[write_frame_];
  }

  bool writeable() const {
    return readable_count_ < kNumFrames;
  }

  void written() {
    write_frame_ = (write_frame_ + 1) % kNumFrames;
    ++readable_count_;
  }

  uint8_t *readable_frame() {
    if (!readable_count_)
      return nullptr;
    return frames_[read_frame_];
  }

  bool readable() const {
    return readable_count_ > 0;
  }

  void read() {
    read_frame_ = (read_frame_ + 1) % kNumFrames;
    --readable_count_;
  }

private:
  uint8_t frames_[kNumFrames][kFrameSize];
  volatile size_t write_frame_;
  volatile size_t read_frame_;
  volatile size_t readable_count_;
};

#endif // FRAMEBUFFER_H_
