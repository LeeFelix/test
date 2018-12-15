// by wangh
#ifndef VIDEO_COMMON_H
#define VIDEO_COMMON_H

#include <condition_variable>
#include <list>
#include <mutex>

#ifdef DEBUG
#include <assert.h>
#else
#undef assert
#define assert(A)
#endif
extern "C" {
#include <libavcodec/avcodec.h>
}

#ifdef DEBUG
#include <execinfo.h>
#include <signal.h>
#include <unwind.h>
void dump(int signo);
#define REGISTER_SIGNAL_SIGSEGV \
  {                             \
    signal(SIGILL, dump);       \
    signal(SIGABRT, dump);      \
    signal(SIGBUS, dump);       \
    signal(SIGFPE, dump);       \
    signal(SIGSEGV, dump);      \
    signal(SIGPIPE, dump);      \
  }
#else
#define REGISTER_SIGNAL_SIGSEGV
#endif

#ifdef DEBUG
#define PRINTF_FUNC printf("%s, thread: 0x%x\n", __func__, pthread_self())
#define PRINTF_FUNC_OUT \
  printf("---- OUT OF %s, thread: 0x%x ----\n", __func__, pthread_self())
#define PRINTF_FUNC_LINE printf("~~ %s: %d\n", __func__, __LINE__)
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF_FUNC
#define PRINTF_FUNC_OUT
#define PRINTF_FUNC_LINE
#define PRINTF(...)
#endif

#ifdef DEBUG
class AutoPrintInterval {
 private:
  char str[64];
  int64_t t_start;

 public:
  AutoPrintInterval(char* s);
  ~AutoPrintInterval();
};
#endif

#define PRINTF_NO_MEMORY                              \
  printf("no memory! %s : %d\n", __func__, __LINE__); \
  assert(0)

#define THREAD_STACK_SIZE (128 * 1024)

#define VIDEO_BIT_RATE 400000
#define STREAM_FRAME_RATE 30
#define VIDEO_ENCODER_LEVEL 51
#define VIDEO_GOP_SIZE 30

typedef struct {
  int width;
  int height;
  enum AVPixelFormat pixel_fmt;
  int video_bit_rate;
  int stream_frame_rate;
  int video_encoder_level;
  int video_gop_size;
  int video_profile;

  int audio_bit_rate;       // 64000
  int audio_sample_rate;    // 44100
  uint64_t channel_layout;  // MONO
  int nb_channels;
  int input_nb_samples;  // 1024, codec may rewrite nb_samples; aac 1024 is
                         // better; for muxing
  enum AVSampleFormat input_sample_fmt;  // for muxing

  int out_nb_samples;                  // for demuxing
  enum AVSampleFormat out_sample_fmt;  // for demuxing
} video_encode_config, media_info;

typedef enum { VIDEO_PACKET, AUDIO_PACKET, PACKET_TYPE_COUNT } PacketType;

class video_object {
 private:
  volatile int ref_cnt;

 public:
  video_object() : ref_cnt(1) {}
  virtual ~video_object() {
    // printf("%s: %p\n", __func__, this);
    assert(ref_cnt == 0);
  }
  void ref();
  int unref();  // return 0 means object is deleted
  int refcount();
  void resetRefcount() {
    //__atomic_store_n(&ref_cnt, 0, __ATOMIC_SEQ_CST);
    ref_cnt = 0;
  }
};

class EncodedPacket : public video_object {
 public:
  AVPacket av_pkt;
  PacketType type;
  bool is_phy_buf;
  union {
    struct timeval time_val;  // store the time for video_packet
    uint64_t audio_index;     // audio packet index
  };
  struct timeval lbr_time_val;
  EncodedPacket();
  ~EncodedPacket();
  int copy_av_packet();
};

#define UNUSED(x) (void)x

#define CREATE_FUNC(__TYPE__)        \
  static __TYPE__* create() {        \
    __TYPE__* pRet = new __TYPE__(); \
    if (pRet && !pRet->init()) {     \
      return pRet;                   \
    } else {                         \
      if (pRet)                      \
        delete pRet;                 \
      return NULL;                   \
    }                                \
  }

typedef void* (*Runnable)(void* arg);

#endif  // VIDEO_COMMON_H
