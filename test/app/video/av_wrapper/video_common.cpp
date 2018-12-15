// by wangh
#include "video_common.h"
extern "C" {
#include <libavutil/time.h>
}

#if 0
#include <unwind.h>
#include <execinfo.h>
#include <signal.h>
void dump(int signo) {
    printf("signo:%d\n",signo);
    void *array[100];
    size_t size;
    char **strings;
    size_t i;
    size = backtrace (array, 100);
    fprintf (stderr,"Obtained %d stack frames\n", size);
    strings = backtrace_symbols (array, size);
    if (!strings) {
        fprintf (stderr,"backtrace string is NULL\n");
    } else {
        fprintf (stderr,"backtrace string is NOT NULL\n");
        for (i = 0; i < size; i++)
            fprintf (stderr,"%d:- %s\n", i, strings[i]);
        free (strings);
    }
    exit(-1);
}
#endif

#ifdef DEBUG
AutoPrintInterval::AutoPrintInterval(char* s) {
  snprintf(str, sizeof(str), "%s", s);
  t_start = av_gettime_relative();
  printf("!before %s\n", s);
}

AutoPrintInterval::~AutoPrintInterval() {
  printf("!after %s, interval : %lld ms\n", str,
         (av_gettime_relative() - t_start) / 1000);
}
#endif

void video_object::ref() {
  __atomic_add_fetch(&ref_cnt, 1, __ATOMIC_SEQ_CST);
}
int video_object::unref() {
  if (!__atomic_sub_fetch(&ref_cnt, 1, __ATOMIC_SEQ_CST)) {
    delete this;
    return 0;
  }
  return 1;
}
int video_object::refcount() {
  return __atomic_load_n(&ref_cnt, __ATOMIC_SEQ_CST);
}

EncodedPacket::EncodedPacket() {
  av_init_packet(&av_pkt);
  type = PACKET_TYPE_COUNT;
  memset(&time_val, 0, sizeof(struct timeval));
  audio_index = 0;
  is_phy_buf = false;
  memset(&lbr_time_val, 0, sizeof(lbr_time_val));
}

EncodedPacket::~EncodedPacket() {
  // printf("pkt: %p, free av_pkt: %d\n", this, av_pkt.size);
  av_packet_unref(&av_pkt);
}

int EncodedPacket::copy_av_packet() {
  assert(is_phy_buf);
  AVPacket pkt = av_pkt;
  if (0 != av_copy_packet(&av_pkt, &pkt)) {
    printf("in %s, av_copy_packet failed!\n", __func__);
    return -1;
  }
  is_phy_buf = false;
  return 0;
}
