#include <CameraHal/CamCifDevHwItf.h>
#include <CameraHal/CamHalVersion.h>
#include <CameraHal/CamHwItf.h>
#include <CameraHal/CamIsp11DevHwItf.h>
#include <CameraHal/CamUSBDevHwItfImc.h>
#include <CameraHal/CameraBuffer.h>
#include <CameraHal/CameraIspTunning.h>
#include <CameraHal/IonCameraBuffer.h>
#include <CameraHal/StrmPUBase.h>
#include <CameraHal/v4l2-config_rockchip.h>
#include <iostream>

#include <linux/videodev2.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {
#include "common.h"
#include <iep/iep.h>
#include <iep/iep_api.h>
#include "fs_manage/fs_fileopera.h"
#include "fs_manage/fs_manage.h"
#include "huffman.h"
#include "rk_rga/rk_rga.h"
#include "video_ion_alloc.h"
#include "vpu.h"

#include <dpp/dpi.h>
#include <dpp/dpp_buffer.h>
#include <dpp/dpp_err.h>
#include <dpp/dpp_frame.h>
#include <dpp/dpp_packet.h>
#include <jpeglib.h>

#include "watermark.h"
#include "temperature.h"
#include "ui_resolution.h"
}

#include "encode_handler.h"
#include "jpeg_header.h"
#include "video_interface.h"

#define MAX_VIDEO_DEVICE 10

#define ADAS_BUFFER_NUM 3
#define ADAS_BUFFER_WIDTH 320
#define ADAS_BUFFER_HEIGHT 240
#define ADAS_FRAME_COUNT 10

#define LDW_TYPE_DAY 1
#define LDW_TYPE_NIGHT 0

//#define NV12_RAW_DATA
#define NV12_RAW_CNT 3

#define STACKSIZE (256 * 1024)

using namespace std;

#define FRONT "isp"
#define USB_FMT_TYPE HAL_FRMAE_FMT_MJPEG

class YUYV_NV12_Stream;
class NV12_Display;
class MJPG_Photo;
class MJPG_NV12;
class NV12_Encode;
class H264_Encode;
class SP_Display;
class MP_DSP;
class NV12_MJPG;
class NV12_RAW;
class NV12_ADAS;
class NV12_IEP;

struct hal {
  shared_ptr<CamHwItf> dev;
  shared_ptr<CamHwItf::PathBase> mpath;
  shared_ptr<CamHwItf::PathBase> spath;
  shared_ptr<YUYV_NV12_Stream> yuyv_nv12;
  shared_ptr<NV12_Display> nv12_disp;
  shared_ptr<MJPG_Photo> mjpg_photo;
  shared_ptr<MJPG_NV12> mjpg_nv12;
  shared_ptr<NV12_Encode> nv12_enc;
  shared_ptr<H264_Encode> h264_enc;
  shared_ptr<IonCameraBufferAllocator> bufAlloc;
  shared_ptr<SP_Display> sp_disp;
  shared_ptr<MP_DSP> mp_dsp;
  shared_ptr<NV12_MJPG> nv12_mjpg;
  shared_ptr<NV12_RAW> nv12_raw;
  shared_ptr<NV12_ADAS> nv12_adas;
  shared_ptr<NV12_IEP> nv12_iep;
};

typedef struct ispinfo {
  float exp_gain;
  float exp_time;
  int doortype;

  unsigned char exp_mean[25];

  float wb_gain_red;
  float wb_gain_green_r;
  float wb_gain_blue;
  float wb_gain_green_b;

  int reserves[16];
} ispinfo_t;

static list<shared_ptr<CameraBuffer>> pool;
static pthread_mutex_t pool_lock;

static pthread_attr_t global_attr;

static bool is_record_mode = true;

static int nv12_raw_cnt = 0;

static unsigned int fb_width;
static unsigned int fb_height;  // fb_width >= fb_height

static int temperature;
static int clk_core;
static unsigned int user_noise = 0;

struct dpp_buffer {
  DppBuffer buffer;
  struct timeval pts;
  uint32_t noise[4];
  struct dpp_sharpness sharpness;
  struct HAL_Buffer_MetaData* isp_meta;
};

struct dpp_buffer_list {
  list<struct dpp_buffer*> buffers;
  pthread_mutex_t mutex;
  pthread_cond_t condition;
};

struct video_dpp {
  DppCtx context;
  unsigned int fun;
  pthread_t dpp_thread;
  pthread_t encode_thread;
  pthread_t photo_thread;
  pthread_t display_thread;
  struct dpp_buffer_list encode_buffer_list;
  struct dpp_buffer_list photo_buffer_list;
  struct dpp_buffer_list display_buffer_list;
  bool stop_flag;
};

struct video_adas {
  DppCtx context;
  unsigned int fun;
  pthread_t dpp_id;
  struct video_ion input[ADAS_BUFFER_NUM];
  list<struct video_ion*> pool;
  pthread_mutex_t pool_lock;
  dsp_ldw_out out;
  int rga_fd;
};

struct video_display {
  bool displaying;
  bool display;
  struct video_ion yuyv;
  int rga_fd;
  struct timeval t0;
};

enum photo_state { PHOTO_ENABLE, PHOTO_BEGIN, PHOTO_END, PHOTO_DISABLE };

struct video_photo {
  enum photo_state state;
  struct video_ion rga_photo;
  struct vpu_encode encode;
  pthread_t pid;
  pthread_mutex_t mutex;
  pthread_cond_t condition;
  int rga_fd;
};

struct video_jpeg_dec {
  struct video_ion out;
  struct vpu_decode* decode;
  bool decoding;
  int rga_fd;
};

#define JPEG_STREAM_NUM 2
struct JpegConfig {
  int width;
  int height;
};

static void* jpeg_encode_one_frame(void* arg);

class JpegStreamReceiver {
 public:
  struct video_ion yuv_hw_buf;
  bool is_processing;
  JpegStreamReceiver()
      : data_cb(nullptr), request_encode(0), is_processing(false) {
    memset(&yuv_hw_buf, 0, sizeof(yuv_hw_buf));
    yuv_hw_buf.client = -1;
    yuv_hw_buf.fd = -1;
  }

  ~JpegStreamReceiver() { video_ion_free(&yuv_hw_buf); }

  inline void set_notify_new_enc_stream_callback(NotifyNewEncStream cb) {
    data_cb = cb;
  }

  inline int get_request_encode() {
    return __atomic_load_n(&request_encode, __ATOMIC_SEQ_CST);
  }

  inline void set_request_encode(bool val) {
    int expected = val ? 0 : 1;
    __atomic_compare_exchange_n(&request_encode, &expected, val ? 1 : 0, false,
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  }

  int process(int src_fd, int src_w, int src_h, struct JpegConfig& config) {
    if (is_processing)
      return 0;

    int ret = 0;
    pthread_t pid = 0;
    int dst_w = config.width;
    int dst_h = config.height;
    if (yuv_hw_buf.width != dst_w || yuv_hw_buf.height != dst_h) {
      video_ion_free(&yuv_hw_buf);
      ret = video_ion_alloc(&yuv_hw_buf, dst_w, dst_h);
      if (ret) {
        printf("%s yuv ion alloc<%d x %d> fail!\n", __func__, dst_w, dst_h);
        return -1;
      }
    }

    printf("src w,h : [%d x %d], dst w,h : [%d x %d]\n", src_w, src_h, dst_w,
           dst_h);
    assert(src_w != 0 && src_h != 0 && dst_w != 0 && dst_h != 0);
    ret = rk_rga_ionfdnv12_to_ionfdnv12_scal_ext(src_fd, src_w, src_h,
                                                 yuv_hw_buf.fd, dst_w, dst_h, 0,
                                                 0, dst_w, dst_h, src_w, src_h);
    if (ret) {
      printf("%s rga scale fail!\n", __func__);
      return -1;
    }
    // create threads for every request?
    ret = pthread_create(&pid, &global_attr, jpeg_encode_one_frame, this);
    if (ret) {
      printf("%s pthread create fail!\n", __func__);
      return -1;
    }
    is_processing = true;
    return 0;
  }

  void notify(void* buf, int size, int width, int height) {
    if (data_cb) {
      VEncStreamInfo info = {.frm_type = JPEG_FRAME,
                             .buf_addr = buf,
                             .buf_size = size,
                             .time_val = {0, 0},
                             .ExtraInfo = {.jpeg_info = {width, height}}};
      data_cb(&info);
    }
  }

 private:
  NotifyNewEncStream data_cb;
  volatile int request_encode;
};

static void* jpeg_encode_one_frame(void* arg) {
  JpegStreamReceiver* receiver = (JpegStreamReceiver*)arg;
  struct video_ion& yuv_hw_buf = receiver->yuv_hw_buf;
  void* src_buf = yuv_hw_buf.buffer;
  int src_size = yuv_hw_buf.width * yuv_hw_buf.height * 3 / 2;
  int src_fd = yuv_hw_buf.fd;

#ifdef USE_WATERMARK
  //if(video_photo_watermark(video) == -1)
#endif
  {
    struct vpu_encode encode;
    memset(&encode, 0, sizeof(encode));
    int ret =
        vpu_nv12_encode_jpeg_init(&encode, yuv_hw_buf.width, yuv_hw_buf.height);
    if (!ret)
      ret = vpu_nv12_encode_jpeg_doing(&encode, src_buf, src_fd, src_size, -1);
    receiver->set_request_encode(false);
    if (!ret)
      receiver->notify(encode.enc_out->data, encode.enc_out->size,
                       yuv_hw_buf.width, yuv_hw_buf.height);
    else
      receiver->notify(NULL, 0, 0, 0);
    vpu_nv12_encode_jpeg_done(&encode);
  }
  receiver->is_processing = false;
  pthread_detach(pthread_self());
  pthread_exit(NULL);
}

struct Video {
  unsigned char businfo[32];
  int width;
  int height;

  struct hal* hal;

  struct Video* pre;
  struct Video* next;

  int type;
  int usb_type;
  pthread_t record_id;
  int deviceid;
  volatile int pthread_run;
  char save_en;

  struct video_dpp* dpp;
  bool dpp_init;
  struct video_adas* adas;

  bool mp4_encoding;

  pthread_mutex_t record_mutex;
  pthread_cond_t record_cond;

  frm_info_t frmFmt;
  frm_info_t spfrmFmt;

  // Means the video starts successfully with all below streams.
  // We need know whether the video is valid in some critical cases.
  bool valid;
  // Maintain the encode status internally, user call interfaces too wayward.
  // Service the start_xx/stop_xx related to encode_hanlder.
  uint32_t encode_status;
#define RECORDING_FLAG 0x00000001
#define WIFI_TRANSFER_FLAG 0x00000002
  EncodeHandler* encode_handler;
  WATERMARK_INFO watermark_info;

  bool cachemode;

  int fps_total;
  int fps_last;
  int fps;

  bool video_save;
  bool front;

  bool high_temp;

  int fps_n;
  int fps_d;

#ifdef NV12_RAW_DATA
  struct video_ion raw[NV12_RAW_CNT];
  int raw_fd;
#endif

  struct video_photo photo;

  struct video_display disp;

  struct video_jpeg_dec jpeg_dec;

  // ugly code, for adapt dingdingpai
  struct JpegConfig jpeg_config[JPEG_STREAM_NUM];
  JpegStreamReceiver* jpeg_receiver[JPEG_STREAM_NUM];
};

extern "C" {
#include "number.h"
#include "parameter.h"
}

#include "video.h"

struct Video* video_list = NULL;
static pthread_rwlock_t notelock;
static int enablerec = 0; // TODO: enablerec seems not necessary
static int enableaudio = 1;

static list<pthread_t> record_id_list;

static bool with_mp = false;
static bool with_adas = false;
static bool with_sp = false;

static struct rk_cams_dev_info g_test_cam_infos;
static void (*rec_event_call)(int cmd, void *msg0, void *msg1);

static void fps_count(struct timeval* t0, int* i, const char* name) {
  long int time = 0;
  struct timeval tv;
  struct timeval* t1 = &tv;
  gettimeofday(t1, NULL);
  if (t0->tv_sec != 0) {
    time = (t1->tv_sec - t0->tv_sec) * 1000000 + t1->tv_usec - t0->tv_usec;
    if (time >= 30000000) {
      printf("%s fps:%.2f\n", name, *i / 30.0);
      *i = 0;
      t0->tv_sec = t1->tv_sec;
      t0->tv_usec = t1->tv_usec;
    }
  } else {
    t0->tv_sec = t1->tv_sec;
    t0->tv_usec = t1->tv_usec;
  }
}

static void video_record_wait(struct Video* video) {
  pthread_mutex_lock(&video->record_mutex);
  if (video->pthread_run)
    pthread_cond_wait(&video->record_cond, &video->record_mutex);
  pthread_mutex_unlock(&video->record_mutex);
}

static void video_record_signal(struct Video* video) {
  pthread_mutex_lock(&video->record_mutex);
  video->pthread_run = 0;
  pthread_cond_signal(&video->record_cond);
  pthread_mutex_unlock(&video->record_mutex);
}

static struct Video* getfastvideo(void) {
  return video_list;
}

static int show_one_number(int width,
                           int height,
                           void* dstbuf,
                           int n,
                           int x_pos,
                           int y_pos) {
  int i, j, l;
  unsigned char y = 16, u = 128, v = 128;  // black
  unsigned char *y_addr = NULL, *uv_addr = NULL;
  unsigned char *start_y = NULL, *start_uv = NULL;

  if (n >= 13 || n < 0 || x_pos < 0 || (x_pos + FONTX) >= width || y_pos < 0 ||
      (y_pos + FONTY) >= height) {
    printf("error input number or position.\n");
    return -1;
  }

  y_addr = (unsigned char*)dstbuf + y_pos * width + x_pos;
  uv_addr = (unsigned char*)dstbuf + width * height + y_pos / 2 * width + x_pos;

  if (*y_addr > 0 && *y_addr < 50)
    y = 160;

  for (j = 0; j < FONTY; j++) {
    for (i = 0; i < FONTX / 8; i++) {
      start_y = y_addr + j * width + i * 8;
      start_uv = uv_addr + j / 2 * width + i * 8;
      for (l = 0; l < 8; l++) {
        if (number[n][j][i] >> l & 0x01) {
          *(start_y + l) = y;
          if (j % 2 == 0) {
            if ((i * 8 + l) % 2 == 0)
              *(start_uv + l) = u;
            else
              *(start_uv + l) = v;
          }
        }
      }
    }
  }

  return 0;
}

static int show_one_alpha(int width,
                          int height,
                          void* dstbuf,
                          char al,
                          int x_pos,
                          int y_pos) {
  int i, j, l;
  unsigned char y = 16, u = 128, v = 128;  // black
  unsigned char *y_addr = NULL, *uv_addr = NULL;
  unsigned char *start_y = NULL, *start_uv = NULL;
  int n = -1;
  char alpha = al;

  if (alpha >= 'a' && alpha <= 'z')
    alpha = alpha - ('a' - 'A');
  if (alpha < 'A' || alpha > 'Z')
    return -1;
  n = alpha - 'A';

  if (n >= 26 || n < 0 || x_pos < 0 || (x_pos + FONTX) >= width || y_pos < 0 ||
      (y_pos + FONTY) >= height) {
    printf("error input number or position.\n");
    return -1;
  }

  y_addr = (unsigned char*)dstbuf + y_pos * width + x_pos;
  uv_addr = (unsigned char*)dstbuf + width * height + y_pos / 2 * width + x_pos;

  if (*y_addr > 0 && *y_addr < 50)
    y = 160;

  for (j = 0; j < FONTY; j++) {
    for (i = 0; i < FONTX / 8; i++) {
      start_y = y_addr + j * width + i * 8;
      start_uv = uv_addr + j / 2 * width + i * 8;
      for (l = 0; l < 8; l++) {
        if (alphabet[n][j][i] >> l & 0x01) {
          *(start_y + l) = y;
          if (j % 2 == 0) {
            if ((i * 8 + l) % 2 == 0)
              *(start_uv + l) = u;
            else
              *(start_uv + l) = v;
          }
        }
      }
    }
  }

  return 0;
}

static int show_string(const char* str,
                       int size,
                       int y_pos,
                       int width,
                       int height,
                       void* dstbuf) {
  int i = 0;
  for (i = 0; i < size; i++) {
    if ((*(str + i) >= 'A' && *(str + i) <= 'Z') ||
        (*(str + i) >= 'a' && *(str + i) <= 'z')) {
      if (show_one_alpha(width, height, dstbuf, *(str + i), FONTX * i + 50,
                         y_pos) < 0)
        return -1;
    } else {
      if (*(str + i) >= '0' && *(str + i) <= '9') {
        if (show_one_number(width, height, dstbuf, *(str + i) - '0',
                            FONTX * i + 50, y_pos) < 0)
          return -1;
      } else {
        if (show_one_number(width, height, dstbuf, 12, FONTX * i + 50, y_pos) <
            0)
          return -1;
      }
    }
  }
}

static int show_number(int width, int height, void* dstbuf) {
  int i;

  for (i = 0; i < 13; i++) {
    if (show_one_number(width, height, dstbuf, i, FONTX * i, height * 3 / 4) <
        0)
      return -1;
  }
  if (show_one_number(width, height, dstbuf, 0, FONTX * i, height * 3 / 4) < 0)
    return -1;

  return 0;
}

static int show_time(int width, int height, void* dstbuf) {
  time_t now;
  struct tm* timenow;
  int show[19];
  int i;

  time(&now);
  timenow = localtime(&now);

  show[0] = (timenow->tm_year + 1900) / 1000;
  show[1] = (timenow->tm_year + 1900) % 1000 / 100;
  show[2] = (timenow->tm_year + 1900) % 100 / 10;
  show[3] = (timenow->tm_year + 1900) % 10;
  show[4] = 10;
  show[5] = (timenow->tm_mon + 1) / 10;
  show[6] = (timenow->tm_mon + 1) % 10;
  show[7] = 10;
  show[8] = timenow->tm_mday / 10;
  show[9] = timenow->tm_mday % 10;
  show[10] = 12;
  show[11] = timenow->tm_hour / 10;
  show[12] = timenow->tm_hour % 10;
  show[13] = 11;
  show[14] = timenow->tm_min / 10;
  show[15] = timenow->tm_min % 10;
  show[16] = 11;
  show[17] = timenow->tm_sec / 10;
  show[18] = timenow->tm_sec % 10;

  for (i = 0; i < sizeof(show) / sizeof(show[0]); i++) {
    if (show_one_number(width, height, dstbuf, show[i], FONTX * i + 50,
                        height * 3 / 4) < 0)
      return -1;
  }

  return 0;
}

static int video_record_read_clk_core(void) {
  FILE* fp = NULL;
  int clk = 0;
  char str[100] = {0};

  fp = fopen("/sys/kernel/debug/clk/clk_summary", "r");
  if (fp) {
    while (!feof(fp)) {
      fscanf(fp, "%s", &str);
      if (!strncmp(str, "clk_core", strlen("clk_core")))
        break;
    }

    fscanf(fp, "%s", &str);
    fscanf(fp, "%s", &str);
    fscanf(fp, "%s", &str);
    clk = atoi(str);
    fclose(fp);
  } else
    printf("%s read fail.\n", __func__);

  return clk;
}

static void video_record_temp_fun(struct Video* video);
static int video_set_fps(struct Video* video, int numerator, int denominator);
static int show_video_mark(int width,
                           int height,
                           void* dstbuf,
                           int fps,
                           void* meta1,
                           uint32_t* noise,
                           struct dpp_sharpness* sharpness) {
#if TEST_VIDEO_MARK
  char name[30] = {0};
  struct HAL_Buffer_MetaData* meta = (struct HAL_Buffer_MetaData*)meta1;

