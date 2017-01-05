#ifndef CALLBACKS_H_
#define CALLBACKS_H_

#include "phash.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ph_action {
  idle,
  get_total_frames,
  get_frames_per_second,
  get_key_frames,
  frame_threshold_differences,
  find_video_segments,
  get_segment_key_frames,
  key_frame_hashes
};

enum ph_error {
  none,
  file_access_error,
  file_format_not_found,
  memory_allocate_error,
  codec_access_error,
  stream_info_not_found,
  video_stream_not_found,
  video_codec_not_found,
  video_fps_not_found,
  frame_count_error
};

struct ph_event {
  ph_action    action;
  ph_error     error;
  int          percent;
};

typedef void(*event_cb)(const ph_event&);

class callback_manager {
public:
  callback_manager()
    : m_callback(nullptr) {
  }

  void set(event_cb cb) { m_callback = cb; }
  void clear() { m_callback = nullptr; }

  void notify(ph_action action, ph_error error);
  void notify(ph_action action, int percent);

private:
  event_cb m_callback;
};

void ph_notify_error(ph_action action, ph_error error);

void ph_notify_status(ph_action action, int percent);

PHASHEXPORT const char* ph_get_action_msg(ph_action action);

PHASHEXPORT const char* ph_get_error_msg(ph_error error);

PHASHEXPORT void ph_callback_set(event_cb cb);

PHASHEXPORT void ph_callback_clear();

PHASHEXPORT void ph_set_cancel_flag(bool value);

#ifdef __cplusplus
}
#endif

#endif /* CALLBACKS_H_ */
