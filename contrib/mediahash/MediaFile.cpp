#include "MediaFile.h"

mediahash::MediaFile::MediaFile(std::string filename)
  : m_filename{ filename } {
}

mediahash::Hash mediahash::MediaFile::getHash() {
  if(m_hash.empty()) {
    m_hash = generateHash();
  }

  return m_hash;
}
