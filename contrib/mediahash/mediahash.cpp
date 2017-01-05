// mediahash.cpp : Defines the entry point for the console application.
//

#include "stdio.h"
#include "Hash.h"
#include "VideoFile.h"
#include <iostream>

int main(int argc, char **argv) {
  if(argc <= 1) {
    std::cout << "mediahash 0.1" << std::endl << std::endl;
    std::cout << "usage: mediahash <file>" << std::endl;
    return -1;
  }

  const char *file = argv[1];
  mediahash::VideoFile videoFile(file);

  auto hash = videoFile.getHash();

  std::cout << file << '\t' << hash.to_string() << std::endl;

  return 0;
}

