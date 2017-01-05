#include "Hash.h"
#include "VideoFile.h"
#include <iostream>
#include <phash.h>
#include <callbacks.h>

mediahash::Hash mediahash::VideoFile::generateHash() const {
  ph_callback_set([](const ph_event& event) {
    if(event.error != ph_error::none) {
      std::cout << ph_get_action_msg(event.action) << " (ERROR: " << ph_get_error_msg(event.error) << ")" << std::endl;
    } else {
      std::cout << ph_get_action_msg(event.action) << " (" << event.percent << "\%)" << std::endl;
    }
  });

  int len;
  auto hashPtr = ph_dct_videohash(getFilename().c_str(), len);
  if(hashPtr == nullptr) {
    throw new std::exception();
  }
  
  return Hash(hashPtr, len);
}