  // if (show_time(width, height, dstbuf) < 0)
  //  return -1;

  /*if(with_adas)
      show_string("ldw on",strlen("ldw on"),150,width,height,dstbuf);
  else
      show_string("ldw off",strlen("ldw off"),150,width,height,dstbuf);*/
  if (with_mp)
    show_string("3dnr on", strlen("3dnr on"), 200, width, height, dstbuf);
  else
    show_string("3dnr off", strlen("3dnr off"), 200, width, height, dstbuf);

  snprintf(name, sizeof(name), "HAL %s", CAMHALVERSION);
  show_string(name, strlen(name), 250, width, height, dstbuf);

  memset(name, 0, sizeof(name));
  snprintf(name, sizeof(name), "fps %d", fps);
  show_string(name, strlen(name), 300, width, height, dstbuf);

  if (meta) {
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "%u %u %u %u %.0f %.0f",
             meta->enabled[HAL_ISP_WDR_ID], meta->enabled[HAL_ISP_GOC_ID],
             meta->wdr.wdr_gain_max_clip_enable, meta->wdr.wdr_gain_max_value,
             meta->exp_time * 1000, meta->exp_gain);
    show_string(name, strlen(name), 350, width, height, dstbuf);
  } else
    show_string("null", strlen("null"), 350, width, height, dstbuf);

  memset(name, 0, sizeof(name));
  snprintf(name, sizeof(name), "temp %d  clk %d", temperature, clk_core);
  show_string(name, strlen(name), 400, width, height, dstbuf);

  memset(name, 0, sizeof(name));
  snprintf(name, sizeof(name), "%u %u %u   %d  %u", noise[0], noise[1],
           noise[2], parameter_get_ex(), user_noise);
  show_string(name, strlen(name), 450, width, height, dstbuf);

  if (sharpness) {
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "%u %u", sharpness->src_shp_l,
             sharpness->src_shp_c);
    show_string(name, strlen(name), 500, width, height, dstbuf);
  }
#endif
  return 0;
}

static int convert_yuyv(int width,
                        int height,
                        void* srcbuf,
                        void* dstbuf,
                        int neadshownum) {
  int *dstint_y, *dstint_uv, *srcint;
  int y_size = width * height;
  int i, j;

  dstint_y = (int*)dstbuf;
  srcint = (int*)srcbuf;
  dstint_uv = (int*)((char*)dstbuf + y_size);

  for (i = 0; i < height; i++) {
    for (j = 0; j<width>> 2; j++) {
      if (i % 2 == 0) {
        *dstint_uv++ =
            (*(srcint + 1) & 0xff000000) | ((*(srcint + 1) & 0x0000ff00) << 8) |
            ((*srcint & 0xff000000) >> 16) | ((*srcint & 0x0000ff00) >> 8);
      }
      *dstint_y++ = ((*(srcint + 1) & 0x00ff0000) << 8) |
                    ((*(srcint + 1) & 0x000000ff) << 16) |
                    ((*srcint & 0x00ff0000) >> 8) | (*srcint & 0x000000ff);
      srcint += 2;
    }
  }

  // if (neadshownum) {
  //  if (show_time(width, height, dstbuf) < 0)
  //    return -1;
  //}

  return 0;
}

static int yuyv_display_scal(struct Video* video,
                             struct Video* video_cur,
                             struct win* video_win,
                             int x,
                             int y) {
  int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;

  while (video_cur) {
    if (video_cur->usb_type != USB_TYPE_H264) {
      src_w = video_cur->disp.yuyv.width;
      src_h = video_cur->disp.yuyv.height;
      src_fd = video_cur->disp.yuyv.fd;
      dst_w = video_win->video_ion.width;
      dst_h = video_win->video_ion.height;
      dst_fd = video_win->video_ion.fd;

      if (rk_rga_ionfdnv12_to_ionfdnv12_scal(video->disp.rga_fd,
                                             src_fd, src_w, src_h, RGA_FORMAT_YCBCR_420_SP,
                                             dst_fd, dst_w, dst_h, RGA_FORMAT_YCBCR_420_SP,
                                             x, y, src_w, src_h, src_w, src_h)) {
        printf("rk_rga_ionfdnv12_to_ionfdnv12_scal fail!\n");
        return -1;
      }
      break;
    }

    video_cur = video_cur->next;
  }

  return video_cur ? 0 : -1;
}

static bool yuyv_display_check(struct Video* video) {
  struct timeval t0, t1;
  gettimeofday(&t1, NULL);
  t0 = video->disp.t0;

  if (t0.tv_sec == 0) {
    video->disp.t0 = t1;
    return true;
  }

  // every 45ms display once, if fps is 30, every two buffer display once
  if ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec > 45000) {
    video->disp.t0 = t1;
    return true;
  } else {
    return false;
  }
}

void yuyv_display_pos(int* x, int* y, int *cnt, int rotate_angle,
                      int buffer_width, int buffer_height, int dst_w, int dst_h) {
  switch (rotate_angle) {
  case 90:
    *x = 0;
    *y = buffer_height - ((*cnt) + 1) * dst_h;
    break;
  case 180:
    *x = (*cnt) * dst_w;
    *y = dst_h;
    break;
  case 270:
    *x = buffer_width - dst_w;
    *y = (*cnt) * dst_h;
    break;
  default:
    *x = buffer_width - dst_w;
    *y = buffer_height - (*cnt) * dst_h;
    break;
  }
  *cnt += 1;
}

static int yuyv_display(struct Video* video, int width, int height, int fd) {
  struct win* video_win;
  int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;
  int x, y;
  struct Video* video_cur = NULL;
  int cnt = 0;
  int ret;
  int screen_w, screen_h;
  int out_device;
  int rotate_angle = 0;
  int buffer_width = 0, buffer_height = 0;

  // if (!yuyv_display_check(video))
  //  return 0;

  video->disp.displaying = true;
  if (!video->pthread_run)
    goto exit_display;

  video_win = rk_fb_getvideowin();
  buffer_width = video_win->video_ion.width;
  buffer_height = video_win->video_ion.height;
  dst_w = ALIGN(buffer_width / 5, 16);
  dst_h = ALIGN(buffer_height / 5, 16);

  if (video->disp.display) {
    video_ion_free(&video->disp.yuyv);
    out_device = rk_fb_get_out_device(&screen_w, &screen_h);
    rotate_angle = (out_device == OUT_DEVICE_HDMI ? 0 : VIDEO_DISPLAY_ROTATE_ANGLE);
    ret = rk_rga_ionfdnv12_to_ionfdnv12_rotate(
        video->disp.rga_fd, fd, width, height, width, height,
        video_win->video_ion.fd, video_win->video_ion.width,
        video_win->video_ion.height, rotate_angle);

    if (0 == ret) {
      // TODO: will not use notelock in the future

      pthread_rwlock_rdlock(&notelock);
      video_cur = video->pre;
      while (video_cur && video_cur->disp.yuyv.buffer) {
        yuyv_display_pos(&x, &y, &cnt, rotate_angle, buffer_width, buffer_height, dst_w, dst_h);
        if (cnt > 3 || x < 0 || y < 0)
          break;
        yuyv_display_scal(video, video_cur, video_win, x, y);
        video_cur = video_cur->pre;
      }
      video_cur = video->next;
      while (video_cur && video_cur->disp.yuyv.buffer) {
        yuyv_display_pos(&x, &y, &cnt, rotate_angle, buffer_width, buffer_height, dst_w, dst_h);
        if (cnt > 3 || x < 0 || y < 0)
          break;
        yuyv_display_scal(video, video_cur, video_win, x, y);
        video_cur = video_cur->next;
      }

      pthread_rwlock_unlock(&notelock);
      if (rk_fb_video_disp(video_win) == -1) {
        printf("rk_fb_video_disp err\n");
        // goto exit_display;
      }
    }
  } else {
    src_w = width;
    src_h = height;
    src_fd = fd;
    if (video->disp.yuyv.width != dst_w || video->disp.yuyv.height != dst_h) {
      video_ion_free(&video->disp.yuyv);
      if (video_ion_alloc(&video->disp.yuyv, dst_w, dst_h)) {
        printf("%s video ion alloc fail!\n", __func__);
        goto exit_display;
      }
    }
    dst_fd = video->disp.yuyv.fd;
    out_device = rk_fb_get_out_device(&screen_w, &screen_h);
    rotate_angle = (out_device == OUT_DEVICE_HDMI ? 0 : VIDEO_DISPLAY_ROTATE_ANGLE);
    rk_rga_ionfdnv12_to_ionfdnv12_rotate(video->disp.rga_fd, src_fd, src_w,
                                         src_h, src_w, src_h, dst_fd, dst_w,
                                         dst_h, rotate_angle);
  }

exit_display:
  video->disp.displaying = false;

  return 0;
}

static int video_query_businfo(struct Video* video) {
  if (video->hal->dev->queryBusInfo(video->businfo))
    return -1;

  return 0;
}

void video_record_getfilename(char* str,
                              unsigned short size,
                              const char* path,
                              int ch,
                              const char* filetype) {
  time_t now;
  struct tm* timenow;
  unsigned int year, mon, day, hour, min, sec;

  time(&now);
  timenow = localtime(&now);

  year = timenow->tm_year + 1900;
  mon = timenow->tm_mon + 1;
  day = timenow->tm_mday;
  hour = timenow->tm_hour;
  min = timenow->tm_min;
  sec = timenow->tm_sec;
  snprintf(str, size, "%s/%04d%02d%02d_%02d%02d%02d_%c.%s", path, year, mon,
           day, hour, min, sec, 'A' + ch, filetype);
}

static int video_record_getaudiocard(struct Video* video) {
  int card = -1;
  DIR* dp;
  struct dirent* dirp;
  char path[128];
  char* temp1;
  char* temp2;
  int sid;
  int id;
  char devicename[32] = {0};
  int len;

  temp1 = strchr((char*)(video->businfo), '-') + 1;
  temp2 = strchr(temp1, '-');
  if (!temp1 || !temp2)
    return -1;
  len = (int)temp2 - (int)temp1;
  sscanf(temp2 + 1, "%d", &sid);
  strncpy(devicename, temp1, len);
  sscanf(&devicename[len - 1], "%d", &id);
  id += 1;

  snprintf(path, sizeof(path), "/sys/devices/%s/usb%d/%d-%d/%d-%d:%d.2/sound/",
           devicename, id, id, sid, id, sid, sid);
  printf("dir = %s\n", path);
  if ((dp = opendir(path)) != NULL) {
    while ((dirp = readdir(dp)) != NULL) {
      char* pcard;
      if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
        continue;

      pcard = strstr(dirp->d_name, "card");
      if (pcard) {
        sscanf(&dirp->d_name[4], "%d", &card);
      }
    }
    closedir(dp);
  }

  return card;
}

static void send_record_time(EncodeHandler* handler, int sec) {
  static int last = -1;

  if (last == sec)
    return;
  else
    last = sec;

  if (rec_event_call)
    (*rec_event_call)(CMD_UPDATETIME, (void *)sec, (void *)0);
}

static int video_encode_init(struct Video* video) {
  int audio_id = -1;
  int fps = 30;
  if (video->type != VIDEO_TYPE_ISP) {
    audio_id = video_record_getaudiocard(video);
    fps = video->frmFmt.fps;
  } else if (video->type == VIDEO_TYPE_ISP) {
    audio_id = 0;
    fps = video->fps_d;
  }

  EncodeHandler* handler = EncodeHandler::create(
      video->deviceid, video->usb_type, video->width,
      video->height, fps, audio_id);
  if (handler) {
    handler->set_global_attr(&global_attr);
    handler->set_audio_mute(enableaudio ? false : true);
    pthread_rwlock_rdlock(&notelock);
    if (video->type == VIDEO_TYPE_ISP) {
      handler->record_time_notify = send_record_time;
    }
    pthread_rwlock_unlock(&notelock);
    video->encode_handler = handler;
    return 0;
  }
  return -1;
}

static int h264_encode_process(struct Video* video,
                               void* virt,
                               int fd,
                               void* hnd,
                               size_t size,
                               struct timeval& time_val) {
  int ret = -1;
  video->mp4_encoding = true;
  if (!video->pthread_run)
    goto exit_h264_encode_process;

  if (video->encode_handler) {
    ret = video->encode_handler->h264_encode_process(virt, fd, hnd, size,
                                                     time_val);
  }

exit_h264_encode_process:
  video->mp4_encoding = false;
  return ret;
}

static void video_encode_exit(struct Video* video) {
  PRINTF_FUNC;
  video->save_en = 0;

  if (video->encode_handler) {
#ifdef USE_WATERMARK
    video->encode_handler->watermark_info = NULL;
#endif

    delete video->encode_handler;
    video->encode_handler = NULL;
  }
  PRINTF_FUNC_OUT;
}

static int video_save_jpeg(struct Video* video,
                           char* filename,
                           void* srcbuf,
                           unsigned int size) {
  int size_start = 0;
  unsigned char* in_buf = (unsigned char*)srcbuf;
  unsigned int buf_size = size;
  while (*(in_buf + buf_size - 2) != 0xFF || *(in_buf + buf_size - 1) != 0xD9)
    buf_size--;

  FILE* fp = fopen(filename, "wb");
  if (!fp) {
    printf("%s: open jpeg file fail\n", __func__);
    return -1;
  }

  if (has_huffman(in_buf, buf_size)) {
    fwrite(in_buf, 1, buf_size, fp);
  } else {
    int dht_size = sizeof(dht_data);
    unsigned char* pdeb = in_buf;
    unsigned char* pcur = in_buf;
    unsigned char* plimit = in_buf + buf_size;
    /* find the SOF0(Start Of Frame 0) of JPEG */
    while ((((pcur[0] << 8) | pcur[1]) != 0xffc0) && (pcur < plimit)) {
      pcur++;
    }
    /* SOF0 of JPEG exist */
    if (pcur < plimit) {
      /* insert huffman table after SOF0 */
      size_start = pcur - pdeb;
      fwrite(in_buf, 1, size_start, fp);
      fwrite(dht_data, 1, dht_size, fp);
      fwrite(pcur, 1, buf_size - size_start, fp);
    }
  }

  fclose(fp);
  printf("video save jpeg frame:%s\n", filename);

  return 0;
}

static void video_record_takephoto_end(struct Video* video) {
  struct Video* video_cur;
  bool end = false;

  pthread_rwlock_rdlock(&notelock);
  video->photo.state = PHOTO_END;

  video_cur = getfastvideo();
  while (video_cur) {
    if (video_cur->photo.state != PHOTO_END)
      break;
    video_cur = video_cur->next;
  }
  if (!video_cur)
    end = true;
  pthread_rwlock_unlock(&notelock);

  // send message for photoend
  if (end && rec_event_call)
    (*rec_event_call)(CMD_PHOTOEND, (void *)0, (void *)1);
}

static int video_record_save_jpeg(struct Video* video,
                                  void* srcbuf,
                                  unsigned int size) {
  char filename[128];

  video_record_getfilename(filename, sizeof(filename), VIDEOFILE_PATH,
                           video->deviceid, "jpg");
  video_save_jpeg(video, filename, srcbuf, size);

  video_record_takephoto_end(video);

  return 0;
}

static void video_print_name(const char* name, bool* print) {
  if (*print) {
    printf("%s\n", name);
    *print = false;
  }
}

class MJPG_Photo : public NewCameraBufferReadyNotifier {
  struct Video* video;

 public:
  MJPG_Photo(struct Video* p) { video = p; }
  ~MJPG_Photo() {}

  virtual bool bufferReady(weak_ptr<CameraBuffer> buffer, int status) {
    UNUSED_PARAM(status);
    shared_ptr<CameraBuffer> spCamBuf = buffer.lock();
    static bool print = true;
    video_print_name("MJPG_Photo", &print);
    if (video->pthread_run && video->photo.state == PHOTO_ENABLE && spCamBuf.get()) {
      spCamBuf->incUsedCnt();

      video->photo.state = PHOTO_BEGIN;
      video_record_save_jpeg(video, spCamBuf->getVirtAddr(), spCamBuf->getDataSize());

      spCamBuf->decUsedCnt();
    }
    return true;
  }
};

static int vpu_nv12_encode_mjpg(struct Video* video,
                                void* srcbuf,
                                int src_fd,
                                size_t src_size) {
  int ret = 0;
  int fd = -1;
  char filename[128];

  video_record_getfilename(filename, sizeof(filename), VIDEOFILE_PATH,
                           video->deviceid, "jpg");
  fd = open((char*)filename, O_WRONLY | O_CREAT, 0666);
  if (fd < 0) {
    printf("Cannot open jpg file\n");
    return -1;
  }

  ret = vpu_nv12_encode_jpeg_doing(&video->photo.encode, srcbuf, src_fd, src_size, fd);
  if (ret) {
    goto exit;
  }

  printf("%s:%s\n", __func__, filename);

  if (fd >= 0) {
    fsync(fd);
    close(fd);
    fd = -1;
  }
  fs_manage_insert_file(filename);

  return 0;

exit:

  if (fd >= 0) {
    fsync(fd);
    close(fd);
    fd = -1;
  }

  return ret;
}

/***************************************************************************************************
static int nv12_encode_mjpg(struct Video *video,void *srcbuf)
{
    FILE *fp;
    struct jpeg_compress_struct compress;
    struct jpeg_error_mgr err;
    JSAMPROW row_pointer[1];
    int row_stride;
    int i = 0,j = 0;
    unsigned char buf[video->width * 3];
    unsigned char *py,*puv;
    char filename[128];

    video_record_getfilename(filename, sizeof(filename), VIDEOFILE_PATH,
video->deviceid, "jpg");

    compress.err = jpeg_std_error(&err);
    jpeg_create_compress(&compress);

    fp = fopen((char *)filename,"wb");
    if (!fp) {
        printf("Cannot open jpg file\n");
        jpeg_destroy_compress(&compress);
        return -1;
    }

    jpeg_stdio_dest(&compress,fp);
    compress.image_width = video->width;
    compress.image_height = video->height;
    compress.input_components = 3;
    compress.in_color_space = JCS_YCbCr;
    compress.dct_method = JDCT_ISLOW;
    jpeg_set_defaults(&compress);

    jpeg_set_quality(&compress,99,TRUE);

    jpeg_start_compress(&compress,TRUE);
    row_stride = compress.image_width * 3;

    py = (unsigned char*)srcbuf;
    puv = (unsigned char *)srcbuf + video->width * video->height;

    j = 1;
    while(compress.next_scanline < compress.image_height) {
        if (j % 2 == 0)
            puv -= compress.image_width;
        for(i = 0;i < compress.image_width;i += 2) {
            buf[i*3 + 0] = *py++;
            buf[i*3 + 1] = *puv++;
            buf[i*3 + 2] = *puv++;
            puv -= 2;
            buf[i*3 + 3] = *py++;
            buf[i*3 + 4] = *puv++;
            buf[i*3 + 5] = *puv++;
        }
        row_pointer[0] = buf;
        (void)jpeg_write_scanlines(&compress,row_pointer,1);
        j++;
    }

    jpeg_finish_compress(&compress);
    jpeg_destroy_compress(&compress);
    if (fp) {
        fclose(fp);
        fp = NULL;
    }

    printf("%s:%s\n",__func__,filename);

    return 0;
}
***************************************************************************************************/

