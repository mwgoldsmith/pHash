#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <vector>
#include <memory>

#include "internal.h"
#include "MediaContext.h"

class VideoProcessor {
protected:
  explicit VideoProcessor(MediaContextPtr pContext);

private:
  int ReadFrames(CImageIListPtr pFrameList) const;

  bool CalculateFrameDifferential(float* pFrameDiff) const;

  std::vector<uint64_t> CalculateBoundryFrames(float* pFrameDiff) const;

public:
  static CImageIListPtr GetSceneChangeFrames(const char* filename);

private:
  MediaContextPtr m_pContext;
};
