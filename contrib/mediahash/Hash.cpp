#include <cinttypes>
#include <memory>
#include "Hash.h"

const char mediahash::Hash::c_alpha[] = "0123456789ABCDEF";
 
mediahash::Hash::Hash()
  : p_hash{ std::unique_ptr<uint64_t[]>(new uint64_t[0]) }
  , u_size{ 0 } {
}

mediahash::Hash::Hash(uint64_t* hash, size_type size)
  : p_hash{ std::unique_ptr<uint64_t[]>(new uint64_t[size]) }
  , u_size{ size } {
  for(size_type i = 0; i < size; ++i) {
    p_hash[i] = hash[i];
  }
}

mediahash::Hash::Hash(const Hash& other)
  : p_hash{ std::unique_ptr<uint64_t[]>(new uint64_t[other.u_size]) }
  , u_size{ other.u_size } {
  for(size_type i = 0; i < other.u_size; ++i) {
    p_hash[i] = other.p_hash[i];
  }
}

mediahash::Hash& mediahash::Hash::operator=(Hash other) {
  if(&other == this)
    return *this;

  p_hash = std::unique_ptr<uint64_t[]>(new uint64_t[other.u_size]);
  u_size = other.u_size;
  for(size_type i = 0; i < other.u_size; ++i) {
    p_hash[i] = other.p_hash[i];
  }

  return *this;
}

std::string mediahash::Hash::to_string() const {
  std::string buffer{ "" };
  char cur[(sizeof(value_type) * 2) + 1];
  cur[sizeof(value_type) * 2] = '\0';

  for(size_type i = 0; i < u_size; ++i) {
    auto value = p_hash[i];

    for(int n = (sizeof(value_type) * 2) - 1; n >= 0; n--) {
      auto num = value % 16;
      cur[n] = c_alpha[num];
      value /= 16;
    }

    buffer.append(cur);
  }

  return buffer;
}

mediahash::Hash::value_type mediahash::Hash::operator[](size_type pos) {
  ASSERT(pos < u_size);
  return p_hash[pos];
}

mediahash::Hash::value_type mediahash::Hash::at(size_type pos) const {
  if(u_size <= pos)
    throw new std::out_of_range("index out of bounds");

  return p_hash[pos];
}