class SP_Display : public NewCameraBufferReadyNotifier {
  struct Video* video;

 public:
  SP_Display(struct Video* p) { video = p; }
  ~SP_Display() {}
  virtual bool bufferReady(weak_ptr<CameraBuffer> buffer, int status) {
    static bool print = true;
    video_print_name("SP_Display", &print);
    bool ret = true;
    static unsigned char cnt = 0;
    UNUSED_PARAM(status);

    // static struct timeval t0;
    // static int i = 0;
    // i++;
    // fps_count(&t0,&i,"SP_Display");
    if (!with_mp) {
      video->fps_total++;

      cnt++;
      video_record_temp_fun(video);
#if TEST_VIDEO_MARK
      if (cnt % 60 == 0)
        clk_core = video_record_read_clk_core();
#endif
    }

    if (with_sp) {
      shared_ptr<CameraBuffer> spCamBuf = buffer.lock();
      if (video->pthread_run && spCamBuf.get()) {
        spCamBuf->incUsedCnt();

        if (yuyv_display(video, spCamBuf->getWidth(), spCamBuf->getHeight(),
                         (int)(spCamBuf->getFd()))) {
          video_record_signal(video);
          ret = false;
        }

        spCamBuf->decUsedCnt();
      }
    }

    return ret;
  }
};

static int mjpg_decode_init(struct Video* video) {
  if (video_ion_alloc_rational(&video->jpeg_dec.out, video->width, video->height, 2, 1)) {
    printf("%s alloc jpeg_dec.out err\n", __func__);
    return -1;
  }

  video->jpeg_dec.decode = (struct vpu_decode*)calloc(1, sizeof(struct vpu_decode));
  if (!video->jpeg_dec.decode) {
    printf("no memory!\n");
    return -1;
  }
  if (vpu_decode_jpeg_init(video->jpeg_dec.decode, video->width, video->height)) {
    printf("%s vpu_decode_jpeg_init fail\n", __func__);
    return -1;
  }

  if ((video->jpeg_dec.rga_fd = rk_rga_open()) <= 0)
    return -1;

  return 0;
}

static void mjpg_decode_exit(struct Video* video) {
  if (video->jpeg_dec.decode) {
    vpu_decode_jpeg_done(video->jpeg_dec.decode);
    free(video->jpeg_dec.decode);
    video->jpeg_dec.decode = NULL;
  }

  video_ion_free(&video->jpeg_dec.out);

  rk_rga_close(video->jpeg_dec.rga_fd);
}

static int mjpg_decode(struct Video* video,
                       void* srcbuf,
                       unsigned int size,
                       void* dstbuf,
                       int dstfd) {
  char* in_data = (char*)srcbuf;
  unsigned int in_size = size;
  int src_fd, src_w, src_h, dst_fd, dst_w, dst_h, vir_w, vir_h;
  int i = 0;
  int ret = 0;

  video->jpeg_dec.decoding = true;
  if (!video->pthread_run)
    goto exit_mjpg_decode;

  while (*(in_data + in_size - 2) != 0xFF || *(in_data + in_size - 1) != 0xD9) {
    in_size--;
    if (i++ > 16) {
      printf("err mjpg data.\n");
      ret = 0;
      goto exit_mjpg_decode;
    }
  }

  if (video->jpeg_dec.out.buffer) {
    if (vpu_decode_jpeg_doing(video->jpeg_dec.decode, in_data, in_size,
                              video->jpeg_dec.out.fd,
                              (char*)video->jpeg_dec.out.buffer)) {
      printf("vpu_decode_jpeg_doing err\n");
      ret = -1;
      goto exit_mjpg_decode;
    }

    src_fd = video->jpeg_dec.out.fd;
    src_w = video->jpeg_dec.out.width;
    src_h = video->jpeg_dec.out.height;
    dst_fd = dstfd;
    dst_w = video->width;
    dst_h = video->height;
    vir_w = (src_w + 15) & (~15);
    vir_h = (src_h + 15) & (~15);
    if (MPP_FMT_YUV420SP == video->jpeg_dec.decode->fmt) {
      rk_rga_ionfdnv12_to_ionfdnv12_scal(video->jpeg_dec.rga_fd,
                                         src_fd, src_w, src_h, RGA_FORMAT_YCBCR_420_SP,
                                         dst_fd, dst_w, dst_h, RGA_FORMAT_YCBCR_420_SP,
                                         0, 0, dst_w, dst_h, vir_w, vir_h);
    } else if (MPP_FMT_YUV422SP == video->jpeg_dec.decode->fmt) {
      rk_rga_ionfdnv12_to_ionfdnv12_scal(video->jpeg_dec.rga_fd,
                                         src_fd, src_w, src_h, RGA_FORMAT_YCBCR_422_SP,
                                         dst_fd, dst_w, dst_h, RGA_FORMAT_YCBCR_420_SP,
                                         0, 0, dst_w, dst_h, vir_w, vir_h);
    } else {
      ret = -1;
      goto exit_mjpg_decode;
    }
  }

exit_mjpg_decode:
  video->jpeg_dec.decoding = false;
  return ret;
}

class MJPG_NV12 : public StreamPUBase {
  struct Video* video;

 public:
  MJPG_NV12(struct Video* p) : StreamPUBase("MJPG_NV12", true, false) {
    video = p;
  }
  ~MJPG_NV12() {}

  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static bool print = true;
    video_print_name("MJPG_NV12", &print);
    if (video->pthread_run && inBuf.get() && outBuf.get()) {
      if (mjpg_decode(video, inBuf->getVirtAddr(), inBuf->getDataSize(),
                      outBuf->getVirtAddr(), (int)(outBuf->getFd()))) {
        video_record_signal(video);
        return false;
      }
      outBuf->setDataSize(video->width * video->height * 3 / 2);
    }
    return true;
  }
};

static void* video_adas_pthread(void* arg) {
  struct Video* video = (struct Video*)arg;
  DPP_RET ret = DPP_OK;
  DppFrame frame_out = 0;
  DPP_FRAME_TYPE frame_type = DPP_FRAME_COPY;
  dsp_ldw_out* adas_out = NULL;
  int cnt = 0;

  do {
    ret = dpp_get_frame(video->adas->context, (DppFrame*)&frame_out);
    if (!frame_out) {
      if (ret == DPP_ERR_TIMEOUT) {
        printf("ADAS: Get frame failed cause by timeout.\n");
        continue;
      } else {
        printf("Get frame failed, DPP request exit.\n");
        break;
      }
    }

    frame_type = dpp_frame_get_type(frame_out);
    if (frame_type == DPP_FRAME_ADAS) {
      adas_out = dpp_frame_get_adas_out(frame_out);
      if (cnt++ == ADAS_FRAME_COUNT) {
        cnt = 0;
        // printf("%d,%d\n%d,%d\n%d,%d\n%d,%d\nflag=%d\n",
        //       adas_out->endPoints[0][0],adas_out->endPoints[0][1],
        //        adas_out->endPoints[1][0],adas_out->endPoints[1][1],
        //        adas_out->endPoints[2][0],adas_out->endPoints[2][1],
        //        adas_out->endPoints[3][0],adas_out->endPoints[3][1],
        //        adas_out->turnflg);
        memcpy(&video->adas->out, adas_out, sizeof(dsp_ldw_out));
        if (rec_event_call)
          (*rec_event_call)(CMD_ADAS, (void *)&video->adas->out, (void *)0);
      }
    }
    dpp_frame_deinit((DppFrame*)frame_out);
    frame_out = NULL;
  } while (video->pthread_run);

  if (frame_out) {
    ret = dpp_frame_deinit((DppFrame*)frame_out);
    if (DPP_OK != ret) {
      printf("dpp_frame_deinit failed.\n");
    }
  }
  video_record_signal(video);

  pthread_exit(NULL);
}

static int video_adas_init(struct Video* video) {
  int i = 0;
  DPP_RET ret = DPP_OK;

  video->adas = new video_adas();
  if (!video->adas) {
    printf("new video_adas() failed!\n");
    return -1;
  }
  video->adas->context = 0;
  video->adas->dpp_id = 0;

  pthread_mutex_init(&video->adas->pool_lock, NULL);

  if ((video->adas->rga_fd = rk_rga_open()) < 0)
    return -1;

  ret = dpp_create((DppCtx*)&video->adas->context, DPP_FUN_ADAS);
  if (DPP_OK != ret) {
    printf("dpp_create adas fail!\n");
    return -1;
  }

  dpp_start(video->adas->context);

  for (i = 0; i < ADAS_BUFFER_NUM; i++) {
    memset (&video->adas->input[i], 0, sizeof(struct video_ion));
    video->adas->input[i].client = -1;
    video->adas->input[i].fd = -1;
    if (video_ion_alloc(&video->adas->input[i], ADAS_BUFFER_WIDTH,
                        ADAS_BUFFER_HEIGHT)) {
      printf("video_adas_init ion alloc fail!\n");
      return -1;
    }
    video->adas->pool.push_back(&video->adas->input[i]);
  }

  if (pthread_create(&video->adas->dpp_id, &global_attr, video_adas_pthread,
                     video)) {
    printf("%s pthread create err!\n", __func__);
    return -1;
  }

  return 0;
}

static void video_adas_exit(struct Video* video) {
  int i = 0;

  printf("%s enter\n", __func__);
  if (video->adas) {
    if (video->adas->context) {
      dpp_stop(video->adas->context);
      printf("dpp_stop!\n");
    }

    if (video->adas->dpp_id) {
      printf("pthread_join dpp_id enter\n");
      pthread_join(video->adas->dpp_id, NULL);
      printf("pthread_join dpp_id exit\n");
      video->adas->dpp_id = 0;
    }

    if (video->adas->context) {
      dpp_destroy(video->adas->context);
      video->adas->context = 0;
    }

    while (video->adas->pool.size())
      video->adas->pool.pop_back();

    for (i = 0; i < ADAS_BUFFER_NUM; i++)
      video_ion_free(&video->adas->input[i]);

    rk_rga_close(video->adas->rga_fd);

    pthread_mutex_destroy(&video->adas->pool_lock);

    delete video->adas;
    video->adas = NULL;
  }
  printf("%s exit\n", __func__);
}

static void video_adas_packet_release(void* packet) {
  int i = 0;
  DppBufferInfo* buf_info = NULL;
  struct video_adas* adas = NULL;
  buf_info = (DppBufferInfo*)dpp_packet_get_buf_info(packet);
  adas = (struct video_adas*)dpp_packet_get_data(packet);

  if (!buf_info || !adas) {
    printf("video_adas_packet_release failed!\n");
    return;
  }

  for (i = 0; i < ADAS_BUFFER_NUM; i++) {
    if (buf_info->ptr == adas->input[i].buffer) {
      pthread_mutex_lock(&adas->pool_lock);
      adas->pool.push_back(&adas->input[i]);
      pthread_mutex_unlock(&adas->pool_lock);
    }
  }

  if (buf_info) {
    delete buf_info;
    buf_info = NULL;
  }
}

static int video_adas_packet_process(struct Video* video,
                                     struct video_ion* in) {
  DppBufferInfo* buf_info;
  DPP_RET ret = DPP_OK;
  DppPacket packet_in = 0;
  int rt = 0;

  buf_info = new DppBufferInfo();
  if (!buf_info)
    return -1;

  buf_info->type = DPP_BUFFER_TYPE_ION;
  buf_info->size = in->width * in->height * 3 / 2;
  buf_info->ptr = in->buffer;
  buf_info->hnd = (void*)in->handle;
  buf_info->fd = in->fd;
  buf_info->phys = (unsigned int)(in->phys);

  ret = dpp_packet_init((DppPacket*)&packet_in, in->width, in->height);
  if (DPP_OK != ret) {
    printf("dpp_packet_init_with_buf_info failed.\n");
    rt = -1;
    goto adas_packet_exit;
  }
  ret = dpp_packet_set_cb(packet_in, video_adas_packet_release);
  if (DPP_OK != ret) {
    printf("dpp_packet_set_cb failed.\n");
    rt = -1;
    goto adas_packet_exit;
  }
  dpp_packet_set_buf_info(packet_in, buf_info);
  dpp_packet_set_ldw_type(packet_in, LDW_TYPE_DAY);  // 0 night, 1 daytime
  dpp_packet_set_data(packet_in, video->adas);
  ret = dpp_put_packet(video->adas->context, packet_in);
  if (DPP_OK != ret) {
    printf("put_packet failed.\n");
    rt = -1;
    goto adas_packet_exit;
  }

adas_packet_exit:

  if (packet_in) {
    ret = dpp_packet_deinit((DppPacket*)packet_in);
    if (DPP_OK != ret) {
      printf("dpp_packet_deinit failed.\n");
      rt = -1;
    }
  }

  return rt;
}

class NV12_ADAS : public StreamPUBase {
  struct Video* video;

 public:
  NV12_ADAS(struct Video* p) : StreamPUBase("NV12_ADAS", true, true) {
    video = p;
  }
  ~NV12_ADAS() {}
  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static int i = 0;
    struct video_ion* out = NULL;
    int src_fd, src_w, src_h, dst_fd, dst_w, dst_h;
    list<struct video_ion*>::iterator iterator;
    static bool print = true;
    video_print_name("NV12_ADAS", &print);

    i++;
    if (i % 2 == 0 && video->pthread_run && inBuf.get() && !video->high_temp) {
      pthread_mutex_lock(&video->adas->pool_lock);
      if (video->adas->pool.size()) {
        iterator = video->adas->pool.begin();
        out = *iterator;
        video->adas->pool.pop_front();
      }
      pthread_mutex_unlock(&video->adas->pool_lock);
      if (out) {
        src_fd = (int)(inBuf->getFd());
        src_w = inBuf->getWidth();
        src_h = inBuf->getHeight();
        dst_fd = out->fd;
        dst_w = out->width;
        dst_h = out->height;
        rk_rga_ionfdnv12_to_ionfdnv12_scal(video->adas->rga_fd,
                                           src_fd, src_w, src_h, RGA_FORMAT_YCBCR_420_SP,
                                           dst_fd, dst_w, dst_h, RGA_FORMAT_YCBCR_420_SP,
                                           0, 0, dst_w, dst_h, src_w, src_h);

        if (video_adas_packet_process(video, out))
          return false;
      }
    }

    return true;
  }
};
#ifdef USE_WATERMARK
static void video_photo_insert_watermark(void* dstbuf,
                                         Uint32 resolution_width,
                                         Uint32 resolution_height,
                                         Uint8* srcbuf,
                                         Uint32 src_width,
                                         Uint32 src_height,
                                         Uint32 x_pos,
                                         Uint32 y_pos) {
  Uint32 i, j;
  Uint8 index;
  Uint8 *y_addr = NULL, *uv_addr = NULL;
  Uint8 *start_y = NULL, *start_uv = NULL;

  if ((x_pos + src_width) >= resolution_width || y_pos < 0 ||
      (y_pos + src_height) >= resolution_height) {
    printf("%s error input number or position.\n", __func__);
    return;
  }

  y_addr = (unsigned char*)dstbuf + y_pos * resolution_width + x_pos;
  uv_addr = (unsigned char*)dstbuf + resolution_width * resolution_height +
            y_pos * resolution_width / 2 + x_pos;

  for (j = 0; j < src_height; j++) {
    start_y = y_addr + j * resolution_width;
    start_uv = uv_addr + j * resolution_width / 2;

    for (i = 0; i < src_width; i++) {
      index = srcbuf[i + j * src_width];
      if (((YUV444_PALETTE_TABLE[index] & 0xff000000) >> 24) == 0xff) {
        *start_y = YUV444_PALETTE_TABLE[index] & 0x000000ff;

        if ((j % 2 == 0) && (i % 2 == 0)) {
          *start_uv = (YUV444_PALETTE_TABLE[index] & 0x0000ff00) >> 8;
          *(start_uv + 1) = (YUV444_PALETTE_TABLE[index] & 0x00ff0000) >> 16;
        }
      }

      start_y++;
      if ((j % 2 == 0) && (i % 2 == 0))
        start_uv += 2;
    }
  }
}

static int video_photo_watermark(struct Video* video) {
  if (!parameter_get_video_mark())
    return 0;

  if (video->watermark_info.watermark_data.buffer) {
    // show time
    if (video->watermark_info.type & WATERMARK_TIME) {
      video_photo_insert_watermark(
          video->photo.rga_photo.buffer, video->width, video->height,
          (Uint8*)video->watermark_info.watermark_data.buffer +
              video->watermark_info.osd_data_offset.time_data_offset,
          video->watermark_info.coord_info.time_bmp.width,
          video->watermark_info.coord_info.time_bmp.height,
          video->watermark_info.coord_info.time_bmp.x,
          video->watermark_info.coord_info.time_bmp.y);
    }

    // show image
    if (video->watermark_info.type & WATERMARK_LOGO) {
      video_photo_insert_watermark(
          video->photo.rga_photo.buffer, video->width, video->height,
          (Uint8*)video->watermark_info.watermark_data.buffer +
              video->watermark_info.osd_data_offset.logo_data_offset,
          video->watermark_info.coord_info.logo_bmp.width,
          video->watermark_info.coord_info.logo_bmp.height,
          video->watermark_info.coord_info.logo_bmp.x,
          video->watermark_info.coord_info.logo_bmp.y);
    }
  }

  return 0;
}
#endif
static void* video_rga_photo_pthread(void* arg) {
  struct Video* video = (struct Video*)arg;

  while (video->pthread_run) {
    pthread_mutex_lock(&video->photo.mutex);
    if (video->photo.state != PHOTO_DISABLE)
      pthread_cond_wait(&video->photo.condition, &video->photo.mutex);
    pthread_mutex_unlock(&video->photo.mutex);

    if (video->photo.state == PHOTO_DISABLE) {
      printf("receive the signal from video_photo_exit()\n");
      break;
    }

    void* buffer = video->photo.rga_photo.buffer;
    int size = video->photo.rga_photo.width * video->photo.rga_photo.height * 3 / 2;
    int fd = video->photo.rga_photo.fd;

#ifdef USE_WATERMARK
    video_photo_watermark(video);
#endif

    vpu_nv12_encode_mjpg(video, buffer, fd, size);

    video_record_takephoto_end(video);
  }

  pthread_exit(NULL);
}

static int video_rga_photo_process(struct Video* video, int fd) {
  int ret = 0;
  int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;

  src_w = video->width;
  src_h = video->height;
  src_fd = fd;
  dst_w = video->photo.rga_photo.width;
  dst_h = video->photo.rga_photo.height;
  dst_fd = video->photo.rga_photo.fd;
  ret = rk_rga_ionfdnv12_to_ionfdnv12_scal(video->photo.rga_fd,
                                           src_fd, src_w, src_h, RGA_FORMAT_YCBCR_420_SP,
                                           dst_fd, dst_w, dst_h, RGA_FORMAT_YCBCR_420_SP,
                                           0, 0, src_w, src_h, src_w, src_h);

  if (ret) {
    printf("%s rga fail!\n", __func__);
    goto exit;
  }

  pthread_mutex_lock(&video->photo.mutex);
  pthread_cond_signal(&video->photo.condition);
  pthread_mutex_unlock(&video->photo.mutex);

  return 0;

exit:
  video_record_takephoto_end(video);

  return ret;
}

