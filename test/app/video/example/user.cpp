/**
 * example for how to get the h264 data
 */

#include <sys/time.h>
extern "C" {
#include "../parameter.h"
}
#include "../av_wrapper/encoder_muxing/muxing/camera_muxer.h"
#include "../av_wrapper/motion_detection/md_processor.h"
#include "../av_wrapper/video_common.h"
#include "../video.h"
#include "user.h"

/**
 * please read the implement of CameraMuxer
 * overwrite some virtual functions in UserMuxer
 */
class UserMuxer : public CameraMuxer {
 private:
  UserMuxer();

 public:
  /** Must overwrite these fuctions if you donot need ffmpeg's logic
   *  of course, you can also overwrite other functions you need
   */
  int init_muxer_processor(MuxProcessor* process);
  void deinit_muxer_processor(AVFormatContext* oc);
  int muxer_write_header(AVFormatContext* oc, char* url);
  int muxer_write_tailer(AVFormatContext* oc);
  int muxer_write_free_packet(MuxProcessor* process, EncodedPacket* pkt);
  int muxer_write_packet(AVFormatContext* ctx, AVStream* st, AVPacket* pkt);

  CREATE_FUNC(UserMuxer)
};

UserMuxer::UserMuxer() {
// example code
#ifdef DEBUG
  snprintf(class_name, sizeof(class_name), "user_muxer");
#endif
  exit_id = MUXER_NORMAL_EXIT;
  no_async = true;  // no cache packet list and no thread handle
}

int UserMuxer::init_muxer_processor(MuxProcessor* process) {
  // Non-implement, TODO
  return 0;
}

void UserMuxer::deinit_muxer_processor(AVFormatContext* oc) {
  // Non-implement, TODO
}

int UserMuxer::muxer_write_header(AVFormatContext* oc, char* url) {
  // Non-implement, TODO
  return 0;
}

int UserMuxer::muxer_write_tailer(AVFormatContext* oc) {
  // Non-implement, TODO
  return 0;
}

int UserMuxer::muxer_write_free_packet(MuxProcessor* process,
                                       EncodedPacket* pkt) {
  return CameraMuxer::muxer_write_free_packet(process, pkt);
}

int UserMuxer::muxer_write_packet(AVFormatContext* ctx,
                                  AVStream* st,
                                  AVPacket* pkt) {
  assert(pkt);
  printf("got data: %p, size: %dbytes \n", pkt->data, pkt->size);
  return 0;
}

static UserMuxer* user_muxer = NULL;
static void (*user_event_call)(int cmd, void* msg0, void* msg1);

extern "C" int user_entry() {
#if 0
    //example code
    user_muxer = UserMuxer::create();
    if (user_muxer) {
        //video node setup is uncertain, trouble issue.
        //Use post message to send msg which will be handled after video node is setup
        if (user_event_call)
          (*user_event_call)(CMD_USER_MUXER, (void *)user_muxer, (void *)"-----");
        return 0;
    }
    return -1;
#endif
  return 0;
}

extern "C" void user_exit() {
#if 0
    //example code
    if (user_muxer) {
        video_record_detach_user_muxer((void*)user_muxer);
        delete user_muxer;
        user_muxer = 0;
    }
#endif
}

class UserMDProcessor : public VPUMDProcessor {
 private:
  struct timeval tval;
  volatile int force_result;

