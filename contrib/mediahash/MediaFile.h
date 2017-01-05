#pragma once
#include "Hash.h"
#include <string>

namespace mediahash {

/*
*
*/
class MediaFile {
public:
  explicit MediaFile(std::string filename);
  virtual ~MediaFile() {}

private:
  virtual Hash generateHash() const = 0;

public:
  Hash getHash();

  std::string getFilename() const {
    return m_filename;
  }

private:
  std::string m_filename;
  Hash        m_hash;
};

}