static int video_dpp_buffer_create(struct dpp_buffer** buffer) {
  struct dpp_buffer* buf = (struct dpp_buffer*)calloc(1, sizeof(*buf));
  if (!buf) {
    printf("Malloc memory for buffer failed.\n");
    (*buffer) = NULL;
    return -1;
  }

  (*buffer) = buf;
  return 0;
}

static int video_dpp_buffer_create_from_frame(struct dpp_buffer** buffer,
                                              DppFrame frame) {
  dpp_buffer* buf = NULL;

  video_dpp_buffer_create(&buf);
  if (buf) {
    buf->buffer = dpp_frame_get_buffer(frame);
    dpp_buffer_inc_ref(buf->buffer);
    buf->pts = dpp_frame_get_pts(frame);
    buf->isp_meta = (struct HAL_Buffer_MetaData*)dpp_frame_get_data(frame);
    dpp_frame_get_noise(frame, buf->noise);
    dpp_frame_get_sharpness(frame, &buf->sharpness);
    (*buffer) = buf;
    return 0;
  }

  return -1;
}

static void video_dpp_buffer_destroy(struct dpp_buffer* buffer) {
  if (buffer) {
    dpp_buffer_dec_ref(buffer->buffer);
    free(buffer);
  }
}

static int video_dpp_buffer_list_init(struct dpp_buffer_list* list) {
  // Cannot memset list which would case null std::list object
  pthread_mutex_init(&list->mutex, NULL);
  pthread_cond_init(&list->condition, NULL);
  return 0;
}

static int video_dpp_buffer_list_deinit(struct dpp_buffer_list* list) {
  pthread_mutex_lock(&list->mutex);
  while (list->buffers.size()) {
    printf("DPP buffer list is not null when deinit.\n");
    struct dpp_buffer* buffer = list->buffers.front();
    list->buffers.pop_front();
    video_dpp_buffer_destroy(buffer);
  }
  pthread_mutex_unlock(&list->mutex);
  pthread_mutex_destroy(&list->mutex);
  pthread_cond_destroy(&list->condition);
  return 0;
}

static int video_dpp_buffer_list_push(struct dpp_buffer_list* list,
                                      struct dpp_buffer* buffer) {
  pthread_mutex_lock(&list->mutex);
  list->buffers.push_back(buffer);
  pthread_cond_signal(&list->condition);
  pthread_mutex_unlock(&list->mutex);

  return 0;
}

static int video_dpp_buffer_list_pop(struct dpp_buffer_list* list,
                                     struct dpp_buffer** buffer,
                                     bool enable_timeout) {
  pthread_mutex_lock(&list->mutex);
  if (list->buffers.empty()) {
    if (enable_timeout) {
      struct timespec timeout;
      struct timeval now;
      gettimeofday(&now, NULL);
      timeout.tv_sec = now.tv_sec + 1;
      timeout.tv_nsec = now.tv_usec * 1000;
      pthread_cond_timedwait(&list->condition, &list->mutex, &timeout);
    } else {
      pthread_cond_wait(&list->condition, &list->mutex);
    }
  }

  if (!list->buffers.empty()) {
    (*buffer) = list->buffers.front();
    list->buffers.pop_front();
  } else {
    (*buffer) = NULL;
  }

  pthread_mutex_unlock(&list->mutex);
  return 0;
}

static void video_dpp_buffer_list_signal(struct dpp_buffer_list* list) {
  pthread_mutex_lock(&list->mutex);
  pthread_cond_signal(&list->condition);
  pthread_mutex_unlock(&list->mutex);
}

static void* video_encode_thread_func(void* arg) {
  struct Video* video = (struct Video*)arg;

  while (video->pthread_run && !video->dpp->stop_flag) {
    struct dpp_buffer* buffer = NULL;
    video_dpp_buffer_list_pop(&video->dpp->encode_buffer_list, &buffer, true);
    if (!buffer)
      continue;

    int fd = dpp_buffer_get_fd(buffer->buffer);
    size_t size = dpp_buffer_get_size(buffer->buffer);

    assert(fd > 0);

    if (video->save_en) {
      MppEncPrepCfg precfg;

      memset(&precfg, 0, sizeof(precfg));
      precfg.src_shp_en_y = buffer->sharpness.src_shp_l;
      precfg.src_shp_en_uv = buffer->sharpness.src_shp_c;
      precfg.src_shp_thr = buffer->sharpness.src_shp_thr;
      precfg.src_shp_div = buffer->sharpness.src_shp_div;
      precfg.src_shp_w0 = buffer->sharpness.src_shp_w0;
      precfg.src_shp_w1 = buffer->sharpness.src_shp_w1;
      precfg.src_shp_w2 = buffer->sharpness.src_shp_w2;
      precfg.src_shp_w3 = buffer->sharpness.src_shp_w3;
      precfg.src_shp_w4 = buffer->sharpness.src_shp_w4;
      if (video->encode_handler)
        video->encode_handler->h264_encode_control(MPP_ENC_SET_PREP_CFG,
                                                   (void*)&precfg);
      if (h264_encode_process(video, NULL, fd, NULL, size, buffer->pts))
        printf("Encode failed!\n");
    }

    video_dpp_buffer_destroy(buffer);
  }

  pthread_exit(NULL);
}

void* video_photo_thread_func(void* arg) {
  struct Video* video = (struct Video*)arg;

  while (video->pthread_run && !video->dpp->stop_flag) {
    struct dpp_buffer* buffer = NULL;
    video_dpp_buffer_list_pop(&video->dpp->photo_buffer_list, &buffer, false);
    if (!buffer)
      continue;

    int fd = dpp_buffer_get_fd(buffer->buffer);
    assert(fd > 0);
    video_rga_photo_process(video, fd);

    for (int i = 0; i < JPEG_STREAM_NUM; ++i) {
      JpegStreamReceiver* receiver = video->jpeg_receiver[i];
      if (receiver && receiver->get_request_encode()) {
        int fd = dpp_buffer_get_fd(buffer->buffer);
        int ret = receiver->process(fd, video->width, video->height,
                                    video->jpeg_config[i]);
        if (ret < 0) {
          receiver->set_request_encode(false);
          receiver->notify(NULL, 0, 0, 0);
        }
      }
    }

    video_dpp_buffer_destroy(buffer);
  }

  pthread_exit(NULL);
}

void* video_display_thread_func(void* arg) {
  struct Video* video = (struct Video*)arg;

  while (video->pthread_run && !video->dpp->stop_flag) {
    struct dpp_buffer* buffer = NULL;
    video_dpp_buffer_list_pop(&video->dpp->display_buffer_list, &buffer, true);
    if (!buffer)
      continue;

    int fd = dpp_buffer_get_fd(buffer->buffer);
    size_t size = dpp_buffer_get_size(buffer->buffer);

    assert(fd > 0);

#if TEST_VIDEO_MARK
    void* address = dpp_buffer_get_ptr(buffer->buffer);
    show_video_mark(video->width, video->height, address, video->fps,
                    buffer->isp_meta, buffer->noise, &buffer->sharpness);
#endif

#if HAVE_DISPLAY
    if (!with_sp) {
      if (yuyv_display(video, video->width, video->height, fd))
        printf("Display failed!\n");
    }
#endif
    video_dpp_buffer_destroy(buffer);
  }

  pthread_exit(NULL);
}

static void* video_dpp_thread_func(void* arg) {
  struct Video* video = (struct Video*)arg;

  // Create encode thread
  if (with_mp) {
    if (is_record_mode) {
      if (pthread_create(&video->dpp->encode_thread, &global_attr,
                         video_encode_thread_func, video)) {
        printf("Encode thread create failed!\n");
        pthread_exit(NULL);
      }
    }
  }

  // Create photo thread
  if (pthread_create(&video->dpp->photo_thread, &global_attr,
                     video_photo_thread_func, video)) {
    printf("Photo pthread create failed!\n");
    pthread_exit(NULL);
  }

  // Create display thread
  if (pthread_create(&video->dpp->display_thread, &global_attr,
                     video_display_thread_func, video)) {
    printf("Display pthread create failed!\n");
    pthread_exit(NULL);
  }

  // DPP thread main loop. Get a frame from dpp, then consturct buffers
  // pushed into next level threads for further processing.
  printf("DPP thread main loop start.\n");
  while (video->pthread_run && !video->high_temp) {
    DppFrame frame = NULL;
    DPP_RET ret = dpp_get_frame(video->dpp->context, (DppFrame*)&frame);
    if (!frame) {
      if (ret == DPP_ERR_TIMEOUT) {
        printf("Get frame from dpp failed cause by timeout.\n");
        continue;
      } else {
        printf("Get frame failed, DPP request exit.\n");
        break;
      }
    }

    struct dpp_buffer* encode_buffer = NULL;
    struct dpp_buffer* photo_buffer = NULL;
    struct dpp_buffer* display_buffer = NULL;

    if (video->dpp->encode_thread) {
      video_dpp_buffer_create_from_frame(&encode_buffer, frame);
      video_dpp_buffer_list_push(&video->dpp->encode_buffer_list,
                                 encode_buffer);
    }

    if (video->photo.state == PHOTO_ENABLE && video->dpp->photo_thread) {
      video->photo.state = PHOTO_BEGIN;
      video_dpp_buffer_create_from_frame(&photo_buffer, frame);
      video_dpp_buffer_list_push(&video->dpp->photo_buffer_list, photo_buffer);
    }
    if (video->dpp->display_thread) {
      video_dpp_buffer_create_from_frame(&display_buffer, frame);
      video_dpp_buffer_list_push(&video->dpp->display_buffer_list,
                                 display_buffer);
    }

    dpp_frame_deinit((DppFrame*)frame);
  }

  // Before exit dpp thread, we should wait photo/encode/display thread
  // have exited first. When stop_flag is set "true", these threads will
  // exit immediately.
  video->dpp->stop_flag = true;

  video_dpp_buffer_list_signal(&video->dpp->encode_buffer_list);
  if (video->dpp->encode_thread) {
    // Release video encode wait condition first.

    printf("Encode thread exit start.\n");
    pthread_join(video->dpp->encode_thread, NULL);
    printf("Encode thread exit end.\n");
    video->dpp->encode_thread = 0;
  }

  video_dpp_buffer_list_signal(&video->dpp->photo_buffer_list);
  if (video->dpp->photo_thread) {
    // Release video photo wait condition first.

    printf("Photo thread exit start.\n");
    pthread_join(video->dpp->photo_thread, NULL);
    printf("Photo thread exit end.\n");
    video->dpp->photo_thread = 0;
  }

  video_dpp_buffer_list_signal(&video->dpp->display_buffer_list);
  if (video->dpp->display_thread) {
    // Release video photo wait condition first.

    printf("Display thread exit start.\n");
    pthread_join(video->dpp->display_thread, NULL);
    printf("Display thread exit end.\n");
    video->dpp->display_thread = 0;
  }

  pthread_exit(NULL);
}

static int video_dpp_init(struct Video* video) {
  DPP_RET ret = DPP_OK;

  pthread_mutex_init(&pool_lock, NULL);

  video->dpp = new video_dpp();
  if (!video->dpp) {
    printf("new dpp() failed!\n");
    return -1;
  }
  video->dpp->context = 0;
  video->dpp->dpp_thread = 0;
  video->dpp->encode_thread = 0;
  video->dpp->photo_thread = 0;
  video->dpp->display_thread = 0;

  ret = dpp_create((DppCtx*)&video->dpp->context, DPP_FUN_3DNR);
  if (DPP_OK != ret) {
    printf(">> Test dpp_create failed.\n");
    return -1;
  }

  dpp_start(video->dpp->context);

  video_dpp_buffer_list_init(&video->dpp->encode_buffer_list);
  video_dpp_buffer_list_init(&video->dpp->photo_buffer_list);
  video_dpp_buffer_list_init(&video->dpp->display_buffer_list);

  if (pthread_create(&video->dpp->dpp_thread, &global_attr,
                     video_dpp_thread_func, video)) {
    printf("%s pthread create err!\n", __func__);
    return -1;
  }

  video->dpp_init = true;

  return 0;
}

static void video_dpp_exit(struct Video* video) {
  printf("%s enter\n", __func__);
  if (video->dpp) {
    if (video->dpp->context)
      dpp_stop(video->dpp->context);

    if (video->dpp->dpp_thread) {
      printf("DPP thread exit start.\n");
      pthread_join(video->dpp->dpp_thread, NULL);
      printf("DPP thread exit end.\n");
      video->dpp->dpp_thread = 0;
    }

    video_dpp_buffer_list_deinit(&video->dpp->encode_buffer_list);
    video_dpp_buffer_list_deinit(&video->dpp->photo_buffer_list);
    video_dpp_buffer_list_deinit(&video->dpp->display_buffer_list);

    if (video->dpp->context) {
      dpp_destroy(video->dpp->context);
      video->dpp->context = 0;
    }

    delete video->dpp;
    video->dpp = NULL;
  }

  video->dpp_init = false;

  pthread_mutex_destroy(&pool_lock);

  printf("%s exit\n", __func__);
}

static void video_dpp_packet_release(void* packet) {
  DppBufferInfo* buf_info = NULL;
  buf_info = (DppBufferInfo*)dpp_packet_get_buf_info(packet);

  if (!buf_info) {
    printf("video_dpp_packet_release failed!\n");
    return;
  }

  pthread_mutex_lock(&pool_lock);

  for (list<shared_ptr<CameraBuffer>>::iterator i = pool.begin();
       i != pool.end(); i++) {
    if ((*i)->getVirtAddr() == buf_info->ptr) {
      shared_ptr<CameraBuffer> buffer = *i;
      pool.erase(i);
      buffer->decUsedCnt();
      if (buf_info) {
        delete buf_info;
        buf_info = NULL;
      }
      break;
    }
  }

  pthread_mutex_unlock(&pool_lock);
}

static int video_dpp_packet_process(struct Video* video,
                                    shared_ptr<CameraBuffer>& inBuf) {
  DppBufferInfo* buf_info;
  DPP_RET ret = DPP_OK;
  DppPacket packet_in = 0;
  assert(inBuf->getMetaData());
  struct v4l2_buffer_metadata_s* metadata_drv =
      (struct v4l2_buffer_metadata_s*)inBuf->getMetaData()->metedata_drv;
  assert(metadata_drv);
  struct timeval time_val = metadata_drv->frame_t.vs_t;
  int rt = 0;
  struct HAL_Buffer_MetaData* meta = inBuf->getMetaData();

  if (!video->dpp)
    return -1;

  buf_info = new DppBufferInfo();
  if (!buf_info)
    return -1;

  buf_info->type = DPP_BUFFER_TYPE_ION;
  buf_info->size = ((inBuf->getWidth() + 15) & ~15) *
                   ((inBuf->getHeight() + 15) & ~15) * 3 / 2;
  buf_info->ptr = inBuf->getVirtAddr();
  buf_info->hnd = inBuf->getHandle();
  buf_info->fd = (int)(inBuf->getFd());
  buf_info->phys = (unsigned int)(inBuf->getPhyAddr());

  ret = dpp_packet_init((DppPacket*)&packet_in, inBuf->getWidth(),
                        inBuf->getHeight());
  if (DPP_OK != ret) {
    printf("dpp_packet_init_with_buf_info failed.\n");
    rt = -1;
    goto packet_process_exit;
  }
  ret = dpp_packet_set_cb(packet_in, video_dpp_packet_release);
  if (DPP_OK != ret) {
    printf("dpp_packet_set_cb failed.\n");
    rt = -1;
    goto packet_process_exit;
  }
  dpp_packet_set_buf_info(packet_in, buf_info);

  dpp_packet_set_pts(packet_in, time_val);

  dpp_packet_set_noise(packet_in, &user_noise);

  dpp_packet_set_idc_enabled(packet_in, parameter_get_video_idc());
  dpp_packet_set_nr_enabled(packet_in, parameter_get_video_3dnr());

  if (meta) {
    struct ispinfo info;
    memset(&info, 0, sizeof(info));
    info.exp_gain = meta->exp_gain;
    info.exp_time = meta->exp_time;
    info.doortype = meta->awb.DoorType;
    info.wb_gain_red = 0;  // TODO, add other isp information below
    info.wb_gain_green_r = 0;
    info.wb_gain_blue = 0;
    info.wb_gain_green_b = 0;
    dpp_packet_set_data(packet_in, meta);
    dpp_packet_set_params(packet_in, &info, sizeof(info));
  }

  ret = dpp_put_packet(video->dpp->context, packet_in);
  if (DPP_OK != ret) {
    printf("put_packet failed.\n");
    rt = -1;
    goto packet_process_exit;
  }

packet_process_exit:
  if (packet_in)
    dpp_packet_deinit((DppPacket*)packet_in);

  return rt;
}

static void video_record_temp_fun(struct Video* video) {
  static int num = 0;
  TempState status;
  int fps = parameter_get_video_frontcamera_fps();
  temp_get_status(&status);
  temperature = status.temp;

  if (parameter_get_debug_temp()) {
    num++;
    if (num == 30) {
      status.level = temp_level1;
    } else if (num == 60) {
      status.level = temp_level2;
    } else if (num == 90) {
      status.level = temp_level1;
    } else if (num == 120) {
      num = 0;
      status.level = temp_level0;
    }
  } else
    num = 0;

  switch (status.level) {
    case temp_level0:
      video->high_temp = false;
      break;
    case temp_level1:
      video->high_temp = true;
      break;
    case temp_level2: {
      if (1 != video->fps_n || (fps / 2) != video->fps_d)
        video_set_fps(video, 1, (fps / 2));
      break;
    }
  }
  if (status.level != temp_level2)
    if (1 != video->fps_n || fps != video->fps_d)
      video_set_fps(video, 1, fps);
}

static int video_set_fps(struct Video* video, int numerator, int denominator) {
  int ret = 0;
  HAL_FPS_INFO_t fps;
  fps.numerator = numerator;
  fps.denominator = denominator;
  printf("fps : %d/%d\n", fps.numerator, fps.denominator);
  ret = video->hal->dev->setFps(fps);
  if (!ret) {
    video->fps_n = numerator;
    video->fps_d = denominator;
    printf("video set fps is %.2f\n", 1.0 * video->fps_d / video->fps_n);
  }
  return ret;
}

class MP_DSP : public StreamPUBase {
  struct Video* video;

 public:
  MP_DSP(struct Video* p) : StreamPUBase("MP_DSP", true, true) { video = p; }
  ~MP_DSP() {}
  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static unsigned char cnt = 0;
    static bool print = true;
    video_print_name("MP_DSP", &print);

    // static struct timeval t0;
    // static int i = 0;
    // i++;
    // fps_count(&t0,&i,"MP_DSP");
    video->fps_total++;

    cnt++;
    video_record_temp_fun(video);
#if TEST_VIDEO_MARK
    if (cnt % 60 == 0)
      clk_core = video_record_read_clk_core();
#endif

