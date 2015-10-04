/*

    pHash, the open source perceptual hash library
    Copyright (C) 2009 Aetilius, Inc.
    All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Evan Klinger - eklinger@phash.org
    David Starkweather - dstarkweather@phash.org

*/

#include "stdafx.h"
#include "CppUnitTest.h"
#include "stdio.h"
#include "phash.h"
#include <iomanip>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace phash_tests {

/* structure for a single hash */
struct VideoHash {
  explicit VideoHash(const char* filename)
    : m_filename(strdup(filename))
      , m_hash(nullptr)
      , m_size(0) {
    int n;
    m_hash = ph_dct_videohash(filename, n);
    m_size = m_hash != nullptr ? n : -1;
  }

  ~VideoHash() {
    if (m_filename != nullptr) {
      free(m_filename);
    }
    if (m_hash != nullptr) {
      free(m_hash);
    }
  }

  VideoHash(VideoHash&& other)
    : m_filename(std::move(other.m_filename))
      , m_hash(std::move(other.m_hash))
      , m_size(std::move(other.m_size)) {
    other.m_filename = nullptr;
    other.m_hash = nullptr;
    other.m_size = 0;
  };

  VideoHash& operator=(VideoHash&& other) {
    if (this != &other) {
      m_filename = std::move(other.m_filename);
      m_hash = std::move(other.m_hash);
      m_size = std::move(other.m_size);
      other.m_filename = nullptr;
      other.m_hash = nullptr;
      other.m_size = 0;
    }

    return *this;
  }

  inline const char* getFilename() const {
    return m_filename;
  }

  inline ulong64* getHash() const {
    return m_hash;
  }

  inline ulong64 getHash(int index) const {
    if (m_hash == nullptr || static_cast<unsigned int>(index) > (m_size - 1))
      return 0;

    return m_hash[index];
  }

  inline int getSize() const {
    return m_size;
  }

  inline double compare(const VideoHash& other, int threashold = 21) {
    return ph_dct_videohash_dist(m_hash, m_size, other.getHash(), other.getSize(), threashold);
  }

private:
  char* m_filename;
  ulong64* m_hash;
  int m_size;
};

static wchar_t* strToWChar(std::string &s)
{
  const char * text = s.c_str();
  size_t size = strlen(text) + 1;
  wchar_t* wa = new wchar_t[size];
  mbstowcs(wa, text, size);
  return wa;
}

VideoHash create_file_video_hash(const char* filename) {
  Logger::WriteMessage(strToWChar(std::string("File: ").append(filename)));

  VideoHash hash{ filename };
  auto len = hash.getSize();
  if (len > 0) {
    printf("length %d\n", len);
    for (auto i = 0; i < len; i++) {
      printf("hash1[%d]=%llx\n", i, hash.getHash(i));
    }
  }

  return hash;
}

std::string ulong64_to_hex(ulong64 value) {
  std::stringstream s;
  s.precision(1);
  return s.str();
}

template <class DataType>
std::string CStr(const DataType  &value){
  std::stringstream strstrmOutStream;
  strstrmOutStream << value;
  return strstrmOutStream.str();
}

std::string CStr(const double  &value) {
  std::stringstream s;
  s.precision(1);
  s << std::fixed << value;
  return s.str();
}

std::string CStr(const ulonng65  &value) {
  std::stringstream s;
  s.precision(1);
  s << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << value;
  return s.str();
}


TEST_CLASS(VideoHashTests) {
public:
  TEST_METHOD(WhenSimilarVideoWithDCTHashTest) {
    auto hash1 = create_file_video_hash("C:\\projects\\libphash\\datao\\divx-0780.avi");
    Assert::IsTrue(hash1.getSize() > 0, L"Could not create a DCT hash for video file 1");

    auto hash2 = create_file_video_hash("C:\\projects\\libphash\\datao\\divx-1500.avi");
    Assert::IsTrue(hash2.getSize() > 0, L"Could not create a DCT hash for video file 2");

    auto sim = hash1.compare(hash2);
    Logger::WriteMessage(strToWChar(std::string("Similarity: ").append(double_to_str(sim))));
  }

};

}
