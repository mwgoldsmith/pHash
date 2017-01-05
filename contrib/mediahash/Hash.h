#pragma once
#include <cinttypes>
#include <memory>
#include <string>
#include <array>
#include "common.h"

namespace mediahash {

/*
*
*/
class Hash {
private:
  static const char c_alpha[];

public:
  typedef uint64_t        value_type;
  typedef unsigned int    size_type;
  typedef uint64_t*       pointer;

  Hash();
  Hash(value_type* hash, size_type size);
  Hash(const Hash& other);
  Hash& operator=(Hash other);

public:
  std::string to_string() const;
  value_type operator[](size_type pos);
  value_type at(size_type pos) const;

  constexpr bool empty() const NOEXCEPT { return u_size == 0; }
  constexpr size_type size() const NOEXCEPT { return u_size; }

private:
  std::unique_ptr<value_type[]> p_hash;
  size_type                     u_size;
};

}