    if (with_mp) {
      if (video->pthread_run && inBuf.get()) {
        if (!video->high_temp) {
          if (!video->dpp_init) {
            printf("MP:temperature is low.\n");
            video_dpp_init(video);
          }

          inBuf->incUsedCnt();

          pthread_mutex_lock(&pool_lock);
          pool.push_back(inBuf);
          if (video_dpp_packet_process(video, inBuf)) {
            pool.pop_back();
            inBuf->decUsedCnt();
            printf("PU push CameraBuffer to DPP failed.\n");
            pthread_mutex_unlock(&pool_lock);
            return false;
          } else {
            pthread_mutex_unlock(&pool_lock);
          }
        } else {
          void* virt = inBuf->getVirtAddr();
          int fd = inBuf->getFd();
          void* hnd = inBuf->getHandle();
          size_t size = inBuf->getDataSize();
          struct timeval time;
          gettimeofday(&time, NULL);

          if (video->dpp_init) {
            printf("MP:temperature is high.\n");
            video_dpp_exit(video);
          }

          // high temperature fps is less than 30,so all process in one.
          if (is_record_mode && video->save_en && video->pthread_run) {
            if (h264_encode_process(video, virt, fd, hnd, size, time)) {
              video_record_signal(video);
            }
          }

          if (video->photo.state == PHOTO_ENABLE) {
            video->photo.state = PHOTO_BEGIN;
            video_rga_photo_process(video, fd);
          }

          if (!with_sp) {
            if (yuyv_display(video, video->width, video->height, fd)) {
              video_record_signal(video);
            }
          }
        }
      }
    }

    return true;
  }
};

class NV12_RAW : public StreamPUBase {
  struct Video* video;

 public:
  NV12_RAW(struct Video* p) : StreamPUBase("MP_DSP", true, true) { video = p; }
  ~NV12_RAW() {}
  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static bool print = true;
    video_print_name("NV12_RAW", &print);
#ifdef NV12_RAW_DATA
    static FILE* f_tmp = NULL;
    static char filename[30] = {0};
    static int cnt = 0;
    char c = '\0';
    char command[50] = {0};
    static int cmd_cnt = 0;
    int i = 0;
    int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;
    static int write_cnt = 0;

    struct timeval t0, t1;

    if (video->pthread_run && inBuf.get() && nv12_raw_cnt % 2) {
      write_cnt++;
      if (write_cnt < 5)
        return true;
      if (!f_tmp) {
        cmd_cnt++;
        snprintf(filename, sizeof(filename), "/mnt/sdcard/NV12_RAW_%d",
                 cmd_cnt);
        f_tmp = fopen(filename, "wb");
      }
      if (f_tmp && cnt < NV12_RAW_CNT) {
        src_w = inBuf->getWidth();
        src_h = inBuf->getHeight();
        src_fd = inBuf->getFd();
        dst_w = video->raw[cnt].width;
        dst_h = video->raw[cnt].height;
        dst_fd = video->raw[cnt].fd;

        gettimeofday(&t0, NULL);
        if (rk_rga_ionfdnv12_to_ionfdnv12_scal(video->raw_fd, src_fd, src_w,
                                               src_h, dst_fd, dst_w, dst_h, 0,
                                               0, src_w, src_h, src_w, src_h)) {
          printf("rk_rga_ionfdnv12_to_ionfdnv12_scal fail!\n");
        }
        gettimeofday(&t1, NULL);
        printf("rga:%ldus\n",
               (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
        cnt++;
      } else if (f_tmp && cnt >= NV12_RAW_CNT) {
        gettimeofday(&t0, NULL);
        for (i = 0; i < NV12_RAW_CNT; i++) {
          fwrite(video->raw[i].buffer, 1, inBuf->getDataSize(), f_tmp);
        }
        gettimeofday(&t1, NULL);
        printf("write:%ldus\n",
               (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
        cnt = 0;
        fflush(f_tmp);
        fclose(f_tmp);
        f_tmp = NULL;
        write_cnt = 0;
      }
    } else {
      if (f_tmp) {
        gettimeofday(&t0, NULL);
        for (i = 0; i < cnt; i++) {
          fwrite(video->raw[i].buffer, 1, inBuf->getDataSize(), f_tmp);
        }
        gettimeofday(&t1, NULL);
        printf("write:%ldus\n",
               (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
        cnt = 0;
        fflush(f_tmp);
        fclose(f_tmp);
        f_tmp = NULL;
        write_cnt = 0;
      }
    }
#endif

    return true;
  }
};

class YUYV_NV12_Stream : public StreamPUBase {
  struct Video* video;

 public:
  YUYV_NV12_Stream(struct Video* p) : StreamPUBase("YUYV_NV12", true, false) {
    video = p;
  }
  ~YUYV_NV12_Stream() {}
  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static bool print = true;
    video_print_name("YUYV_NV12_Stream", &print);
    if (video->pthread_run && inBuf.get() && outBuf.get()) {
      if (convert_yuyv(video->width, video->height, inBuf->getVirtAddr(),
                       outBuf->getVirtAddr(), parameter_get_video_mark())) {
        video_record_signal(video);
        return false;
      }
    }

    return true;
  }
};

class H264_Encode : public StreamPUBase {
  struct Video* video;

 public:
  H264_Encode(struct Video* p) : StreamPUBase("NV12_Display", true, true) {
    video = p;
  }
  ~H264_Encode() {}
  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static bool print = true;
    video_print_name("H264_Encode", &print);
    if (video->save_en && video->pthread_run) {
      void* virt = NULL;
      int fd = -1;
      void* hnd = NULL;
      size_t size = 0;
      struct timeval time_val = {0};
      gettimeofday(&time_val, NULL);
      if (inBuf.get()) {
        virt = inBuf->getVirtAddr();
        fd = (int)inBuf->getFd();
        hnd = inBuf->getHandle();
        size = inBuf->getDataSize();
      }
      if (video->save_en && video->pthread_run &&
          h264_encode_process(video, virt, fd, hnd, size, time_val)) {
        video_record_signal(video);
        return false;
      }
    }
    return true;
  }
};

class NV12_Encode : public StreamPUBase {
  struct Video* video;

 public:
  NV12_Encode(struct Video* p) : StreamPUBase("NV12_Display", true, true) {
    video = p;
  }
  ~NV12_Encode() {}

  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static bool print = true;
    video_print_name("NV12_Encode", &print);
    if (video->save_en && video->pthread_run) {
      void* virt = NULL;
      int fd = -1;
      void* hnd = NULL;
      size_t size = 0;
      struct timeval time_val = {0};
      gettimeofday(&time_val, NULL);
      if (inBuf.get()) {
        struct HAL_Buffer_MetaData* meta = inBuf->getMetaData();
        if (parameter_get_video_mark()) {
          if (video->type == VIDEO_TYPE_ISP) {
            uint32_t noise[3] = {0};
            show_video_mark(inBuf->getWidth(), inBuf->getHeight(),
                            inBuf->getVirtAddr(), video->fps, meta, noise,
                            NULL);
          }
          // else
          //  show_time(inBuf->getWidth(), inBuf->getHeight(),
          //            inBuf->getVirtAddr());
        }
        virt = inBuf->getVirtAddr();
        fd = (int)inBuf->getFd();
        hnd = inBuf->getHandle();
        size = inBuf->getWidth() * inBuf->getHeight() * 3 / 2;
      }
      if (video->save_en && video->pthread_run &&
          h264_encode_process(video, virt, fd, hnd, size, time_val)) {
        video_record_signal(video);
        return false;
      }
    }
    return true;
  }
};

class NV12_Display : public StreamPUBase {
  struct Video* video;

 public:
  NV12_Display(struct Video* p) : StreamPUBase("NV12_Display", true, true) {
    video = p;
  }
  ~NV12_Display() {}
  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static bool print = true;
    video_print_name("NV12_Display", &print);
    if (video->pthread_run && inBuf.get()) {
      if (yuyv_display(video, inBuf->getWidth(), inBuf->getHeight(),
                       (int)(inBuf->getFd()))) {
        video_record_signal(video);
        return false;
      }
    }
    return true;
  }
};

class NV12_MJPG : public StreamPUBase {
  struct Video* video;

 public:
  NV12_MJPG(struct Video* p) : StreamPUBase("NV12_MJPG", true, true) {
    video = p;
  }
  ~NV12_MJPG() {}

  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static bool print = true;
    video_print_name("NV12_MJPG", &print);
    if (video->pthread_run && video->photo.state == PHOTO_ENABLE && inBuf.get()) {
      video->photo.state = PHOTO_BEGIN;
      video_rga_photo_process(video, inBuf->getFd());
    }
    return true;
  }
};

static void iep_process_deinterlace(uint16_t src_w,
                                    uint16_t src_h,
                                    uint32_t src_fd,
                                    uint16_t dst_w,
                                    uint16_t dst_h,
                                    uint32_t dst_fd) {
  iep_interface* api = iep_interface::create_new();
  iep_img src1;
  iep_img dst1;

  src1.act_w = src_w;
  src1.act_h = src_h;
  src1.x_off = 0;
  src1.y_off = 0;
  src1.vir_w = src_w;
  src1.vir_h = src_h;
  src1.format = IEP_FORMAT_YCbCr_420_SP;
  src1.mem_addr = src_fd;
  src1.uv_addr = src_fd | (src_w * src_h) << 10;
  src1.v_addr = 0;

  dst1.act_w = dst_w;
  dst1.act_h = dst_h;
  dst1.x_off = 0;
  dst1.y_off = 0;
  dst1.vir_w = dst_w;
  dst1.vir_h = dst_h;
  dst1.format = IEP_FORMAT_YCbCr_420_SP;
  dst1.mem_addr = dst_fd;
  dst1.uv_addr = dst_fd | (dst_w * dst_h) << 10;
  dst1.v_addr = 0;

  api->init(&src1, &dst1);

  api->config_yuv_deinterlace();

  if (api->run_sync())
    printf("%d failure\n", getpid());

  iep_interface::reclaim(api);
}

class NV12_IEP : public StreamPUBase {
  struct Video* video;

 public:
  NV12_IEP(struct Video* p) : StreamPUBase("NV12_IEP", true, false) {
    video = p;
  }
  ~NV12_IEP() {}
  bool processFrame(shared_ptr<CameraBuffer> inBuf,
                    shared_ptr<CameraBuffer> outBuf) {
    static bool print = true;
    video_print_name("NV12_IEP", &print);
    if (video->pthread_run && inBuf.get() && outBuf.get()) {
      iep_process_deinterlace(inBuf->getWidth(), inBuf->getHeight(),
                              inBuf->getFd(), outBuf->getWidth(),
                              outBuf->getHeight(), outBuf->getFd());
    }

    return true;
  }
};

static void video_enumSensorFmts(struct Video* video) {
  vector<frm_info_t> frmInfos;
  frm_info_t frmFmt;
  int i;

  video->hal->dev->enumSensorFmts(frmInfos);

  for (i = 0; i < frmInfos.size(); i++) {
    frmFmt = frmInfos[i];
    printf("width = %d,height = %d,fmt = %d,fps = %d\n", frmFmt.frmSize.width,
           frmFmt.frmSize.height, frmFmt.frmFmt, frmFmt.fps);
  }
}

static int video_set_white_balance(struct Video* video, int i) {
  enum HAL_WB_MODE mode;

  switch (i) {
    case 0:
      mode = HAL_WB_AUTO;
      break;
    case 1:
      mode = HAL_WB_DAYLIGHT;
      break;
    case 2:
      mode = HAL_WB_FLUORESCENT;
      break;
    case 3:
      mode = HAL_WB_CLOUDY_DAYLIGHT;
      break;
    case 4:
      mode = HAL_WB_INCANDESCENT;
      break;
    default:
      printf("video%d set white balance input error!\n", video->deviceid);
      return -1;
  }

  if (video->hal->dev->setWhiteBalance(mode)) {
    printf("video%d set white balance failed!\n", video->deviceid);
    return -1;
  }

  printf("video%d set white balance sucess!\n", video->deviceid);

  return 0;
}

static int video_set_exposure_compensation(struct Video* video, int i) {
  int aeBias;

  switch (i) {
    case 0:
      aeBias = -300;
      break;
    case 1:
      aeBias = -200;
      break;
    case 2:
      aeBias = -100;
      break;
    case 3:
      aeBias = 0;
      break;
    case 4:
      aeBias = 100;
      break;
    default:
      printf("video%d set AeBias input error!\n", video->deviceid);
      return -1;
  }

  if (video->hal->dev->setAeBias(aeBias)) {
    printf("video%d set AeBias failed!\n", video->deviceid);
    return -1;
  }

  printf("video%d set AeBias success!\n", video->deviceid);

  return 0;
}

static int video_set_power_line_frequency(struct Video* video, int i) {
  if (i != 1 && i != 2) {
    printf("%s error paramerter:%d!\n", __func__, i);
    return 0;
  }

  if (video->hal->dev->setAntiBandMode((enum HAL_AE_FLK_MODE)i)) {
    printf("video%d set power line frequency failed!\n", video->deviceid);
    return -1;
  }

  return 0;
}

static int video_try_format(struct Video* video, frm_info_t& in_frmFmt) {
  if (!video->hal->dev->tryFormat(in_frmFmt, video->frmFmt)) {
    switch (video->frmFmt.frmFmt) {
      case HAL_FRMAE_FMT_MJPEG:
        video->usb_type = USB_TYPE_MJPEG;
        break;
      case HAL_FRMAE_FMT_YUYV:
        video->usb_type = USB_TYPE_YUYV;
        break;
      case HAL_FRMAE_FMT_H264:
        video->usb_type = USB_TYPE_H264;
        break;
      default:
        break;
    }
  } else
    return -1;

  video->width = video->frmFmt.frmSize.width;
  video->height = video->frmFmt.frmSize.height;

  return 0;
}

static void video_h264_save(struct Video* video) {
  struct Video* video_cur;

  pthread_rwlock_rdlock(&notelock);
  video_cur = getfastvideo();
  while (video_cur) {
    if (!strncmp((char*)(video_cur->businfo), (char*)(video->businfo),
                 sizeof(video->businfo))) {
      if (video_cur->usb_type != USB_TYPE_H264)
        video_cur->save_en = 0;
      else
        video->save_en = 0;
    }

    video_cur = video_cur->next;
  }
  pthread_rwlock_unlock(&notelock);
}

static bool video_record_h264_have_added(void) {
  struct Video* video = NULL;
  bool ret = false;

  pthread_rwlock_rdlock(&notelock);
  video = getfastvideo();

  while (video) {
    if (video->usb_type == USB_TYPE_H264) {
      ret = true;
      break;
    }
    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);

  return ret;
}

static int video_init_setting(struct Video* video) {
  if (video->type != VIDEO_TYPE_CIF) {
    if (video_set_power_line_frequency(video, parameter_get_video_fre()))
      return -1;
  }

  if (video_set_white_balance(video, parameter_get_wb()))
    return -1;

  if (video->type == VIDEO_TYPE_ISP) {
    if (video_set_exposure_compensation(video, parameter_get_ex()))
      return -1;
  }

  return 0;
}

static int isp_video_init(struct Video* video,
                          int num,
                          unsigned int width,
                          unsigned int height,
                          unsigned int fps) {
  int i = 0;
  bool exist = false;
  frm_info_t in_frmFmt = {
      .frmSize = {width, height}, .frmFmt = HAL_FRMAE_FMT_NV12, .fps = fps,
  };
  if (with_sp) {
    if (with_mp) {
      frm_info_t spfrmFmt = {
          .frmSize = {fb_width, fb_height},
          .frmFmt = HAL_FRMAE_FMT_NV12,
          .fps = fps,
      };
      memcpy(&video->spfrmFmt, &spfrmFmt, sizeof(frm_info_t));
    } else {
      frm_info_t spfrmFmt = {
          .frmSize = {width, height}, .frmFmt = HAL_FRMAE_FMT_NV12, .fps = fps,
      };
      memcpy(&video->spfrmFmt, &spfrmFmt, sizeof(frm_info_t));
    }
  }

  video->hal->dev = getCamHwItf(&(g_test_cam_infos.isp_dev));
  //(shared_ptr<CamHwItf>)(new CamIsp11DevHwItf());
  if (!video->hal->dev.get()) {
    printf("no memory!\n");
    return -1;
  }

  for (i = 0; i < g_test_cam_infos.num_camers; i++) {
    if (g_test_cam_infos.cam[i]->type == RK_CAM_ATTACHED_TO_ISP) {
      printf("connected isp camera name %s,input id %d\n",
             g_test_cam_infos.cam[i]->name, g_test_cam_infos.cam[i]->index);

      if (video->hal->dev->initHw(g_test_cam_infos.cam[i]->index) == false) {
        printf("video%d init fail!\n", num);
        return -1;
      }

      exist = true;
      break;
    }
  }

  if (!exist)
    return -1;

#if 0
    if (video_try_format(video,in_frmFmt)) {
        printf("video try format failed!\n");
        return -1;
    }
#else
  memcpy(&video->frmFmt, &in_frmFmt, sizeof(frm_info_t));
  video->type = VIDEO_TYPE_ISP;
  video->width = video->frmFmt.frmSize.width;
  video->height = video->frmFmt.frmSize.height;
#endif

  if (video_init_setting(video))
    return -1;

  return 0;
}

int isp_video_path_sp(struct Video* video) {
  video->hal->spath = video->hal->dev->getPath(CamHwItf::MP);
  if (video->hal->spath.get() == NULL) {
    printf("%s:path doesn't exist!\n", __func__);
    return -1;
  }

  printf(
      "isp_video_path_sp:video->hal->bufAlloc.get() = %p\n"
      "   video->spfrmFmt.frmSize.width = %d, video->spfrmFmt.frmSize.height = "
      "%d\n"
      "   video->spfrmFmt.frmFmt = %d, video->spfrmFmt.fps = %d\n",
      video->hal->bufAlloc.get(), video->spfrmFmt.frmSize.width,
      video->spfrmFmt.frmSize.height, video->spfrmFmt.frmFmt,
      video->spfrmFmt.fps);

  if (with_mp) {
    if (video->hal->spath->prepare(video->spfrmFmt, 2,
                                   *(video->hal->bufAlloc.get()), false,
                                   0) == false) {
      printf("sp prepare faild!\n");
      return -1;
    }
  } else {
    if (video->hal->spath->prepare(video->spfrmFmt, 4,
                                   *(video->hal->bufAlloc.get()), false,
                                   0) == false) {
      printf("sp prepare faild!\n");
      return -1;
    }
  }
  printf("sp: width = %4d,height = %4d\n", video->spfrmFmt.frmSize.width,
         video->spfrmFmt.frmSize.height);

  video_set_fps(video, 1, video->spfrmFmt.fps);

  video->hal->sp_disp = shared_ptr<SP_Display>(new SP_Display(video));
  if (!video->hal->sp_disp.get()) {
    printf("new SP_Display failed!\n");
    return -1;
  }
  video->hal->sp_disp->setReqFmt(video->spfrmFmt);
  video->hal->spath->addBufferNotifier(video->hal->sp_disp.get());

  if (!with_mp) {
#ifdef NV12_RAW_DATA
    video->hal->nv12_raw = shared_ptr<NV12_RAW>(new NV12_RAW(video));
    if (!video->hal->nv12_raw.get()) {
      printf("new NV12_RAW failed!\n");
      return -1;
    }
    video->hal->spath->addBufferNotifier(video->hal->nv12_raw.get());
    video->hal->nv12_raw->prepare(video->spfrmFmt, 0, NULL);
#endif

    video->width = video->spfrmFmt.frmSize.width;
    video->height = video->spfrmFmt.frmSize.height;
    if (is_record_mode) {
      video->hal->nv12_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
      if (!video->hal->nv12_enc.get()) {
        printf("new NV12_Encode failed!\n");
        return -1;
      }
      video->hal->nv12_enc->prepare(video->spfrmFmt, 0, NULL);
      video->hal->spath->addBufferNotifier(video->hal->nv12_enc.get());
    }

    video->hal->nv12_mjpg = shared_ptr<NV12_MJPG>(new NV12_MJPG(video));
    if (!video->hal->nv12_mjpg.get()) {
      printf("new NV12_MJPG failed!\n");
      return -1;
    }
    video->hal->nv12_mjpg->prepare(video->spfrmFmt, 0, NULL);
    video->hal->spath->addBufferNotifier(video->hal->nv12_mjpg.get());
  }

  if (with_adas) {
    video->hal->nv12_adas = shared_ptr<NV12_ADAS>(new NV12_ADAS(video));
    if (!video->hal->nv12_adas.get()) {
      printf("new NV12_ADAS failed!\n");
      return -1;
    }
    video->hal->spath->addBufferNotifier(video->hal->nv12_adas.get());
    video->hal->nv12_adas->prepare(video->spfrmFmt, 0, NULL);
  }

  return 0;
}

