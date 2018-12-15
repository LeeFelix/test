// by wangh

#ifndef ENCODE_HANDLER_H
#define ENCODE_HANDLER_H

extern "C" {
#include "video_ion_alloc.h"
#include <alsa/asoundlib.h>
#include <libavutil/time.h>

#include "watermark.h"
}
#include "mpp/rk_mpi_cmd.h"
#include "alsa_capture.h"
#include "av_wrapper/encoder_muxing/encoder/aac_encoder.h"
#include "av_wrapper/encoder_muxing/encoder/h264_encoder.h"
#include "av_wrapper/encoder_muxing/muxing/cache/camera_cache_muxer.h"
#include "av_wrapper/encoder_muxing/muxing/mp4/camera_mp4.h"
#include "av_wrapper/encoder_muxing/muxing/ts/camera_ts.h"
#include "av_wrapper/encoder_muxing/packet_dispatcher.h"
#include "av_wrapper/message_queue.h"
#include "av_wrapper/motion_detection/md_processor.h"
#include "av_wrapper/motion_detection/mv_dispatcher.h"

#define SCALED_WIDTH 640   // 320
#define SCALED_HEIGHT 384  // 240
#define SCALED_BIT_RATE 400000

typedef enum {
  START_ENCODE = 1,  // initial h264 encoding
  STOP_ENCODE,
  START_ENCODE2,  // scaled h264 encoding
  STOP_ENCODE2,
  START_CACHE,  // cache encoded data
  STOP_CACHE,
  START_RECODEMP4,
  STOP_RECODEMP4,
  START_TSTRANSFER,
  STOP_TSTRANSFER,
  SAVE_MP4_IMMEDIATELY,
  ATTACH_USER_MUXER,  // attach to initial h264
  DETACH_USER_MUXER,
  ATTACH_USER_MUXER2,  // attach to scaled h264
  DETACH_USER_MUXER2,
  ATTACH_USER_MDP,
  DETACH_USER_MDP,
  RECORD_RATE_CHANGE,
  GEN_IDR_FRAME,  // generate an idr for next encode frame
  ID_NB
} EncoderMessageID;

class EncodeHandler;

// Feedback recording duration in sec.
typedef void (*record_time_notifier)(EncodeHandler* handler, int sec);

// Be called before waiting cond.
typedef void (*call_bef_wait)(void* arg);

class FuncBefWaitMsg {
 public:
  FuncBefWaitMsg(call_bef_wait func, void* arg) : func_(func), arg_(arg) {}
  void operator()(void) { func_(arg_); }

 private:
  call_bef_wait func_;
  void* arg_;
};

/* test for ts transfer */
class BUFFEREDH264Encoder;

class EncodeHandler {
 private:
  int device_id;
  int usb_type;
  MessageQueue<EncoderMessageID>* msg_queue;

  struct alsa_capture* audio_capture;
  bool audio_enable;
  bool mute;
  pthread_t audio_tid;

#ifdef DEBUG
  pthread_t process_pid;
#endif
#if defined(STRESS_TEST) || defined(MUL_ENCODERS_DEBUG)
  int64_t t_pre_data_coming;
#endif
  struct video_ion encode_dst_buff;
  struct video_ion encode_mv_buff;
  DataBuffer_t output_buf, mv_buf;

  struct timeval start_timestamp;
  H264Encoder* h264_encoder;
  AACEncoder* aac_encoder;
  PacketDispatcher h264aac_pkt_dispatcher;
  CameraMp4* cameramp4;

  CameraCacheMuxer* cameracache;
  MVDispatcher mv_dispatcher;

  BUFFEREDH264Encoder* scale_h264_encoder;  // for scaled yuv h264 encoding
  PacketDispatcher scale_h264_pkt_dispatcher;
  CameraTs* camerats;

  EncodeHandler();
  // dev_id: camera device id
  // ac_dev_id: audio capture device id
  int init(int dev_id,
           int usb_type,
           int width,
           int height,
           int fps,
           int ac_dev_id,
           enum AVPixelFormat pixel_fmt);
  int encode_start_audio_thread();
  void encode_stop_audio_thread();
  int h264aac_encoder_ref();
  void h264aac_encoder_unref();
  int h264_encoder_ref();
  void h264_encoder_unref();
  int aac_encoder_ref();
  void aac_encoder_unref();

