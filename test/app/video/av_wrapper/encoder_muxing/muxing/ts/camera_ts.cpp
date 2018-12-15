// by wangh
#include "camera_ts.h"

CameraTs::CameraTs() {
#ifdef DEBUG
  snprintf(class_name, sizeof(class_name), "CameraTs");
#endif
  snprintf(format, sizeof(format), "mpegts");
  exit_id = MUXER_IMMEDIATE_EXIT;
}

CameraTs::~CameraTs() {}

int CameraTs::init_uri(char* uri, int rate) {
  // av_log_set_level(AV_LOG_TRACE);
  if (!strncmp(uri, "rtsp", 4)) {
    snprintf(format, sizeof(format), "rtsp");
  } else if (!strncmp(uri, "rtp", 3)) {
    snprintf(format, sizeof(format), "rtp_mpegts");
  }
  int ret = CameraMuxer::init_uri(uri, rate);
  max_video_num = rate;  // 1 seconds
  return ret;
}

void CameraTs::stop_current_job() {
  CameraMuxer::stop_current_job();
  free_packet_list();
}