int isp_video_path_mp(struct Video* video) {
  int i = 0;

  video->hal->mpath = video->hal->dev->getPath(CamHwItf::MP);
  if (video->hal->mpath.get() == NULL) {
    printf("%s:path doesn't exist!\n", __func__);
    return -1;
  }

  printf(
      "isp_video_path_mp:video->hal->bufAlloc.get() = %p\n"
      "   video->frmFmt.frmSize.width = %d, video->frmFmt.frmSize.height = %d\n"
      "   video->frmFmt.frmFmt = %d, video->frmFmt.fps = %d\n",
      video->hal->bufAlloc.get(), video->frmFmt.frmSize.width,
      video->frmFmt.frmSize.height, video->frmFmt.frmFmt, video->frmFmt.fps);

  if (video->hal->mpath->prepare(
          video->frmFmt, 5, *(video->hal->bufAlloc.get()), false, 0) == false) {
    printf("mp prepare faild!\n");
    return -1;
  }
  printf("mp: width = %4d,height = %4d\n", video->frmFmt.frmSize.width,
         video->frmFmt.frmSize.height);

  video_set_fps(video, 1, video->frmFmt.fps);

  video->hal->mp_dsp = shared_ptr<MP_DSP>(new MP_DSP(video));
  if (!video->hal->mp_dsp.get()) {
    printf("new MP_DSP failed!\n");
    return -1;
  }
  video->hal->mpath->addBufferNotifier(video->hal->mp_dsp.get());
  video->hal->mp_dsp->prepare(video->frmFmt, 0, NULL);

#ifdef NV12_RAW_DATA
  video->hal->nv12_raw = shared_ptr<NV12_RAW>(new NV12_RAW(video));
  if (!video->hal->nv12_raw.get()) {
    printf("new NV12_RAW failed!\n");
    return -1;
  }
  video->hal->mpath->addBufferNotifier(video->hal->nv12_raw.get());
  video->hal->nv12_raw->prepare(video->frmFmt, 0, NULL);

  for (i = 0; i < NV12_RAW_CNT; i++) {
    memset (&video->raw[i], 0, sizeof(struct video_ion));
    video->raw[i].client = -1;
    video->raw[i].fd = -1;
    if (video_ion_alloc(&video->raw[i], video->width, video->height)) {
      printf("isp_video_path_mp ion alloc fail!\n");
      return -1;
    }
  }

  video->raw_fd = rk_rga_open();
  if (!video->raw_fd)
    return -1;
#endif

  if (!with_sp) {
    if (with_adas) {
      video->hal->nv12_adas = shared_ptr<NV12_ADAS>(new NV12_ADAS(video));
      if (!video->hal->nv12_adas.get()) {
        printf("new NV12_ADAS failed!\n");
        return -1;
      }
      video->hal->mpath->addBufferNotifier(video->hal->nv12_adas.get());
      video->hal->nv12_adas->prepare(video->frmFmt, 0, NULL);
    }
  }

  return 0;
}

static int isp_video_path(struct Video* video) {
  video->hal->bufAlloc =
      shared_ptr<IonCameraBufferAllocator>(new IonCameraBufferAllocator());
  if (!video->hal->bufAlloc.get()) {
    printf("new IonCameraBufferAllocator failed!\n");
    return -1;
  }

  if (with_mp) {
    if (isp_video_path_mp(video))
      return -1;
  }

  if (with_sp) {
    if (isp_video_path_sp(video))
      return -1;
  }

  return 0;
}

int isp_video_start_sp(struct Video* video) {
  if (!video->hal->spath->start()) {
    printf("spath start failed!\n");
    return -1;
  }

  if (!with_mp) {
#ifdef NV12_RAW_DATA
    if (!video->hal->nv12_raw->start()) {
      printf("nv12_raw start failed!\n");
      return -1;
    }
#endif

    if (is_record_mode && !video->hal->nv12_enc->start()) {
      printf("nv12_enc start failed!\n");
      return -1;
    }

    if (!video->hal->nv12_mjpg->start()) {
      printf("nv12_mjpg start failed!\n");
      return -1;
    }
  }

  if (with_adas) {
    if (!video->hal->nv12_adas->start()) {
      printf("nv12_adas start failed!\n");
      return -1;
    }
  }

  return 0;
}

int isp_video_start_mp(struct Video* video) {
  if (!video->hal->mpath->start()) {
    printf("mpath start failed!\n");
    return -1;
  }

  if (!video->hal->mp_dsp->start()) {
    printf("mp_dsp start failed!\n");
    return -1;
  }

#ifdef NV12_RAW_DATA
  if (!video->hal->nv12_raw->start()) {
    printf("nv12_raw start failed!\n");
    return -1;
  }
#endif

  if (!with_sp) {
    if (with_adas) {
      if (!video->hal->nv12_adas->start()) {
        printf("nv12_adas start failed!\n");
        return -1;
      }
    }
  }

  return 0;
}

static int isp_video_start(struct Video* video) {
  if (with_mp) {
    if (isp_video_start_mp(video))
      return -1;
  }

  if (with_sp) {
    if (isp_video_start_sp(video))
      return -1;
  }

  return 0;
}

void isp_video_deinit_sp(struct Video* video) {
  if (!with_mp) {
#ifdef NV12_RAW_DATA
    if (video->hal->nv12_raw.get()) {
      video->hal->spath->removeBufferNotifer(video->hal->nv12_raw.get());
      video->hal->nv12_raw->stop();
      video->hal->nv12_raw->releaseBuffers();
    }
#endif

    if (is_record_mode && video->hal->nv12_enc.get()) {
      video->hal->spath->removeBufferNotifer(video->hal->nv12_enc.get());
      video->hal->nv12_enc->stop();
      video->hal->nv12_enc->releaseBuffers();
    }

    if (video->hal->nv12_mjpg.get()) {
      video->hal->spath->removeBufferNotifer(video->hal->nv12_mjpg.get());
      video->hal->nv12_mjpg->stop();
      video->hal->nv12_mjpg->releaseBuffers();
    }
  }

  if (with_adas) {
    if (video->hal->nv12_adas.get()) {
      video->hal->spath->removeBufferNotifer(video->hal->nv12_adas.get());
      video->hal->nv12_adas->stop();
      video->hal->nv12_adas->releaseBuffers();
    }
  }

  if (video->hal->sp_disp.get()) {
    video->hal->spath->removeBufferNotifer(video->hal->sp_disp.get());
  }

  if (video->hal->spath.get()) {
    video->hal->spath->stop();
    video->hal->spath->releaseBuffers();
  }
}

void isp_video_deinit_mp(struct Video* video) {
  int i = 0;

#ifdef NV12_RAW_DATA
  if (video->hal->nv12_raw.get()) {
    video->hal->mpath->removeBufferNotifer(video->hal->nv12_raw.get());
    video->hal->nv12_raw->stop();
    video->hal->nv12_raw->releaseBuffers();
  }

  for (i = 0; i < NV12_RAW_CNT; i++)
    video_ion_free(&video->raw[i]);

  if (video->raw_fd)
    rk_rga_close(video->raw_fd);
#endif

  if (!with_sp) {
    if (with_adas) {
      if (video->hal->nv12_adas.get()) {
        video->hal->mpath->removeBufferNotifer(video->hal->nv12_adas.get());
        video->hal->nv12_adas->stop();
        video->hal->nv12_adas->releaseBuffers();
      }
    }
  }

  if (video->hal->mpath.get()) {
    video->hal->mpath->stop();
    video->hal->mpath->releaseBuffers();
  }
}

void isp_video_mp_dsp_stop(struct Video* video) {
  if (video->hal->mp_dsp.get()) {
    video->hal->mpath->removeBufferNotifer(video->hal->mp_dsp.get());
    video->hal->mp_dsp->stop();
  }
}

void isp_video_mp_dsp_release(struct Video* video) {
  if (video->hal->mp_dsp.get())
    video->hal->mp_dsp->releaseBuffers();
}

static void isp_video_deinit(struct Video* video) {
  if (with_mp)
    isp_video_deinit_mp(video);

  if (with_sp)
    isp_video_deinit_sp(video);

  if (video->hal->dev.get()) {
    video->hal->dev->deInitHw();
  }
}

static int cif_video_init(struct Video* video,
                          int num,
                          unsigned int width,
                          unsigned int height,
                          unsigned int fps) {
  int i;
  int cif_num = -1;
  short inputid = parameter_get_cif_inputid();
  bool exist = false;
  frm_info_t in_frmFmt = {
      .frmSize = {width, height}, .frmFmt = HAL_FRMAE_FMT_NV12, .fps = fps,
  };

  // translate to cif num
  for (i = 0; i < g_test_cam_infos.num_camers; i++) {
    if (g_test_cam_infos.cam[i]->type == RK_CAM_ATTACHED_TO_CIF &&
        (((struct rk_cif_dev_info*)(g_test_cam_infos.cam[i]->dev))
             ->video_node.video_index == num)) {
      cif_num =
          ((struct rk_cif_dev_info*)(g_test_cam_infos.cam[i]->dev))->cif_index;
    }
  }
  if (cif_num == -1) {
    printf("can't find cif which video num is %d\n", num);
    return -1;
  }

  video->hal->dev = shared_ptr<CamHwItf>(
      new CamCifDevHwItf(&(g_test_cam_infos.cif_devs.cif_devs[cif_num])));
  if (!video->hal->dev.get()) {
    printf("no memory!\n");
    return -1;
  }

  for (i = 0; i < g_test_cam_infos.num_camers; i++) {
    if (g_test_cam_infos.cam[i]->type == RK_CAM_ATTACHED_TO_CIF &&
        (((struct rk_cif_dev_info*)(g_test_cam_infos.cam[i]->dev))
             ->video_node.video_index == num) &&
        g_test_cam_infos.cam[i]->index == inputid) {
      printf("connected cif camera name %s,input id %d\n",
             g_test_cam_infos.cam[i]->name, g_test_cam_infos.cam[i]->index);

      if (video->hal->dev->initHw(g_test_cam_infos.cam[i]->index) == false) {
        printf("video%d init fail!\n", num);
        return -1;
      }

      exist = true;
      break;
    }
  }

  if (!exist) {
    printf("cif inputid %d no exist\n", inputid);
    return -1;
  }

  if (video_try_format(video, in_frmFmt)) {
    printf("video try format failed!\n");
    return -1;
  }

  if (video_init_setting(video))
    return -1;

  return 0;
}

static int cif_video_path(struct Video* video) {
  video->hal->bufAlloc =
      shared_ptr<IonCameraBufferAllocator>(new IonCameraBufferAllocator());
  if (!video->hal->bufAlloc.get()) {
    printf("new IonCameraBufferAllocator failed!\n");
    return -1;
  }

  video->hal->mpath = video->hal->dev->getPath(CamHwItf::MP);
  if (video->hal->mpath.get() == NULL) {
    printf("%s:path doesn't exist!\n", __func__);
    return -1;
  }

  if (video->hal->mpath->prepare(
          video->frmFmt, 4, *(video->hal->bufAlloc.get()), false, 0) == false) {
    printf("mp prepare faild!\n");
    return -1;
  }
  printf("cif: width = %4d,height = %4d\n", video->frmFmt.frmSize.width,
         video->frmFmt.frmSize.height);

  video->hal->nv12_iep = shared_ptr<NV12_IEP>(new NV12_IEP(video));
  if (!video->hal->nv12_iep.get()) {
    printf("new NV12_IEP failed!\n");
    return -1;
  }
  video->hal->mpath->addBufferNotifier(video->hal->nv12_iep.get());
  video->hal->nv12_iep->prepare(video->frmFmt, 4, video->hal->bufAlloc);

  video->hal->nv12_disp = shared_ptr<NV12_Display>(new NV12_Display(video));
  if (!video->hal->nv12_disp.get()) {
    printf("new NV12_Display failed!\n");
    return -1;
  }
  video->hal->nv12_disp->prepare(video->frmFmt, 0, NULL);
  video->hal->nv12_iep->addBufferNotifier(video->hal->nv12_disp.get());

  if (is_record_mode) {
    video->hal->nv12_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
    if (!video->hal->nv12_enc.get()) {
      printf("new NV12_Encode failed!\n");
      return -1;
    }
    video->hal->nv12_enc->prepare(video->frmFmt, 0, NULL);
    video->hal->nv12_iep->addBufferNotifier(video->hal->nv12_enc.get());
  }

  return 0;
}

static int cif_video_start(struct Video* video) {
  if (!video->hal->mpath->start()) {
    printf("mpath start failed!\n");
    return -1;
  }

  if (!video->hal->nv12_iep->start()) {
    printf("nv12_disp start failed!\n");
    return -1;
  }

  if (!video->hal->nv12_disp->start()) {
    printf("nv12_disp start failed!\n");
    return -1;
  }

  if (is_record_mode && !video->hal->nv12_enc->start()) {
    printf("nv12_enc start failed!\n");
    return -1;
  }

  return 0;
}

static void cif_video_deinit(struct Video* video) {
  if (video->hal->nv12_disp.get()) {
    video->hal->nv12_iep->removeBufferNotifer(video->hal->nv12_disp.get());
    video->hal->nv12_disp->stop();
    video->hal->nv12_disp->releaseBuffers();
  }

  if (is_record_mode && video->hal->nv12_enc.get()) {
    video->hal->nv12_iep->removeBufferNotifer(video->hal->nv12_enc.get());
    video->hal->nv12_enc->stop();
    video->hal->nv12_enc->releaseBuffers();
  }

  if (video->hal->nv12_iep.get()) {
    video->hal->mpath->removeBufferNotifer(video->hal->nv12_iep.get());
    video->hal->nv12_iep->stop();
    video->hal->nv12_iep->releaseBuffers();
  }

  if (video->hal->mpath.get()) {
    video->hal->mpath->stop();
    video->hal->mpath->releaseBuffers();
  }

  if (video->hal->dev.get()) {
    video->hal->dev->deInitHw();
  }
}

static int usb_video_init(struct Video* video,
                          int num,
                          unsigned int width,
                          unsigned int height,
                          unsigned int fps) {
  frm_info_t in_frmFmt = {
      .frmSize = {width, height}, .frmFmt = USB_FMT_TYPE, .fps = fps,
  };

  video->hal->dev =
      (shared_ptr<CamHwItf>)(new CamUSBDevHwItf());  // use usb test
  if (!video->hal->dev.get()) {
    printf("no memory!\n");
    return -1;
  }

  if (video->hal->dev->initHw(num) == false) {
    printf("video%d init fail!\n", num);
    return -1;
  }

  if (video_try_format(video, in_frmFmt)) {
    printf("video try format failed!\n");
    return -1;
  }

  if (video->usb_type == USB_TYPE_H264 && !is_record_mode)
    return -1;

  if (video->usb_type == USB_TYPE_H264 || video_record_h264_have_added())
    video_h264_save(video);

  if (video_init_setting(video))
    return -1;

  return 0;
}

static int usb_video_path_yuyv(struct Video* video) {
  frm_info_t frmFmt = {
      .frmSize = {video->frmFmt.frmSize.width, video->frmFmt.frmSize.height},
      .frmFmt = HAL_FRMAE_FMT_NV12,
      .fps = video->frmFmt.fps,
  };

  if (video->hal->mpath->prepare(video->frmFmt, 4,
                                 *(CameraBufferAllocator*)(NULL), false,
                                 0) == false) {
    printf("mp prepare faild!\n");
    return -1;
  }

  video->hal->yuyv_nv12 =
      shared_ptr<YUYV_NV12_Stream>(new YUYV_NV12_Stream(video));

  if (!video->hal->yuyv_nv12.get()) {
    printf("new YUYV_NV12_Stream failed!\n");
    return -1;
  }

  video->hal->mpath->addBufferNotifier(video->hal->yuyv_nv12.get());
  video->hal->yuyv_nv12->prepare(frmFmt, 4, video->hal->bufAlloc);

  video->hal->nv12_disp = shared_ptr<NV12_Display>(new NV12_Display(video));
  if (!video->hal->nv12_disp.get()) {
    printf("new NV12_Display failed!\n");
    return -1;
  }
  video->hal->nv12_disp->prepare(frmFmt, 0, NULL);
  video->hal->yuyv_nv12->addBufferNotifier(video->hal->nv12_disp.get());

  if (is_record_mode) {
    video->hal->nv12_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
    if (!video->hal->nv12_enc.get()) {
      printf("new NV12_Encode failed!\n");
      return -1;
    }
    video->hal->nv12_enc->prepare(frmFmt, 0, NULL);
    video->hal->yuyv_nv12->addBufferNotifier(video->hal->nv12_enc.get());
  }

  video->hal->nv12_mjpg = shared_ptr<NV12_MJPG>(new NV12_MJPG(video));
  if (!video->hal->nv12_mjpg.get()) {
    printf("new NV12_MJPG failed!\n");
    return -1;
  }
  video->hal->nv12_mjpg->prepare(frmFmt, 0, NULL);
  video->hal->yuyv_nv12->addBufferNotifier(video->hal->nv12_mjpg.get());

  return 0;
}

static int usb_video_path_mjpeg(struct Video* video) {
  frm_info_t frmFmt = {
      .frmSize = {video->frmFmt.frmSize.width, video->frmFmt.frmSize.height},
      .frmFmt = HAL_FRMAE_FMT_YUV422P,
      .fps = video->frmFmt.fps,
  };

  if (video->hal->mpath->prepare(video->frmFmt, 4,
                                 *(CameraBufferAllocator*)(NULL), false,
                                 0) == false) {
    printf("mp prepare faild!\n");
    return -1;
  }

  video->hal->mjpg_photo = shared_ptr<MJPG_Photo>(new MJPG_Photo(video));
  if (!video->hal->mjpg_photo.get()) {
    printf("new MJPG_Photo failed!\n");
    return -1;
  }
  video->hal->mjpg_photo->setReqFmt(video->frmFmt);
  video->hal->mpath->addBufferNotifier(video->hal->mjpg_photo.get());

  video->hal->mjpg_nv12 = shared_ptr<MJPG_NV12>(new MJPG_NV12(video));
  if (!video->hal->mjpg_nv12.get()) {
    printf("new MJPG_NV12 failed!\n");
    return -1;
  }
  video->hal->mpath->addBufferNotifier(video->hal->mjpg_nv12.get());
  video->hal->mjpg_nv12->prepare(frmFmt, 2, video->hal->bufAlloc);

  video->hal->nv12_disp = shared_ptr<NV12_Display>(new NV12_Display(video));
  if (!video->hal->nv12_disp.get()) {
    printf("new NV12_Display failed!\n");
    return -1;
  }
  video->hal->nv12_disp->prepare(frmFmt, 0, NULL);
  video->hal->mjpg_nv12->addBufferNotifier(video->hal->nv12_disp.get());

  if (is_record_mode) {
    video->hal->nv12_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
    if (!video->hal->nv12_enc.get()) {
      printf("new NV12_Encode failed!\n");
      return -1;
    }
    video->hal->nv12_enc->prepare(frmFmt, 0, NULL);
    video->hal->mjpg_nv12->addBufferNotifier(video->hal->nv12_enc.get());
  }

  return 0;
}

