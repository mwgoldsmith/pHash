#ifndef VIDEOPROCESSOR_H
#define VIDEOPROCESSOR_H

#include <cstdint>
#include <vector>

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

#endif /* VIDEOPROCESSOR_H */