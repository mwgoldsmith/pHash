#include "callbacks.h"

constexpr struct {
  ph_action   action;
  const char* message;
} action_messages[] = {
  { ph_action::idle,                        "Idle" },
  { ph_action::get_total_frames,            "Calculate total number of frames in video" },
  { ph_action::get_frames_per_second,       "Calculate the frames per second of the video" },
  { ph_action::get_key_frames,              "Determine the key frames in the video" },
  { ph_action::frame_threshold_differences, "Analyze difference between frames by adaptive threshold" },
  { ph_action::find_video_segments,         "Determine each video segment" },
  { ph_action::get_segment_key_frames,      "Retrieve the frame at the start of each video segment" },
  { ph_action::key_frame_hashes,            "Calculate hash from each key frame" }
};

constexpr struct {
  ph_error    error;
  const char* message;
} error_messages[] = {
  { ph_error::file_access_error,      "Failed to access the specified file" },
  { ph_error::file_format_not_found,  "Could not identify the format of the specified file" },
  { ph_error::memory_allocate_error,  "Failed to allocate memory of the specified size" },
  { ph_error::codec_access_error,     "Could not open the required codec" },
  { ph_error::stream_info_not_found,  "Could not find any streams in the file" },
  { ph_error::video_stream_not_found, "Could not find a video stream in the file" },
  { ph_error::video_codec_not_found,  "Could not find the required codec" },
  { ph_error::video_fps_not_found,    "Could not determine the video FPS" },
  { ph_error::frame_count_error,      "Could not determine the video frame count" }
};

void callback_manager::notify(ph_action action, ph_error error) {
  if(m_callback != nullptr) {
    ph_event event;
    event.action = action;
    event.error = error;
    event.percent = -1;

    m_callback(event);
  }
}

void callback_manager::notify(ph_action action, int percent) {
  if(m_callback != nullptr) {
    ph_event event;
    event.action = action;
    event.error = ph_error::none;
    event.percent = percent;

    m_callback(event);
  }
}

static bool m_cancel_flag = false;
static callback_manager m_callbacks;

void ph_notify_error(ph_action action, ph_error error) {
  m_callbacks.notify(action, error);
}

void ph_notify_status(ph_action action, int percent) {
  m_callbacks.notify(action, percent);
}

const char* ph_get_action_msg(ph_action action) {
  for(auto msg : action_messages) {
    if(msg.action == action)
      return msg.message;
  }

  ASSERT(false);
  return nullptr;
}

const char* ph_get_error_msg(ph_error error) {
  for(auto msg : error_messages) {
    if(msg.error == error)
      return msg.message;
  }

  ASSERT(false);
  return nullptr;
}

void ph_callback_set(event_cb cb) {
  m_callbacks.set(cb);
}

void ph_callback_clear() {
  m_callbacks.clear();
}

void ph_set_cancel_flag(bool value) {
  m_cancel_flag = false;
}

bool ph_get_cancel_flag() {
  return m_cancel_flag;
}