static int usb_video_path_h264(struct Video* video) {
  if (video->hal->mpath->prepare(video->frmFmt, 4,
                                 *(CameraBufferAllocator*)(NULL), false,
                                 0) == false) {
    printf("mp prepare faild!\n");
    return -1;
  }

  video->hal->h264_enc = shared_ptr<H264_Encode>(new H264_Encode(video));
  if (!video->hal->h264_enc.get()) {
    printf("new H264_Encode failed!\n");
    return -1;
  }
  video->hal->mpath->addBufferNotifier(video->hal->h264_enc.get());
  video->hal->h264_enc->prepare(video->frmFmt, 0, NULL);

  return 0;
}

static int usb_video_path(struct Video* video) {
  video->hal->mpath = video->hal->dev->getPath(CamHwItf::MP);
  if (video->hal->mpath.get() == NULL) {
    printf("mpath doesn't exist!\n");
    return -1;
  }

  if (video->usb_type != USB_TYPE_H264)
    video->hal->bufAlloc =
        shared_ptr<IonCameraBufferAllocator>(new IonCameraBufferAllocator());

  switch (video->usb_type) {
    case USB_TYPE_YUYV:
      printf("video%d path YUYV\n", video->deviceid);
      if (usb_video_path_yuyv(video))
        return -1;
      break;
    case USB_TYPE_MJPEG:
      printf("video%d path MJPEG\n", video->deviceid);
      if (usb_video_path_mjpeg(video))
        return -1;
      break;
    case USB_TYPE_H264:
      printf("video%d path H264\n", video->deviceid);
      if (usb_video_path_h264(video))
        return -1;
      break;
    default:
      printf("video%d path NULL\n", video->deviceid);
      return -1;
  }

  return 0;
}

static int usb_video_start_yuyv(struct Video* video) {
  if (!video->hal->yuyv_nv12->start()) {
    printf("yuyv_nv12 start failed!\n");
    return -1;
  }

  if (!video->hal->nv12_disp->start()) {
    printf("nv12_disp start failed!\n");
    return -1;
  }

  if (is_record_mode && !video->hal->nv12_enc->start()) {
    printf("nv12_enc start failed!\n");
    return -1;
  }

  if (!video->hal->nv12_mjpg->start()) {
    printf("nv12_mjpg start failed!\n");
    return -1;
  }

  return 0;
}

static int usb_video_start_mjpeg(struct Video* video) {
  if (!video->hal->mjpg_nv12->start()) {
    printf("mjpg_nv12 start failed!\n");
    return -1;
  }

  if (!video->hal->nv12_disp->start()) {
    printf("nv12_disp start failed!\n");
    return -1;
  }

  if (is_record_mode && !video->hal->nv12_enc->start()) {
    printf("nv12_enc start failed!\n");
    return -1;
  }

  return 0;
}

static int usb_video_start_h264(struct Video* video) {
  if (!video->hal->h264_enc->start()) {
    printf("h264_enc start failed!\n");
    return -1;
  }

  return 0;
}

static int usb_video_start(struct Video* video) {
  if (!video->hal->mpath->start()) {
    printf("mpath start failed!\n");
    return -1;
  }

  switch (video->usb_type) {
    case USB_TYPE_YUYV:
      if (usb_video_start_yuyv(video))
        return -1;
      break;
    case USB_TYPE_MJPEG:
      if (usb_video_start_mjpeg(video))
        return -1;
      break;
    case USB_TYPE_H264:
      if (usb_video_start_h264(video))
        return -1;
      break;
    default:
      return -1;
  }

  return 0;
}

static void usb_video_deinit_yuyv(struct Video* video) {
  if (video->hal->nv12_mjpg.get()) {
    video->hal->yuyv_nv12->removeBufferNotifer(video->hal->nv12_mjpg.get());
    video->hal->nv12_mjpg->stop();
    video->hal->nv12_mjpg->releaseBuffers();
  }

  if (is_record_mode && video->hal->nv12_enc.get()) {
    video->hal->yuyv_nv12->removeBufferNotifer(video->hal->nv12_enc.get());
    video->hal->nv12_enc->stop();
    video->hal->nv12_enc->releaseBuffers();
  }

  if (video->hal->nv12_disp.get()) {
    video->hal->yuyv_nv12->removeBufferNotifer(video->hal->nv12_disp.get());
    video->hal->nv12_disp->stop();
    video->hal->nv12_disp->releaseBuffers();
  }

  if (video->hal->yuyv_nv12.get()) {
    video->hal->mpath->removeBufferNotifer(video->hal->yuyv_nv12.get());
    video->hal->yuyv_nv12->stop();
    video->hal->yuyv_nv12->releaseBuffers();
  }
}

static void usb_video_deinit_mjpeg(struct Video* video) {
  if (is_record_mode && video->hal->nv12_enc.get()) {
    video->hal->mjpg_nv12->removeBufferNotifer(video->hal->nv12_enc.get());
    video->hal->nv12_enc->stop();
    video->hal->nv12_enc->releaseBuffers();
  }

  if (video->hal->nv12_disp.get()) {
    video->hal->mjpg_nv12->removeBufferNotifer(video->hal->nv12_disp.get());
    video->hal->nv12_disp->stop();
    video->hal->nv12_disp->releaseBuffers();
  }

  if (video->hal->mjpg_nv12.get()) {
    video->hal->mpath->removeBufferNotifer(video->hal->mjpg_nv12.get());
    video->hal->mjpg_nv12->stop();
    video->hal->mjpg_nv12->releaseBuffers();
  }

  if (video->hal->mjpg_photo.get()) {
    video->hal->mpath->removeBufferNotifer(video->hal->mjpg_photo.get());
  }
}

static void usb_video_deinit_h264(struct Video* video) {
  if (video->hal->h264_enc.get()) {
    video->hal->mpath->removeBufferNotifer(video->hal->h264_enc.get());
    video->hal->h264_enc->stop();
    video->hal->h264_enc->releaseBuffers();
  }
}

static void usb_video_deinit(struct Video* video) {
  switch (video->usb_type) {
    case USB_TYPE_YUYV:
      usb_video_deinit_yuyv(video);
      break;
    case USB_TYPE_MJPEG:
      usb_video_deinit_mjpeg(video);
      break;
    case USB_TYPE_H264:
      usb_video_deinit_h264(video);
      break;
  }

  if (video->hal->mpath.get()) {
    video->hal->mpath->stop();
    video->hal->mpath->releaseBuffers();
  }

  if (video->hal->dev.get()) {
    video->hal->dev->deInitHw();
  }
}

static void video_record_addnode(struct Video* video) {
  struct Video* video_cur;

  pthread_rwlock_wrlock(&notelock);
  video_cur = getfastvideo();
  if (video_cur == NULL) {
    video_list = video;
  } else {
    while (video_cur->next) {
      video_cur = video_cur->next;
    }
    video->pre = video_cur;
    video_cur->next = video;
  }

  pthread_rwlock_unlock(&notelock);
}

static void video_exit_disp(void) {
  struct win* video_win;

  video_win = rk_fb_getvideowin();
  video_ion_buffer_black(&video_win->video_ion);
  if (rk_fb_video_disp(video_win) == -1) {
    printf("rk_fb_video_disp err\n");
  }
}

static void video_record_exit_disp(void) {
  struct win* video_win;

  video_win = rk_fb_getvideowin();
  video_ion_buffer_black(&video_win->video_ion);
  if (rk_fb_video_disp(video_win) == -1) {
    printf("rk_fb_video_disp err\n");
  }
}

static void video_record_deletenode(struct Video* video) {
  struct Video* video_cur = NULL;
  pthread_rwlock_wrlock(&notelock);

  if (video->pre == 0) {
    video_list = video->next;
    if (video_list)
      video_list->pre = 0;
    else
      video_exit_disp();
  } else {
    video->pre->next = video->next;
    if (video->next)
      video->next->pre = video->pre;
  }

  video->pre = NULL;
  video->next = NULL;

  video_cur = getfastvideo();
  while (video_cur) {
    if (video_cur->disp.display)
      break;
    else
      video_cur = video_cur->next;
  }
  if (!video_cur) {
    video_cur = getfastvideo();
    while (video_cur) {
      if (video_cur->usb_type != USB_TYPE_H264) {
        video_cur->disp.display = true;
        break;
      } else
        video_cur = video_cur->next;
    }
  }

  if (video->usb_type == USB_TYPE_MJPEG)
    mjpg_decode_exit(video);

#ifdef USE_WATERMARK
  watermark_deinit(&video->watermark_info);
#endif

  if (video->hal) {
    delete video->hal;
    video->hal = NULL;
  }

  pthread_mutex_destroy(&video->record_mutex);
  pthread_cond_destroy(&video->record_cond);

  if (video) {
    free(video);
    video = NULL;
  }

  pthread_rwlock_unlock(&notelock);
}

void video_record_set_front_camera(void) {
  struct Video* video = NULL;
  struct Video* head = NULL;
  struct Video* pre = NULL;
  struct Video* next = NULL;
  struct Video* end = NULL;

  pthread_rwlock_rdlock(&notelock);

  video = getfastvideo();
  while (video) {
    if (strstr((char*)video->businfo, FRONT) &&
        video->usb_type != USB_TYPE_H264 && video->pre) {
      head = getfastvideo();
      pre = video->pre;
      next = video->next;
      pre->next = next;
      if (next)
        next->pre = pre;
      video_list = video;
      video_list->pre = NULL;
      video_list->next = head;
      head->pre = video_list;
      break;
    }
    video = video->next;
  }

  video = getfastvideo();
  if (video && video->type != VIDEO_TYPE_CIF) {
    video->disp.display = true;
    video = video->next;
    while (video) {
      video->disp.display = false;
      video = video->next;
    }
  } else {
    while (video && video->next) {
      if (video->type != VIDEO_TYPE_CIF)
        break;
      video->disp.display = false;
      video = video->next;
    }
    if (video) {
      video->disp.display = true;
      video = video->next;
    }
    while (video) {
      video->disp.display = false;
      video = video->next;
    }
  }

  pthread_rwlock_unlock(&notelock);
}

static void video_record_wait_flag(const bool* flag, const char* name) {
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);

  while (*flag) {
    gettimeofday(&t1, NULL);
    if (t1.tv_sec - t0.tv_sec > 0) {
      printf("%s %s\n", __func__, name);
      t0 = t1;
    }
    pthread_yield();
  }
}

static int video_photo_init(struct Video *video) {
  if (video->type == VIDEO_TYPE_USB && video->usb_type == USB_TYPE_MJPEG)
    return 0;

  pthread_mutex_init(&video->photo.mutex, NULL);
  pthread_cond_init(&video->photo.condition, NULL);

  if (video_ion_alloc(&video->photo.rga_photo, video->width, video->height))
    return -1;
  if (vpu_nv12_encode_jpeg_init(&video->photo.encode, video->width, video->height))
    return -1;

  if ((video->photo.rga_fd = rk_rga_open()) <= 0)
    return -1;

  if (pthread_create(&video->photo.pid, &global_attr, video_rga_photo_pthread, video)) {
    printf("%s pthread create fail!\n", __func__);
    return -1;
  }

  return 0;
}

static void video_photo_exit(struct Video *video) {
  if (video->type == VIDEO_TYPE_USB && video->usb_type == USB_TYPE_MJPEG)
    return;

  vpu_nv12_encode_jpeg_done(&video->photo.encode);
  video_ion_free(&video->photo.rga_photo);

  if (video->photo.pid) {
    pthread_mutex_lock(&video->photo.mutex);
    video->photo.state = PHOTO_DISABLE;
    pthread_cond_signal(&video->photo.condition);
    pthread_mutex_unlock(&video->photo.mutex);
    pthread_join(video->photo.pid, NULL);
  }

  rk_rga_close(video->photo.rga_fd);

  pthread_mutex_destroy(&video->photo.mutex);
  pthread_cond_destroy(&video->photo.condition);
}

static void* video_record(void* arg) {
  struct Video* video = (struct Video*)arg;
  struct timeval t0;

  printf("%s:%d\n", __func__, __LINE__);

  if ((video->disp.rga_fd = rk_rga_open()) <= 0)
    goto record_exit;

  printf("%s:%d\n", __func__, __LINE__);

  if (video->usb_type == USB_TYPE_MJPEG) {
    if (mjpg_decode_init(video)) {
      goto record_exit;
    }
  }

  printf("%s:%d\n", __func__, __LINE__);

  if (video->type == VIDEO_TYPE_ISP && with_mp) {
    if (video_dpp_init(video)) {
      printf("test init failed!\n");
      goto record_exit;
    }
  }

  printf("%s:%d\n", __func__, __LINE__);

  if (video->type == VIDEO_TYPE_ISP && with_adas) {
    if (video_adas_init(video)) {
      printf("adas init failed!\n");
      goto record_exit;
    }
  }

  printf("%s:%d\n", __func__, __LINE__);

  if (video_photo_init(video))
    goto record_exit;

  printf("%s:%d\n", __func__, __LINE__);

  if (video->type == VIDEO_TYPE_ISP) {
    if (isp_video_path(video))
      goto record_exit;

    if (isp_video_start(video)) {
      printf("isp video start err!\n");
      goto record_exit;
    }
  } else if (video->type == VIDEO_TYPE_USB) {
    if (usb_video_path(video))
      goto record_exit;

    if (usb_video_start(video)) {
      printf("usb video start err!\n");
      goto record_exit;
    }
  } else if (video->type == VIDEO_TYPE_CIF) {
    if (cif_video_path(video))
      goto record_exit;

    if (cif_video_start(video)) {
      printf("cif video start err!\n");
      goto record_exit;
    }
  }

  printf("%s start\n", __func__);
  video->valid = true;
  video_record_wait(video);

record_exit:
  video_record_wait_flag(&video->mp4_encoding, "mp4_encoding");
  video_record_wait_flag(&video->disp.displaying, "displaying");
  video_record_wait_flag(&video->jpeg_dec.decoding, "jpeg_dec.decoding");
  while(video->photo.state != PHOTO_END)
    pthread_yield();

  video_ion_free(&video->disp.yuyv);

  if (is_record_mode)
    video_encode_exit(video);

  // MP_DSP release buffer is asynchronous, we need stop MP_DSP first,
  // and wait dpp release the buffer, the last release MP_DSP buffer.
  if (video->type == VIDEO_TYPE_ISP && with_mp) {
    isp_video_mp_dsp_stop(video);
    video_dpp_exit(video);
    isp_video_mp_dsp_release(video);
  }

  if (video->type == VIDEO_TYPE_ISP)
    isp_video_deinit(video);
  else if (video->type == VIDEO_TYPE_USB)
    usb_video_deinit(video);
  else if (video->type == VIDEO_TYPE_CIF)
    cif_video_deinit(video);

  if (video->type == VIDEO_TYPE_ISP && with_adas)
    video_adas_exit(video);

  video_photo_exit(video);

  rk_rga_close(video->disp.rga_fd);

  video_record_deletenode(video);

  pthread_exit(NULL);
}

static int video_record_query_businfo(struct Video* video, int id) {
  int fd = 0;
  char dev[20] = {0};
  struct v4l2_capability cap;

  snprintf(dev, sizeof(dev), "/dev/video%d", id);
  fd = open(dev, O_RDWR);
  if (fd <= 0) {
    // printf("open %s failed\n",dev);
    return -1;
  }

  if (ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
    // printf("%s VIDIOC_QUERYCAP failed!\n",dev);
    close(fd);
    return -1;
  }

  close(fd);

  memcpy(video->businfo, cap.bus_info, sizeof(video->businfo));
  // printf("%s businfo:%s\n",dev,video->businfo);

  return 0;
}

static bool video_record_isp_have_added(void) {
  struct Video* video = NULL;
  bool ret = false;

  pthread_rwlock_rdlock(&notelock);
  video = getfastvideo();

  while (video) {
    if (video->type == VIDEO_TYPE_ISP) {
      ret = true;
      break;
    }
    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);

  return ret;
}

static bool video_check_abmode(struct Video *video)
{
  if ((parameter_get_abmode() == 0 && video->front) ||
      (parameter_get_abmode() == 1 && !video->front) ||
      (parameter_get_abmode() == 2))
    return true;
  else
    return false;
}

// In order to ensure the camhal will call h264_encode_process
// at least once after it gets an error by hotplug usb camera
static void invoke_fake_buffer_ready(void* arg) {
  struct Video* vnode = (struct Video*)arg;
  if (!vnode || !vnode->hal)
    return;
  if (vnode->type == VIDEO_TYPE_USB) {
    if (vnode->usb_type == USB_TYPE_H264) {
      if (vnode->hal->h264_enc.get())
        vnode->hal->h264_enc->invokeFakeNotify();
    } else {
      if (vnode->hal->nv12_enc.get())
        vnode->hal->nv12_enc->invokeFakeNotify();
    }
  } else {
    // cvbs TODO
    printf("!!TODO, not usb video is dynamically deleted, unsupported now\n");
  }
}

static void start_record(struct Video* vnode) {
  EncodeHandler* handler = vnode->encode_handler;
  if (handler) {
    assert(!(vnode->encode_status & RECORDING_FLAG));
    handler->send_record_mp4_start();
    vnode->encode_status |= RECORDING_FLAG;
  }
}

static void stop_record(struct Video* vnode) {
  if (!vnode->valid)
    return;
  EncodeHandler* handler = vnode->encode_handler;
  if (handler && (vnode->encode_status & RECORDING_FLAG)) {
    FuncBefWaitMsg func(invoke_fake_buffer_ready, vnode);
    handler->send_record_mp4_stop(&func);
    vnode->encode_status &= ~RECORDING_FLAG;
  }
}