 public:
  UserMDProcessor() : VPUMDProcessor() {
    memset(&tval, 0, sizeof(tval));
    force_result = -1;
  }
  int get_force_result() {
    return __atomic_exchange_n(&force_result, -1, __ATOMIC_SEQ_CST);
  }
  void set_force_result(int val) {
    __atomic_store_n(&force_result, val, __ATOMIC_SEQ_CST);
  }
  int calc_time_val(MDResult& result, bool& set_val, bool force_timeout) {
    int low = get_low_frame_rate();
    printf(
        "md result: %d, low frame rate: %d, force_timeout: %d,"
        "tval.tv_sec: %ld, tval.tv_usec: %d\n",
        result.change, low, force_timeout, tval.tv_sec, tval.tv_usec);
    if (result.change) {
      memset(&tval, 0, sizeof(tval));
      if (low) {
        set_val = false;
        return 1;
      }
    } else if (!result.change && low <= 0) {
      if (force_timeout) {
        memset(&tval, 0, sizeof(tval));
        set_val = true;
        return 1;
      }
      struct timeval now;
      gettimeofday(&now, NULL);
      if (tval.tv_sec == 0 && tval.tv_usec == 0) {
        tval = now;
        return 0;
      } else {
        if ((now.tv_sec - tval.tv_sec) * 1000000LL + now.tv_usec -
                tval.tv_usec >=
            10 * 1000000LL) {
          memset(&tval, 0, sizeof(tval));
          set_val = true;
          return 1;
        }
      }
    }
    return 0;
  }
  int push_mv_data(void* mv_data, void* raw_yuv, size_t yuv_size) {
    // example code
    return VPUMDProcessor::push_mv_data(mv_data, raw_yuv, yuv_size);
  }
  uint32_t simple_process(void* mv_data) {
    // example code
    return VPUMDProcessor::simple_process(mv_data);
  }
};

static UserMDProcessor* md_process = NULL;
static MDAttributes md_attr = {0};

#define DEBUG_MD 0
#if DEBUG_MD
static bool haha = true;
#endif

static void MD_Result_doing(MDResult result, void* arg) {
  UserMDProcessor* p = (UserMDProcessor*)arg;
  bool low_frame_rate = false;
#if DEBUG_MD
  if (haha) {
    result.change = true;
    haha = false;
  } else {
    result.change = false;
  }
#endif
  int force_change = p->get_force_result();
  bool force_timeout = false;
  if (force_change >= 0) {
    result.change = force_change;
    force_timeout = true;
  }
  printf("md result: %d\n", result.change);
  if (p->calc_time_val(result, low_frame_rate, force_timeout)) {
    assert(sizeof(void*) >= sizeof(UserMDProcessor*));
    assert(sizeof(void*) >= sizeof(bool));
    if (user_event_call)
      (*user_event_call)(CMD_USER_RECORD_RATE_CHANGE, (void*)p,
                         (void*)low_frame_rate);
  }
}

extern "C" void start_motion_detection() {
  if (parameter_get_video_de() == 1 && !md_process) {
    md_process = new UserMDProcessor();
    if (md_process) {
      md_process->register_result_cb(MD_Result_doing, (void*)md_process);
      md_attr.horizontal_area_num = 0;  // unused
      md_attr.vertical_area_num = 0;    // unused
      md_attr.time_interval = 500;      // milliseconds
      md_attr.weight = 5;
      md_attr.buffered_frames = 1;
      if (user_event_call)
        (*user_event_call)(CMD_USER_MDPROCESSOR, (void*)md_process,
                           (void*)&md_attr);
    }
    printf("start_motion_detection md_process: %p\n", md_process);
  }
}

extern "C" void stop_motion_detection() {
  // example code
  // assert(parameter_get_video_de() == 0);
  printf("stop_motion_detection md_process: %p\n", md_process);
  if (md_process) {
    PRINTF_FUNC_LINE;
    video_record_detach_user_mdprocessor((void*)md_process);
    PRINTF_FUNC_LINE;
    delete md_process;
    PRINTF_FUNC_LINE;
    md_process = 0;
#if DEBUG_MD
    haha = true;
#endif
  }
}

extern "C" int set_motion_detection_trigger_value(bool result) {
  if (md_process) {
    md_process->set_force_result(result ? 1 : 0);
    return 0;
  }
  return -1;
}

extern "C" int USER_RegEventCallback(void (*call)(int cmd,
                                                  void* msg0,
                                                  void* msg1)) {
  user_event_call = call;
  return 0;
}
