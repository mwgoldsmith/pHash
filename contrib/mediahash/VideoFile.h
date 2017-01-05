#pragma once
#include "MediaFile.h"

namespace mediahash {

/*
*
*/
class VideoFile : public MediaFile {
public:
  explicit VideoFile(const std::string& filename)
    : MediaFile(filename) {
  }

public:
  Hash generateHash() const override;
};

}