extern "C" int video_record_addvideo(int id,
                                     struct ui_frame* front,
                                     struct ui_frame* back,
                                     char rec_immediately) {
  struct Video* video;
  int width = 0, height = 0, fps = 0;
  pthread_attr_t attr;

  if (id < 0 || id >= MAX_VIDEO_DEVICE) {
    printf("video%d exit!\n", id);
    return 0;
  }

  video = (struct Video*)calloc(1, sizeof(struct Video));
  if (!video) {
    printf("no memory!\n");
    goto addvideo_exit;
  }

  pthread_mutex_init(&video->record_mutex, NULL);
  pthread_cond_init(&video->record_cond, NULL);

  video->disp.yuyv.client = -1;
  video->disp.yuyv.fd = -1;
  video->photo.rga_photo.client = -1;
  video->photo.rga_photo.fd = -1;
  video->photo.encode.jpeg_enc_out.client = -1;
  video->photo.encode.jpeg_enc_out.fd = -1;
  video->jpeg_dec.out.client = -1;
  video->jpeg_dec.out.fd = -1;
  video->pthread_run = 1;
  video->photo.state = PHOTO_END;

  if (video_record_query_businfo(video, id))
    goto addvideo_exit;

  if (strstr((char*)video->businfo, "isp")) {
    if (video_record_isp_have_added())
      goto addvideo_exit;
    else
      video->type = VIDEO_TYPE_ISP;
  } else if (strstr((char*)video->businfo, "usb")) {
    video->type = VIDEO_TYPE_USB;
  } else if (strstr((char*)video->businfo, "cif")) {
#ifdef USE_CIF_CAMERA
    video->type = VIDEO_TYPE_CIF;
#else
    goto addvideo_exit;
#endif
  } else {
    printf("businfo error!\n");
    goto addvideo_exit;
  }

  if (strstr((char*)video->businfo, FRONT)) {
    width = front->width;
    height = front->height;
    fps = front->fps;
    video->front = true;
    video->fps_n = 1;
    video->fps_d = fps;
  } else {
    width = back->width;
    height = back->height;
    fps = back->fps;
    video->front = false;
  }

  if (!video_check_abmode(video))
    goto addvideo_exit;

  video->hal = new hal();
  if (!video->hal) {
    printf("no memory!\n");
    goto addvideo_exit;
  }

  video->deviceid = id;
  video->save_en = 1;

  if (video->type == VIDEO_TYPE_ISP) {
    printf("video%d is isp\n", video->deviceid);
    if (isp_video_init(video, id, width, height, fps))
      goto addvideo_exit;
  } else if (video->type == VIDEO_TYPE_USB) {
    printf("video%d is usb\n", video->deviceid);
    if (usb_video_init(video, id, width, height, fps))
      goto addvideo_exit;
  } else if (video->type == VIDEO_TYPE_CIF) {
    printf("video%d is cif\n", video->deviceid);
    if (cif_video_init(video, id, width, height, fps))
      goto addvideo_exit;
  }

  video->pre = 0;
  video->next = 0;

  video_record_addnode(video);

#ifdef USE_WATERMARK
  if (!watermark_setcfg(video->width, video->height, &video->watermark_info))
    watermark_init(video->width, video->height, &video->watermark_info);
#endif

  if (pthread_attr_init(&attr)) {
    printf("pthread_attr_init failed!\n");
    goto addvideo_exit;
  }
  if (pthread_attr_setstacksize(&attr, 256 * 1024)) {
    printf("pthread_attr_setstacksize failed!\n");
    goto addvideo_exit;
  }

  if (is_record_mode && video_encode_init(video)) {
    goto addvideo_exit;
  }

#ifdef USE_WATERMARK
  if (video->encode_handler)
    video->encode_handler->watermark_info = &video->watermark_info;
#endif

  if (rec_immediately)
    start_record(video);

  video_record_set_front_camera();

  if (pthread_create(&video->record_id, &attr, video_record, video)) {
    printf("%s pthread create err!\n", __func__);
    goto addvideo_exit;
  }

  pthread_rwlock_rdlock(&notelock);
  record_id_list.push_back(video->record_id);
  pthread_rwlock_unlock(&notelock);

  if (pthread_attr_destroy(&attr))
    printf("pthread_attr_destroy failed!\n");

  return 0;

addvideo_exit:

  if (video) {
    if (is_record_mode)
      video_encode_exit(video);

    if (video->hal) {
      if (video->hal->dev.get())
        video->hal->dev->deInitHw();

      delete video->hal;
      video->hal = NULL;
    }

    pthread_mutex_destroy(&video->record_mutex);
    pthread_cond_destroy(&video->record_cond);

    free(video);
    video = NULL;
  }

  printf("video%d exit!\n", id);

  return -1;
}

extern "C" int video_record_deletevideo(int deviceid) {
  struct Video* video_cur;

  pthread_rwlock_rdlock(&notelock);
  video_cur = getfastvideo();
  while (video_cur) {
    if (video_cur->deviceid == deviceid) {
      stop_record(video_cur);
      video_record_signal(video_cur);
      break;
    }
    video_cur = video_cur->next;
  }
  pthread_rwlock_unlock(&notelock);

  return 0;
}

static void video_record_get_parameter() {
  with_mp = true;

#if 1
  if (!with_mp)
    with_sp = true;
  else
    with_sp = false;
#else
  with_sp = true;
#endif

  switch (parameter_get_video_ldw()) {
    case 0:
      with_adas = false;
      break;
    case 1:
      with_adas = true;
      break;
    default:
      with_adas = false;
      break;
  }

#if 0
    with_mp = false;
    with_sp = true;
    with_adas = false;
#endif

  printf("%s\n", with_adas ? "with adas" : "without adas");
  printf("%s\n", with_mp ? "with mp" : "without mp");
  printf("%s\n", with_sp ? "with sp" : "without sp");

  printf("%s\n",
         parameter_get_video_autorec() ? "with autorec" : "without autorec");
}

extern "C" void video_record_init(struct ui_frame* front,
                                  struct ui_frame* back) {
  int i;
  unsigned int temp;
  // system("echo -1000 > /proc/$(pidof video)/oom_score_adj");
  system("echo 0 > /proc/sys/vm/overcommit_memory");
  pthread_rwlock_init(&notelock, NULL);
  if (pthread_attr_init(&global_attr)) {
    printf("pthread_attr_init failed!\n");
    return;
  }
  if (pthread_attr_setstacksize(&global_attr, STACKSIZE)) {
    printf("pthread_attr_setstacksize failed!\n");
    return;
  }

  video_record_get_parameter();

  rk_fb_get_resolution((int*)&fb_width, (int*)&fb_height);
  if (fb_width < fb_height) {
    temp = fb_width;
    fb_width = fb_height;
    fb_height = temp;
  }
  fb_width = (fb_width + 15) & (~15);
  fb_height = (fb_height + 15) & (~15);

  memset(&g_test_cam_infos, 0, sizeof(g_test_cam_infos));
  CamHwItf::getCameraInfos(&g_test_cam_infos);

  for (i = 0; i < MAX_VIDEO_DEVICE; i++)
    video_record_addvideo(i, front, back, 0);
}

extern "C" void video_record_deinit(void) {
  struct Video* video_cur;
  int i = 0, j = 0;
  pthread_t record_id[MAX_VIDEO_DEVICE] = { 0 };

  pthread_rwlock_rdlock(&notelock);
  video_cur = getfastvideo();
  while (video_cur) {
    video_record_signal(video_cur);
    video_cur = video_cur->next;
  }
  for (list<pthread_t>::iterator it = record_id_list.begin();
       it != record_id_list.end(); it++)
    record_id[i++] = *it;
  record_id_list.clear();
  pthread_rwlock_unlock(&notelock);

  for (j = 0; j < i; j++) {
    printf("pthread_join record id: %lu\n", record_id[j]);
    pthread_join(record_id[j], NULL);
  }

  if (pthread_attr_destroy(&global_attr))
    printf("pthread_attr_destroy failed!\n");

  video_record_exit_disp();

  pthread_rwlock_destroy(&notelock);
}

static inline bool encode_handler_is_ready(struct Video* node) {
  return node->valid && node->encode_handler;
}

// what means of this ?
static inline bool video_is_active(struct Video* node) {
  return ((parameter_get_abmode() == 0 && node->front) ||
          (parameter_get_abmode() == 1 && !node->front) ||
          (parameter_get_abmode() == 2));
}

extern "C" int video_record_startrec(void) {
  int ret = -1;
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  while (video) {
    if (video->encode_handler &&
        !(video->encode_status & RECORDING_FLAG) &&
        video_is_active(video)) {
      enablerec += 1;
      video->encode_handler->send_record_mp4_start();
      video->encode_status |= RECORDING_FLAG;
      ret = 0;
    }
    video = video->next;
  }
  pthread_rwlock_unlock(&notelock);
  PRINTF("%s, enablerec: %d\n", __func__, enablerec);
  PRINTF_FUNC_OUT;
  return ret;
}
extern "C" int runapp(char* cmd);
extern "C" void video_record_stoprec(void) {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  while (video) {
    if (video->encode_status & RECORDING_FLAG) {
      if (encode_handler_is_ready(video))
        video->encode_handler->send_record_mp4_stop();
      video->encode_status &= ~RECORDING_FLAG;
      enablerec -= 1;
    }
    video = video->next;
  }
  pthread_rwlock_unlock(&notelock);
  char cmd[] = "sync\0";
  runapp(cmd);
  PRINTF("%s, enablerec: %d\n", __func__, enablerec);
  PRINTF_FUNC_OUT;
}

extern "C" void video_record_blocknotify(int prev_num, int later_num) {
  Video* video_cur;
  char* filename;

  pthread_rwlock_rdlock(&notelock);
  video_cur = getfastvideo();
  if (video_cur == NULL) {
    pthread_rwlock_unlock(&notelock);
    return;
  } else {
    filename = (char*)video_cur->encode_handler->filename;
    printf("video_record_blocknotify filename = %s\n", filename);
    fs_manage_blocknotify(prev_num, later_num, filename);
    while (video_cur->next) {
      video_cur = video_cur->next;
      filename = (char*)video_cur->encode_handler->filename;
      printf("video_record_blocknotify filename = %s\n", filename);
      fs_manage_blocknotify(prev_num, later_num, filename);
    }
  }
  pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_savefile(void) {
  pthread_rwlock_rdlock(&notelock);
  struct Video* video = getfastvideo();
  while (video) {
    if (enablerec && encode_handler_is_ready(video))
      video->encode_handler->send_save_file_immediately();
    video = video->next;
  }
  pthread_rwlock_unlock(&notelock);
}

void video_record_start_ts_transfer(char* url) {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  if (video && url && video->encode_handler &&
      !(video->encode_status & WIFI_TRANSFER_FLAG)) {
    assert(url);
    enablerec += 1;
    video->encode_handler->send_ts_transfer_start(url);
    video->encode_status |= WIFI_TRANSFER_FLAG;
  }
  pthread_rwlock_unlock(&notelock);
  PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}

void video_record_stop_ts_transfer(char sync) {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  while (video) {
    // Alas! The fastvideo may be change between start/stop ts transfer...
    if (video->encode_status & WIFI_TRANSFER_FLAG) {
      if (encode_handler_is_ready(video))
        video->encode_handler->sync_ts_transter_stop();
      enablerec -= 1;
      video->encode_status &= ~WIFI_TRANSFER_FLAG;
    }
    video = video->next;
  }
  pthread_rwlock_unlock(&notelock);
  PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}

void video_record_attach_user_muxer(void* muxer, char* uri, int need_av) {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  if (video && encode_handler_is_ready(video)) {
    enablerec += 1;
    video->encode_handler->send_attach_user_muxer((CameraMuxer*)muxer, uri,
                                                  need_av);
  }
  pthread_rwlock_unlock(&notelock);
  PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}

void video_record_detach_user_muxer(void* muxer) {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  if (video && encode_handler_is_ready(video)) {
    video->encode_handler->sync_detach_user_muxer((CameraMuxer*)muxer);
    enablerec -= 1;
  }
  pthread_rwlock_unlock(&notelock);
  PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}

void video_record_attach_user_mdprocessor(void* processor, void* md_attr) {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  if (video && encode_handler_is_ready(video)) {
    enablerec += 1;
    video->encode_handler->send_attach_user_mdprocessor(
        (VPUMDProcessor*)processor, (MDAttributes*)md_attr);
  }
  pthread_rwlock_unlock(&notelock);
  PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}
void video_record_detach_user_mdprocessor(void* processor) {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  if (video && encode_handler_is_ready(video)) {
    video->encode_handler->sync_detach_user_mdprocessor(
        (VPUMDProcessor*)processor);
    enablerec -= 1;
  }
  pthread_rwlock_unlock(&notelock);
  PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}
void video_record_rate_change(void* processor, int low_frame_rate) {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  if (video && encode_handler_is_ready(video)) {
    video->encode_handler->send_rate_change((VPUMDProcessor*)processor,
                                            low_frame_rate);
  }
  pthread_rwlock_unlock(&notelock);
}

void video_record_start_cache(int sec) {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  while (video && encode_handler_is_ready(video)) {
    video->encode_handler->send_cache_data_start(sec);
    video = video->next;
  }
  pthread_rwlock_unlock(&notelock);
}

void video_record_stop_cache() {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  while (video && encode_handler_is_ready(video)) {
    video->encode_handler->send_cache_data_stop();
    video = video->next;
  }
  pthread_rwlock_unlock(&notelock);
}

void video_record_reset_bitrate() {
  pthread_rwlock_rdlock(&notelock);
  Video* video = getfastvideo();
  while (video && encode_handler_is_ready(video)) {
    video->encode_handler->reset_video_bit_rate();
    video = video->next;
  }
  pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_setaudio(int flag) {
  pthread_rwlock_rdlock(&notelock);
  enableaudio = flag;
  Video* video = getfastvideo();
  if (video && video->encode_handler) {
    video->encode_handler->set_audio_mute(enableaudio ? false : true);
  }
  pthread_rwlock_unlock(&notelock);
}

extern "C" int video_record_set_power_line_frequency(int i) {
  struct Video* video = NULL;

  // 1:50Hz,2:60Hz
  if (i != 1 && i != 2) {
    printf("%s parameter wrong\n", __func__);
    return -1;
  }

  pthread_rwlock_rdlock(&notelock);
  video = getfastvideo();

  while (video) {
    video_set_power_line_frequency(video, i);
    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);

  return 0;
}

extern "C" int video_record_set_white_balance(int i) {
  struct Video* video = NULL;

  pthread_rwlock_rdlock(&notelock);
  video = getfastvideo();

  while (video) {
    video_set_white_balance(video, i);

    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);

  return 0;
}

extern "C" int video_record_set_exposure_compensation(int i) {
  struct Video* video = NULL;

  pthread_rwlock_rdlock(&notelock);
  video = getfastvideo();

  while (video) {
    video_set_exposure_compensation(video, i);

    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);

  return 0;
}

extern "C" void video_record_takephoto(void) {
  struct Video* video_cur;

  pthread_rwlock_rdlock(&notelock);
  video_cur = getfastvideo();
  while (video_cur) {
    if (video_cur->photo.state == PHOTO_END)
      video_cur->photo.state = PHOTO_ENABLE;
    video_cur = video_cur->next;
  }
  pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_display_switch() {
  struct Video* video = NULL;
  struct Video* end = NULL;

  pthread_rwlock_rdlock(&notelock);

  video = getfastvideo();

  if (video && video->next) {
    while (video && !video->disp.display) {
      video = video->next;
    }
    if (video) {
      video->disp.display = false;
      video = video->next;
      if (!video) {
        video = getfastvideo();
        video->disp.display = true;
      } else {
        while (video) {
          if (video->usb_type != USB_TYPE_H264) {
            video->disp.display = true;
            break;
          } else {
            video = video->next;
          }
        }
        if (!video) {
          video = getfastvideo();
          video->disp.display = true;
        }
      }
    } else {
      video = getfastvideo();
      video->disp.display = true;
    }

    /*******************************************************************************************
    do {
        video_list = video->next;
        video_list->pre = NULL;
        end = video_list;
        while(end->next)
            end = end->next;
        end->next = video;
        video->pre = end;
        video->next = NULL;
        video = getfastvideo();
        end = NULL;
    } while(video && video->next && video->usb_type == USB_TYPE_H264);
    *******************************************************************************************/
  }

  pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_set_record_mode(bool mode) {
  is_record_mode = mode;
}

extern "C" void video_record_inc_nv12_raw_cnt(void) {
  nv12_raw_cnt++;
  printf("nv12_raw_cnt = %d\n", nv12_raw_cnt);
}

extern "C" void video_record_fps_count(void) {
  struct Video* video = NULL;

  pthread_rwlock_rdlock(&notelock);

  video = getfastvideo();
  while (video) {
    video->fps = video->fps_total - video->fps_last;
    video->fps_last = video->fps_total;

    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);
}

extern "C" int video_record_get_list_num(void) {
  struct Video* video = NULL;
  int num = 0;

  pthread_rwlock_rdlock(&notelock);

  video = getfastvideo();
  while (video) {
    num++;

    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);

  return num;
}

static int video_record_get_resolution(struct Video* video,
                                       struct ui_frame* frame,
                                       int count) {
  int ret = -1;
  int i = 0;
  struct ui_frame isp[2] = {UI_FRAME_1080P, UI_FRAME_720P};
  struct ui_frame usb[2] = {UI_FRAME_720P, UI_FRAME_480P};

  memset(frame, 0, sizeof(struct ui_frame) * count);

  if (video->type == VIDEO_TYPE_ISP) {
    for (i = 0; i < count && i < sizeof(isp) / sizeof(isp[0]); i++) {
      frame[i].width = isp[i].width;
      frame[i].height = isp[i].height;
      frame[i].fps = isp[i].fps;
    }
    ret = 0;
  } else if (video->type != VIDEO_TYPE_ISP) {
    for (i = 0; i < count && i < sizeof(usb) / sizeof(usb[0]); i++) {
      frm_info_t in_frmFmt = {
          .frmSize = {(unsigned int)usb[i].width, (unsigned int)usb[i].height},
          .frmFmt = USB_FMT_TYPE,
          .fps = 30,
      };
      video_try_format(video, in_frmFmt);
      frame[i].width = video->width;
      frame[i].height = video->height;
      frame[i].fps = in_frmFmt.fps;
    }
    ret = 0;
  } else {
    printf("%s : unsupport type\n", __func__);
  }

  return ret;
}

extern "C" int video_record_get_front_resolution(struct ui_frame* frame,
                                                 int count) {
  struct Video* video = NULL;
  int ret = -1;

  pthread_rwlock_rdlock(&notelock);

  video = getfastvideo();
  while (video) {
    if ((!strcmp("isp", FRONT) && video->type == VIDEO_TYPE_ISP) ||
        (!strcmp("usb", FRONT) && video->type != VIDEO_TYPE_ISP)) {
      ret = video_record_get_resolution(video, frame, count);
    }

    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);

  return ret;
}

extern "C" int video_record_get_back_resolution(struct ui_frame* frame,
                                                int count) {
  struct Video* video = NULL;
  int ret = -1;

  pthread_rwlock_rdlock(&notelock);

  video = getfastvideo();
  while (video) {
    if ((strcmp("isp", FRONT) && video->type == VIDEO_TYPE_ISP) ||
        (strcmp("usb", FRONT) && video->type != VIDEO_TYPE_ISP)) {
      ret = video_record_get_resolution(video, frame, count);
    }

    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);

  return ret;
}

extern "C" void video_record_update_time() {
  struct Video* video = NULL;

  pthread_rwlock_rdlock(&notelock);

  WATERMARK_RECT rect;
  rect.x = WATERMARK_TIME_X;
  rect.y = WATERMARK_TIME_Y;
  rect.w = WATERMARK_TIME_W;
  rect.h = WATERMARK_TIME_H;
  rect.width = WATERMARK_TIME_W;
  rect.height = WATERMARK_TIME_H;

  video = getfastvideo();
  while (video) {
    if (video->watermark_info.type & WATERMARK_TIME) {
#ifdef USE_WATERMARK
      update_rectBmp(
          rect, video->watermark_info.coord_info.time_bmp,
          (Uint8*)video->watermark_info.watermark_data.buffer +
              video->watermark_info.osd_data_offset.time_data_offset);
#endif
    }

    video = video->next;
  }

  pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_get_user_noise(void) {
  FILE* fp = NULL;
  char noise[10] = {0};

  fp = fopen("/mnt/sdcard/dsp_cfg_noise", "rb");
  if (fp) {
    fgets(noise, sizeof(noise), fp);
    user_noise = atoi(noise);
    printf("user_noise = %d\n", user_noise);
    fclose(fp);
  } else {
    printf("/mnt/sdcard/dsp_cfg_noise not exist!\n");
  }
}

extern "C" int REC_RegEventCallback(void (*call)(int cmd, void *msg0, void *msg1))
{
  rec_event_call = call;
  return 0;
}

#include "video_interface.cpp_internal"
