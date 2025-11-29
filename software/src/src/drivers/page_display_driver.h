// page_display_driver.h - Page-based display driver wrapper
//
// Copyright (c) 2016 Patrick Dowling

#ifndef PAGE_DISPLAY_DRIVER_H_
#define PAGE_DISPLAY_DRIVER_H_

#include <stdint.h>

template <typename display_driver>
class PagedDisplayDriver {
public:
  static constexpr size_t kNumPages = display_driver::kNumPages;
  static constexpr size_t kPageSize = display_driver::kPageSize;

  void Init() {
    display_driver::Init();
    frame_ = nullptr;
    page_ = 0;
  }

  bool Flush() {
    display_driver::Flush();
    return frame_ == nullptr;
  }

  void Begin(const uint8_t *frame) {
    frame_ = frame;
    page_ = 0;
  }

  bool frame_valid() const {
    return frame_ != nullptr;
  }

  void Update() {
    if (frame_) {
      if (display_driver::SendPage(page_, frame_)) {
        frame_ += kPageSize;
        ++page_;
        if (page_ >= kNumPages)
          frame_ = nullptr;
      }
    }
  }

private:
  const uint8_t *frame_;
  size_t page_;
};

#endif // PAGE_DISPLAY_DRIVER_H_