  int scale_h264_encoder_ref();
  void scale_h264_encoder_unref();

  int mp4_encode_start_job();
  void mp4_encode_stop_current_job();

  int send_message(EncoderMessageID id,
                   void* arg1 = NULL,
                   void* arg2 = NULL,
                   void* arg3 = NULL,
                   bool sync = false,
                   FuncBefWaitMsg* call_func = NULL);
  int attach_user_muxer(CameraMuxer* muxer,
                        char* uri,
                        bool need_video,
                        bool need_audio,
                        int encoder_id);
  void detach_user_muxer(CameraMuxer* muxer, int encoder_id);
  int attach_user_mdprocessor(VPUMDProcessor* p, MDAttributes* attr);
  void detach_user_mdprocessor(VPUMDProcessor* p);

 public:
  static const int H264_ENCODER_ID = 1;
  static const int SCALED_H264_ENCODER_ID = 2;
  char filename[128];
  pthread_attr_t* global_attr;
  // registe callback
  record_time_notifier record_time_notify;

  video_encode_config common_config;
  video_encode_config scale_h264_config;

  WATERMARK_INFO* watermark_info;

  ~EncodeHandler();
  static EncodeHandler* create(int dev_id,
                               int usb_type,
                               int w,
                               int h,
                               int fps,
                               int ac_dev_id,
                               enum AVPixelFormat pixel_fmt = AV_PIX_FMT_NV12);
  AACEncoder* get_aac_encoder() { return aac_encoder; }
  PacketDispatcher* get_h264aac_pkt_dispatcher() {
    return &h264aac_pkt_dispatcher;
  }
  struct alsa_capture* get_audio_capture() {
    return audio_capture;
  }
  bool is_audio_enable() { return audio_enable; }
  void set_enable_audio(bool en_val) { audio_enable = en_val; }
  pthread_t get_audio_tid() { return audio_tid; }
  bool get_audio_mute() { return mute; }
  void set_audio_mute(bool mute_val) { mute = mute_val; }
  void set_global_attr(pthread_attr_t* attr) { global_attr = attr; }
  void reset_video_bit_rate();

  // for usb input
  void set_pixel_format(enum AVPixelFormat pixel_fmt,
                        int encoder_id = H264_ENCODER_ID);

  void send_encoder_start(int encoder_id);
  void send_encoder_stop(int encoder_id);
  void send_record_mp4_start();
  void send_record_mp4_stop(FuncBefWaitMsg* call_func = NULL);
  void send_ts_transfer_start(char* url);
  void send_ts_transter_stop();
  void sync_ts_transter_stop();
  void send_save_file_immediately();
  void send_cache_data_start(int cache_duration);
  void send_cache_data_stop();

  /** muxer  : user muxer
   *  uri    : dst path
   *  need_av: 1, need video; 2, need audio; 3, need both
   *  channel: H264_ENCODER_ID, initial h264;
   *           SCALED_H264_ENCODER_ID, scaled h264
   */
  void send_attach_user_muxer(CameraMuxer* muxer,
                              char* uri,
                              int need_av,
                              int encoder_id = H264_ENCODER_ID);
  void sync_detach_user_muxer(CameraMuxer* muxer,
                              int encoder_id = H264_ENCODER_ID);

  void send_attach_user_mdprocessor(VPUMDProcessor* p, MDAttributes* attr);
  void sync_detach_user_mdprocessor(VPUMDProcessor* p);
  void send_rate_change(VPUMDProcessor* p, int low_bit_rate);
  void send_gen_idr_frame(int encoder_id);

  int h264_encode_process(void* virt,
                          int fd,
                          void* buf_hnd,
                          size_t size,
                          struct timeval& time_val);
  // must called before h264_encode_process in the same thread
  int h264_encode_control(int cmd, void* param);
};

#endif  // ENCODE_HANDLER_H
