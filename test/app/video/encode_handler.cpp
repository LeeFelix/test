#include <stdio.h>
extern "C" {
#include "fs_manage/fs_fileopera.h"
#include "parameter.h"
#include "rk_rga/rk_rga.h"
}

#include "encode_handler.h"
#include "video.h"

class BUFFEREDH264Encoder : public H264Encoder {
 private:
  struct video_ion encode_src_buff;
  struct video_ion encode_dst_buff;
  DataBuffer_t src_buf, dst_buf;
  BUFFEREDH264Encoder();

 public:
  ~BUFFEREDH264Encoder();
  int encode_init_config(video_encode_config* config);
  EncodedPacket* encode_one_frame(int src_fd, video_encode_config* src_config);
  CREATE_FUNC(BUFFEREDH264Encoder)
};

BUFFEREDH264Encoder::BUFFEREDH264Encoder() {
  memset(&encode_src_buff, 0, sizeof(struct video_ion));
  encode_src_buff.client = -1;
  encode_src_buff.fd = -1;
  memset(&encode_dst_buff, 0, sizeof(struct video_ion));
  encode_dst_buff.client = -1;
  encode_dst_buff.fd = -1;
  memset(&src_buf, 0, sizeof(DataBuffer_t));
  memset(&dst_buf, 0, sizeof(DataBuffer_t));
}

BUFFEREDH264Encoder::~BUFFEREDH264Encoder() {
  video_ion_free(&encode_src_buff);
  video_ion_free(&encode_dst_buff);
}

int BUFFEREDH264Encoder::encode_init_config(video_encode_config* config) {
  int ret = H264Encoder::encode_init_config(config);
  if (0 > ret) {
    return ret;
  }

  if (video_ion_alloc(&encode_src_buff, config->width, config->height) == -1) {
    printf("%s alloc encode_src_buff err, no memory!\n", __func__);
    return -1;
  }
  if (video_ion_alloc_rational(&encode_dst_buff, config->width, config->height,
                               1, 2) == -1) {
    printf("%s alloc encode_dst_buff err, no memory!\n", __func__);
    return -1;
  }
  src_buf.vir_addr = (uint8_t*)encode_src_buff.buffer;
  src_buf.phy_fd = encode_src_buff.fd;
  src_buf.handle = (void*)(&encode_src_buff.handle);
  src_buf.buf_size = encode_src_buff.size;
  dst_buf.vir_addr = (uint8_t*)encode_dst_buff.buffer;
  dst_buf.phy_fd = encode_dst_buff.fd;
  dst_buf.handle = (void*)(&encode_dst_buff.handle);
  dst_buf.buf_size = encode_dst_buff.size;

  return 0;
}

EncodedPacket* BUFFEREDH264Encoder::encode_one_frame(
    int src_fd,
    video_encode_config* src_config) {
  assert(encode_src_buff.buffer);
  int ret = rk_rga_ionfdnv12_to_ionfdnv12_scal_ext(
      src_fd, src_config->width, src_config->height, encode_src_buff.fd,
      encode_src_buff.width, encode_src_buff.height, 0, 0,
      encode_src_buff.width, encode_src_buff.height, src_config->width,
      src_config->height);
  if (ret == 0) {
    EncodedPacket* pkt = new EncodedPacket();
    if (!pkt) {
      printf("alloc EncodedPacket failed\n");
      assert(0);
      return NULL;
    }
    ret = encode_video_frame(&src_buf, &dst_buf, NULL, &pkt->av_pkt);
    if (0 == ret) {
      pkt->type = VIDEO_PACKET;
      pkt->is_phy_buf = false;
      return pkt;
    }
    pkt->unref();
  }
  return NULL;
}

EncodeHandler::EncodeHandler() {
  msg_queue = NULL;
  audio_capture = NULL;
  audio_enable = false;
  mute = false;
  audio_tid = 0;
#ifdef DEBUG
  process_pid = 0;
#endif
#if defined(STRESS_TEST) || defined(MUL_ENCODERS_DEBUG)
  t_pre_data_coming = 0;
#endif
  memset(&encode_dst_buff, 0, sizeof(encode_dst_buff));
  encode_dst_buff.client = -1;
  encode_dst_buff.fd = -1;
  memset(&encode_mv_buff, 0, sizeof(encode_mv_buff));
  encode_mv_buff.client = -1;
  encode_mv_buff.fd = -1;
  memset(&common_config, 0, sizeof(common_config));
  memset(&scale_h264_config, 0, sizeof(scale_h264_config));
  memset(&start_timestamp, 0, sizeof(start_timestamp));
  watermark_info = NULL;
  h264_encoder = NULL;
  scale_h264_encoder = NULL;
  aac_encoder = NULL;
  cameramp4 = NULL;
  camerats = NULL;
  cameracache = NULL;

  filename[0] = 0;
  global_attr = NULL;
  record_time_notify = NULL;
}

EncodeHandler::~EncodeHandler() {
  // should closed all these pointers before deleting encode hanlder
  assert(!audio_enable);
  assert(!camerats);
  assert(!cameramp4);
  assert(!h264_encoder);
  assert(!aac_encoder);
  assert(!scale_h264_encoder);

  video_ion_free(&encode_dst_buff);
  video_ion_free(&encode_mv_buff);

  if (audio_capture) {
    if (audio_capture->handle) {
      printf("warning: audio capture is alive when exit encode\n");
      alsa_capture_done(audio_capture);
    }
    free(audio_capture);
    audio_capture = NULL;
  }

  if (msg_queue) {
    if (!msg_queue->Empty()) {
      printf(
          "warning: encoder handler msg_queue is not empty,"
          " camera error before??\n");
    }
    msg_queue->clear();
    delete msg_queue;
    msg_queue = NULL;
  }
}

EncodeHandler* EncodeHandler::create(int dev_id,
                                     int usb_type,
                                     int w,
                                     int h,
                                     int fps,
                                     int ac_dev_id,
                                     enum AVPixelFormat pixel_fmt) {
  EncodeHandler* pHandler = new EncodeHandler();
  if (pHandler &&
      !pHandler->init(dev_id, usb_type, w, h, fps, ac_dev_id, pixel_fmt)) {
    return pHandler;
  } else {
    if (pHandler) {
      delete pHandler;
    }
    return NULL;
  }
}

int EncodeHandler::init(int dev_id,
                        int usb_type,
                        int width,
                        int height,
                        int fps,
                        int ac_dev_id,
                        enum AVPixelFormat pixel_fmt) {
  // w*h/2 is enough
  if (video_ion_alloc_rational(&encode_dst_buff, width, height, 1, 2) == -1) {
    PRINTF_NO_MEMORY;
    return -1;
  }
  // 32 bits per MB. picture width should be aligned up to 256 pixels(16MBs).
  if (video_ion_alloc_rational(&encode_mv_buff, (width + 255) & (~255), height,
                               4, 256) == -1) {
    PRINTF_NO_MEMORY;
    return -1;
  }

  if (ac_dev_id >= 0) {
    audio_capture =
        (struct alsa_capture*)calloc(1, sizeof(struct alsa_capture));
    if (!audio_capture) {
      PRINTF_NO_MEMORY;
      return -1;
    }
    audio_capture->sample_rate = 44100;
    audio_capture->format = SND_PCM_FORMAT_S16_LE;
    audio_capture->dev_id = ac_dev_id;
  }

  output_buf.vir_addr = (uint8_t*)encode_dst_buff.buffer;
  output_buf.phy_fd = encode_dst_buff.fd;
  output_buf.handle = (void*)(&encode_dst_buff.handle);
  output_buf.buf_size = encode_dst_buff.size;

  mv_buf.vir_addr = (uint8_t*)encode_mv_buff.buffer;
  mv_buf.phy_fd = encode_mv_buff.fd;
  mv_buf.handle = (void*)(&encode_mv_buff.handle);
  mv_buf.buf_size = encode_mv_buff.size;

  common_config.width = width;
  common_config.height = height;
  common_config.pixel_fmt = pixel_fmt;
  common_config.video_bit_rate =
      parameter_get_bit_rate_per_pixel() * width * height;
  common_config.video_bit_rate /= 1000000;
  common_config.video_bit_rate *= 1000000;
  common_config.stream_frame_rate = fps;
  common_config.video_encoder_level = 52;
  common_config.video_gop_size = 15;
  common_config.video_profile = 100;

  if (audio_capture) {
    common_config.audio_bit_rate = 64000;
    common_config.input_nb_samples = 1024;
    switch (audio_capture->format) {
      case SND_PCM_FORMAT_S16_LE:
        common_config.input_sample_fmt = AV_SAMPLE_FMT_S16;
        break;
      default:
        // TODO add other fmts
        assert(0);
    }

    // TODO
    if (audio_capture->channel == 1)
      common_config.channel_layout = AV_CH_LAYOUT_MONO;
    else
      common_config.channel_layout = AV_CH_LAYOUT_STEREO;

    common_config.audio_sample_rate = audio_capture->sample_rate;
  }

  // set the deault values
  scale_h264_config = common_config;
  scale_h264_config.width = SCALED_WIDTH;
  scale_h264_config.height = SCALED_HEIGHT;
  scale_h264_config.video_bit_rate = SCALED_BIT_RATE;

  msg_queue = new MessageQueue<EncoderMessageID>();
  if (!msg_queue) {
    PRINTF_NO_MEMORY;
    return -1;
  }
  this->device_id = dev_id;
  this->usb_type = usb_type;
  return 0;
}

// should have unref h264_encoder and change the bit_rate_per_pixel in parameter
void EncodeHandler::reset_video_bit_rate() {
  assert(!h264_encoder);
  common_config.video_bit_rate = parameter_get_bit_rate_per_pixel() *
                                 common_config.width * common_config.height;
  common_config.video_bit_rate /= 1000000;
  common_config.video_bit_rate *= 1000000;
}

// should have unref h264_encoder
void EncodeHandler::set_pixel_format(enum AVPixelFormat pixel_fmt,
                                     int encoder_id) {
  if (encoder_id == H264_ENCODER_ID)
    common_config.pixel_fmt = pixel_fmt;
  else
    scale_h264_config.pixel_fmt = pixel_fmt;
}

int EncodeHandler::send_message(EncoderMessageID id,
                                void* arg1,
                                void* arg2,
                                void* arg3,
                                bool sync,
                                FuncBefWaitMsg* call_func) {
  Message<EncoderMessageID>* msg = Message<EncoderMessageID>::create(id);
  if (msg) {
    msg->arg1 = arg1;
    msg->arg2 = arg2;
    msg->arg3 = arg3;
    if (sync) {
      std::mutex* mtx = (std::mutex*)msg->arg2;
      std::condition_variable* cond = (std::condition_variable*)msg->arg3;
      {
        std::unique_lock<std::mutex> _lk(*mtx);
        msg_queue->Push(msg);
        if (call_func)
          (*call_func)();
        cond->wait(_lk);
      }
    } else {
      msg_queue->Push(msg);
    }
    return 0;
  } else {
    PRINTF_NO_MEMORY;
    return -1;
  }
}

#define DECLARE_MUTEX_COND \
  std::mutex mtx;          \
  std::condition_variable cond

void EncodeHandler::send_encoder_start(int encoder_id) {
  EncoderMessageID msg_id =
      (encoder_id == H264_ENCODER_ID) ? START_ENCODE : START_ENCODE2;
  send_message(msg_id);
}

void EncodeHandler::send_encoder_stop(int encoder_id) {
  EncoderMessageID msg_id =
      (encoder_id == H264_ENCODER_ID) ? STOP_ENCODE : STOP_ENCODE2;
  DECLARE_MUTEX_COND;
  send_message(msg_id, (void*)true, &mtx, &cond, true);
}

void EncodeHandler::send_record_mp4_start() {
  send_message(START_RECODEMP4);
}

void EncodeHandler::send_record_mp4_stop(FuncBefWaitMsg* call_func) {
  DECLARE_MUTEX_COND;
  send_message(STOP_RECODEMP4, (void*)true, &mtx, &cond, true, call_func);
}

void EncodeHandler::send_ts_transfer_start(char* url) {
  char* uri = (char*)malloc(strlen(url) + 1);
  if (!uri) {
    PRINTF_NO_MEMORY;
    return;
  }
  sprintf(uri, "%s", url);
  if (send_message(START_TSTRANSFER, uri) < 0) {
    free(uri);
  }
}

void EncodeHandler::send_ts_transter_stop() {
  send_message(STOP_TSTRANSFER);
}

void EncodeHandler::sync_ts_transter_stop() {
  DECLARE_MUTEX_COND;
  send_message(STOP_TSTRANSFER, NULL, &mtx, &cond, true);
}
void EncodeHandler::send_save_file_immediately() {
  send_message(SAVE_MP4_IMMEDIATELY);
}

void EncodeHandler::send_cache_data_start(int cache_duration) {
  send_message(START_CACHE, (void*)cache_duration);
}

void EncodeHandler::send_cache_data_stop() {
  send_message(STOP_CACHE);
}

void EncodeHandler::send_attach_user_muxer(CameraMuxer* muxer,
                                           char* uri,
                                           int need_av,
                                           int encoder_id) {
  EncoderMessageID msg_id =
      (encoder_id == H264_ENCODER_ID) ? ATTACH_USER_MUXER : ATTACH_USER_MUXER2;
  send_message(msg_id, muxer, uri, (void*)need_av);
}

void EncodeHandler::sync_detach_user_muxer(CameraMuxer* muxer, int encoder_id) {
  DECLARE_MUTEX_COND;
  EncoderMessageID msg_id =
      (encoder_id == H264_ENCODER_ID) ? DETACH_USER_MUXER : DETACH_USER_MUXER2;
  send_message(msg_id, muxer, &mtx, &cond, true);
}

void EncodeHandler::send_attach_user_mdprocessor(VPUMDProcessor* p,
                                                 MDAttributes* attr) {
  send_message(ATTACH_USER_MDP, p, attr);
}

void EncodeHandler::sync_detach_user_mdprocessor(VPUMDProcessor* p) {
  DECLARE_MUTEX_COND;
  send_message(DETACH_USER_MDP, p, &mtx, &cond, true);
}

void EncodeHandler::send_rate_change(VPUMDProcessor* p, int low_frame_rate) {
  send_message(RECORD_RATE_CHANGE, p, (void*)low_frame_rate);
}

void EncodeHandler::send_gen_idr_frame(int encoder_id) {
  send_message(GEN_IDR_FRAME, (void*)encoder_id);
}

static void* video_record_audio(void* arg) {
  EncodeHandler* handler = (EncodeHandler*)arg;
  AACEncoder* aac_encoder = handler->get_aac_encoder();
  PacketDispatcher* h264aac_pkt_dispatcher =
      handler->get_h264aac_pkt_dispatcher();
  struct alsa_capture* audio_capture = handler->get_audio_capture();
  int ret = 0;
  int nb_samples = 0;
  while (1) {
    // get from av framework
    void* audio_buf = NULL;
    int channels = 0;
    if (handler->get_audio_tid() && !handler->is_audio_enable()) {
      bool finish = false;
      while (!finish) {
        AVPacket av_pkt;
        uint64_t a_idx = 0;
        av_init_packet(&av_pkt);
        aac_encoder->encode_flush_audio_data(&av_pkt, a_idx, finish);
        PRINTF("~~finish : %d\n", finish);
        if (!finish) {
          EncodedPacket* pkt = new EncodedPacket();
          if (!pkt) {
            av_packet_unref(&av_pkt);
            printf("alloc EncodedPacket failed\n");
            assert(0);
            ret = -1;
            goto out;
          }
          pkt->type = AUDIO_PACKET;
          pkt->is_phy_buf = false;
          pkt->av_pkt = av_pkt;
          pkt->audio_index = a_idx;
          h264aac_pkt_dispatcher->Dispatch(pkt);
          pkt->unref();
        }
      }
      break;
    }

    aac_encoder->encode_get_audio_buffer(&audio_buf, &nb_samples, &channels);
    if (audio_buf) {
      EncodedPacket* pkt = NULL;
      audio_capture->buffer_frames = nb_samples;
      int capture_samples = alsa_capture_doing(audio_capture, audio_buf);
      if (capture_samples <= 0) {
        printf("capture_samples : %d, expect samples: %d\n", capture_samples,
               nb_samples);
        alsa_capture_done(audio_capture);
        if (alsa_capture_open(audio_capture, audio_capture->dev_id))
          goto out;
        capture_samples = alsa_capture_doing(audio_capture, audio_buf);
        if (capture_samples <= 0) {
          printf(
              "there seems something wrong with audio capture, disable it\n");
          printf("capture_samples : %d, expect samples: %d\n", capture_samples,
                 nb_samples);
          goto out;
        }
      }
      if (handler->get_audio_mute()) {
        memset(audio_buf, 0, nb_samples * 2 * channels);
      }

      pkt = new EncodedPacket();
      if (!pkt) {
        printf("alloc EncodedPacket failed\n");
        assert(0);
        ret = -1;
        handler->set_enable_audio(false);
        goto out;
      }
      pkt->is_phy_buf = false;
      aac_encoder->encode_audio_frame(&pkt->av_pkt, pkt->audio_index);
      pkt->type = AUDIO_PACKET;
      h264aac_pkt_dispatcher->Dispatch(pkt);
      pkt->unref();
    } else {
      printf("audio_buf is null\n");
    }
  }
out:
  printf("%s out\n", __func__);
  pthread_exit((void*)ret);
}

int EncodeHandler::encode_start_audio_thread() {
  if (audio_capture && audio_capture->dev_id >= 0 && !audio_enable) {
    if (alsa_capture_open(audio_capture, audio_capture->dev_id) == -1) {
      printf("alsa_capture_open err\n");
      return -1;
    }
    pthread_t tid = 0;
    if (pthread_create(&tid, global_attr, video_record_audio, (void*)this)) {
      printf("create audio pthread err\n");
      return -1;
    }
    audio_enable = true;
    audio_tid = tid;
  }
  return 0;
}

void EncodeHandler::encode_stop_audio_thread() {
  if (audio_tid) {
    audio_enable = false;
    pthread_join(audio_tid, NULL);
    audio_tid = 0;
  }
  if (audio_capture) {
    alsa_capture_done(audio_capture);
  }
}

#define ENCODER_REF(CLASSTYPE, VAR, PCONFIG)         \
  if (!VAR) {                                        \
    CLASSTYPE* encoder = CLASSTYPE::create();        \
    if (!encoder) {                                  \
      printf("create " #VAR " failed\n");            \
      return -1;                                     \
    }                                                \
    if (0 != encoder->encode_init_config(PCONFIG)) { \
      encoder->unref();                              \
      return -1;                                     \
    }                                                \
    VAR = encoder;                                   \
  } else {                                           \
    VAR->ref();                                      \
  }                                                  \
  PRINTF("ref " #VAR " : %p, refcount : %d\n", VAR, VAR->refcount())

#define ENCODER_UNREF(VAR)                                       \
  if (VAR) {                                                     \
    int refcnt = VAR->refcount();                                \
    if (0 == VAR->unref()) {                                     \
      VAR = NULL;                                                \
    }                                                            \
    PRINTF("unref " #VAR " : %p, refcount : %d\n", VAR, refcnt); \
  }

// the following functions should only be called in h264_encode_process

int EncodeHandler::h264_encoder_ref() {
  ENCODER_REF(H264Encoder, h264_encoder, &common_config);
  return 0;
}
void EncodeHandler::h264_encoder_unref() {
  ENCODER_UNREF(h264_encoder);
}
int EncodeHandler::aac_encoder_ref() {
  if (!audio_capture)
    return 0;  // audio device is not ready
  ENCODER_REF(AACEncoder, aac_encoder, &common_config);
  if (aac_encoder->refcount() == 1) {
    if (0 != encode_start_audio_thread()) {
      printf("start audio thread failed\n");
      if (0 == aac_encoder->unref()) {
        aac_encoder = NULL;
      }
      return -1;
    }
  }
  return 0;
}
void EncodeHandler::aac_encoder_unref() {
  if (!aac_encoder)
    return;
  int refcnt = aac_encoder->refcount();
  if (refcnt == 1) {
    encode_stop_audio_thread();
  }
  if (0 == aac_encoder->unref()) {
    aac_encoder = NULL;
  }
  PRINTF("unref aac_encoder : %p, refcount : %d\n", aac_encoder, refcnt);
}

int EncodeHandler::h264aac_encoder_ref() {
  int ret = h264_encoder_ref();
  if (0 == ret) {
    ret = aac_encoder_ref();
  }
  return ret;
}

void EncodeHandler::h264aac_encoder_unref() {
  aac_encoder_unref();
  h264_encoder_unref();
}

int EncodeHandler::scale_h264_encoder_ref() {
  ENCODER_REF(BUFFEREDH264Encoder, scale_h264_encoder, &scale_h264_config);
  return 0;
}

void EncodeHandler::scale_h264_encoder_unref() {
  ENCODER_UNREF(scale_h264_encoder);
}

int EncodeHandler::attach_user_muxer(CameraMuxer* muxer,
                                     char* uri,
                                     bool need_video,
                                     bool need_audio,
                                     int encoder_id) {
  assert(muxer);
  H264Encoder* vencoder = NULL;
  AACEncoder* aencoder = NULL;
  int ret = 0;
  int rate = 0;
  if (need_video) {
    if (encoder_id == H264_ENCODER_ID) {
      ret = h264_encoder_ref();
      vencoder = ret ? NULL : h264_encoder;
    } else {
      ret = scale_h264_encoder_ref();
      vencoder = ret ? NULL : scale_h264_encoder;
    }
    rate = vencoder->encode_config.stream_frame_rate;
  }
  if (need_audio) {
    ret = aac_encoder_ref();
    aencoder = ret ? NULL : aac_encoder;
  }
  if (0 > muxer->init_uri(uri, rate)) {
    goto err;
  }
  muxer->set_encoder(vencoder, aencoder);
  if (0 > muxer->start_new_job(global_attr)) {
    goto err;
  } else {
    if (encoder_id == H264_ENCODER_ID)
      h264aac_pkt_dispatcher.AddHandler(muxer);
    else
      scale_h264_pkt_dispatcher.AddHandler(muxer);
  }
  return 0;
err:
  if (vencoder) {
    if (vencoder == h264_encoder)
      h264_encoder_unref();
    else if (vencoder == scale_h264_encoder)
      scale_h264_encoder_unref();
  }
  if (aencoder)
    aac_encoder_unref();
  return -1;
}

void EncodeHandler::detach_user_muxer(CameraMuxer* muxer, int encoder_id) {
  BaseEncoder* vencoder = muxer->get_video_encoder();
  BaseEncoder* aencoder = muxer->get_audio_encoder();
  if (vencoder) {
    if (vencoder == h264_encoder)
      h264_encoder_unref();
    else if (vencoder == scale_h264_encoder)
      scale_h264_encoder_unref();
  }
  if (aencoder && aencoder == aac_encoder)
    aac_encoder_unref();
  if (encoder_id == H264_ENCODER_ID)
    h264aac_pkt_dispatcher.RemoveHandler(muxer);
  else
    scale_h264_pkt_dispatcher.RemoveHandler(muxer);
  muxer->stop_current_job();
  muxer->sync_jobs_complete();
}

int EncodeHandler::attach_user_mdprocessor(VPUMDProcessor* p,
                                           MDAttributes* attr) {
  assert(p);
  int ret = h264_encoder_ref();
  if (ret) {
    h264_encoder_unref();
    return -1;
  }
  H264Encoder* vencoder = h264_encoder;
  video_encode_config& config = vencoder->encode_config;
  p->set_video_encoder(vencoder);
  p->init(config.width, config.height, config.stream_frame_rate);

  if (0 > p->set_attributes(attr)) {
    goto err;
  } else {
    mv_dispatcher.AddHandler(p);
  }
  return 0;
err:
  if (vencoder)
    h264_encoder_unref();
  return -1;
}

void EncodeHandler::detach_user_mdprocessor(VPUMDProcessor* p) {
  BaseEncoder* vencoder = p->get_video_encoder();
  if (vencoder && vencoder == h264_encoder)
    h264_encoder_unref();
  mv_dispatcher.RemoveHandler(p);
  // low_frame_rate_mode = false;
  if (cameramp4) {
    struct timeval tval = {0};
    cameramp4->set_low_frame_rate(false, tval);
  }
}

#define MUXER_ALLOC(CLASSTYPE, VAR, PCACHEMUXER, ENCODER_REF_FUNC,             \
                    ENCODER_UNREF_FUNC, URI, VIDEOENCODER, AUDIOENCODER)       \
  do {                                                                         \
    assert(!VAR);                                                              \
    if (0 > ENCODER_REF_FUNC()) {                                              \
      msg_ret = -1;                                                            \
      break;                                                                   \
    }                                                                          \
    CLASSTYPE* muxer = CLASSTYPE::create();                                    \
    if (!muxer) {                                                              \
      printf("create " #VAR " fail\n");                                        \
      msg_ret = -1;                                                            \
      break;                                                                   \
    }                                                                          \
    if (0 >                                                                    \
        muxer->init_uri(URI, VIDEOENCODER->encode_config.stream_frame_rate)) { \
      ENCODER_UNREF_FUNC();                                                    \
      delete muxer;                                                            \
      msg_ret = -1;                                                            \
      break;                                                                   \
    }                                                                          \
    muxer->set_encoder(VIDEOENCODER, AUDIOENCODER);                            \
    if (PCACHEMUXER) {                                                         \
      CameraCacheMuxer* cmuxer = *PCACHEMUXER;                                 \
      if (cmuxer) {                                                            \
        ENCODER_UNREF_FUNC();                                                  \
        h264aac_pkt_dispatcher.RemoveHandler(cmuxer);                          \
        cmuxer->flush_packet_list(muxer);                                      \
        *PCACHEMUXER = NULL;                                                   \
      }                                                                        \
    }                                                                          \
    if (0 > muxer->start_new_job(global_attr)) {                               \
      ENCODER_UNREF_FUNC();                                                    \
      delete muxer;                                                            \
      msg_ret = -1;                                                            \
    } else {                                                                   \
      VAR = muxer;                                                             \
    }                                                                          \
  } while (0);                                                                 \
  if (0 > msg_ret)                                                             \
  break

#define MUXER_FREE(VAR, ENCODER_UNREF_FUNC)    \
  if (VAR) {                                   \
    ENCODER_UNREF_FUNC();                      \
    VAR->stop_current_job();                   \
    delete VAR;                                \
    VAR = NULL;                                \
  }

int EncodeHandler::mp4_encode_start_job() {
  video_record_getfilename(filename, sizeof(filename), VIDEOFILE_PATH,
                           device_id, "mp4");
  if (0 > cameramp4->init_uri(filename,
                              h264_encoder->encode_config.stream_frame_rate)) {
    delete cameramp4;
    cameramp4 = NULL;
    return -1;
  }
  memset(&start_timestamp, 0, sizeof(struct timeval));
  if (0 == cameramp4->start_new_job(global_attr)) {
    // as mp4 no cache in list, should manage muxers strictly when start or stop
    h264aac_pkt_dispatcher.AddHandler(cameramp4);
    return 0;
  }
  return -1;
}

void EncodeHandler::mp4_encode_stop_current_job() {
  // as mp4 no cache in list, should manage muxers strictly when start or stop
  h264aac_pkt_dispatcher.RemoveHandler(cameramp4);
  cameramp4->stop_current_job();
  cameramp4->reset_lbr_time();
}

static void do_notify(void* arg1, void* arg2) {
  std::mutex* mtx = (std::mutex*)arg1;
  std::condition_variable* cond = (std::condition_variable*)arg2;
  if (mtx) {
    assert(cond);
    std::unique_lock<std::mutex> lk(*mtx);
    cond->notify_one();
  }
}

static const char* file_cache_path = "/mnt/sdcard/.h264cache";

// TODO, move msg_queue handler to thread which create video
int EncodeHandler::h264_encode_process(void* virt,
                                       int fd,
                                       void* buf_hnd,
                                       size_t size,
                                       struct timeval& time_val) {
#ifdef TRACE_FUNCTION
  printf("usb_type: %d, [%d x %d] ", usb_type, common_config.width,
         common_config.height);
  PRINTF_FUNC_LINE;
#endif

  int recorded_sec = 0;
  bool del_recoder_mp4 = false;
  bool video_save = false;
  Message<EncoderMessageID>* msg = NULL;
  assert(msg_queue);
  if (!msg_queue->Empty()) {
    int msg_ret = 0;
    char* uri = NULL;
    CameraCacheMuxer** pcache_muxer = NULL;
#ifdef DEBUG
    if (!process_pid) {
      process_pid = pthread_self();
    }
    if (pthread_self() != process_pid) {
      printf("warning: h264_encode_process run in different thread!\n");
    }
#endif
    msg = msg_queue->PopFront();
#ifdef TRACE_FUNCTION
    printf("[%d x %d] ", common_config.width, common_config.height);
    PRINTF("video msg id: %d\n\n", msg->GetID());
#endif
    EncoderMessageID msg_id = msg->GetID();
    switch (msg_id) {
      case START_ENCODE:
        if (0 > h264aac_encoder_ref())
          msg_ret = -1;
        break;
      case STOP_ENCODE:
        h264aac_encoder_unref();
        do_notify(msg->arg2, msg->arg3);
        break;
      case START_ENCODE2:
        if (0 > scale_h264_encoder_ref())
          msg_ret = -1;
        break;
      case STOP_ENCODE2:
        scale_h264_encoder_unref();
        do_notify(msg->arg2, msg->arg3);
        break;
      case START_CACHE:
        if (((int)msg->arg1) > 2) {
          uri = (char*)file_cache_path;
        }
        assert(!pcache_muxer);
        MUXER_ALLOC(CameraCacheMuxer, cameracache, pcache_muxer,
                    h264aac_encoder_ref, h264aac_encoder_unref, uri,
                    h264_encoder, aac_encoder);
        h264aac_pkt_dispatcher.AddHandler(cameracache);
        break;
      case STOP_CACHE:
        h264aac_pkt_dispatcher.RemoveHandler(cameracache);
        MUXER_FREE(cameracache, h264aac_encoder_unref);
        break;
      case START_RECODEMP4:
        assert(!cameramp4);
        video_record_getfilename(filename, sizeof(filename), VIDEOFILE_PATH,
                                 device_id, "mp4");
        memset(&start_timestamp, 0, sizeof(struct timeval));
        pcache_muxer = &cameracache;
        MUXER_ALLOC(CameraMp4, cameramp4, pcache_muxer, h264aac_encoder_ref,
                    h264aac_encoder_unref, filename, h264_encoder, aac_encoder);
        // cameramp4->set_low_frame_rate(low_frame_rate_mode, time_val);
        h264aac_pkt_dispatcher.AddHandler(cameramp4);
        break;
      case STOP_RECODEMP4:
        del_recoder_mp4 = (bool)msg->arg1;
        if (!cameramp4) {
          if (del_recoder_mp4) {
            do_notify(msg->arg2, msg->arg3);
          }
          break;
        }
        if (del_recoder_mp4) {
          h264aac_encoder_unref();
        }
        mp4_encode_stop_current_job();
        if (del_recoder_mp4) {
          // delete the mp4 muxer
          cameramp4->sync_jobs_complete();
          delete cameramp4;
          cameramp4 = NULL;
          do_notify(msg->arg2, msg->arg3);
          PRINTF("del cameramp4 done.\n");
        } else if (!del_recoder_mp4 && (bool)msg->arg2) {
          // restart immediately
          if (0 > mp4_encode_start_job()) {
            msg_ret = -1;
            break;
          }
          h264_encoder->set_force_encode_idr(true);
        }
        break;
      case START_TSTRANSFER:
        assert(!pcache_muxer);
        MUXER_ALLOC(CameraTs, camerats, pcache_muxer, scale_h264_encoder_ref,
                    scale_h264_encoder_unref, (char*)msg->arg1,
                    scale_h264_encoder, NULL);
        scale_h264_pkt_dispatcher.AddHandler(camerats);
        break;
      case STOP_TSTRANSFER:
        scale_h264_pkt_dispatcher.RemoveHandler(camerats);
        MUXER_FREE(camerats, scale_h264_encoder_unref);
        do_notify(msg->arg2, msg->arg3);
        break;
      case SAVE_MP4_IMMEDIATELY:
        video_save = true;
        break;
      case ATTACH_USER_MUXER:
      case ATTACH_USER_MUXER2: {
        CameraMuxer* muxer = (CameraMuxer*)msg->arg1;
        char* uri = (char*)msg->arg2;
        uint32_t val = (uint32_t)msg->arg3;
        bool need_video = (val & 1) ? true : false;
        bool need_audio = (val & 2) ? true : false;
        attach_user_muxer(muxer, uri, need_video, need_audio,
                          msg_id == ATTACH_USER_MUXER ? H264_ENCODER_ID
                                                      : SCALED_H264_ENCODER_ID);
        break;
      }
      case DETACH_USER_MUXER:
      case DETACH_USER_MUXER2: {
        CameraMuxer* muxer = (CameraMuxer*)msg->arg1;
        detach_user_muxer(muxer, msg_id == DETACH_USER_MUXER
                                     ? H264_ENCODER_ID
                                     : SCALED_H264_ENCODER_ID);
        do_notify(msg->arg2, msg->arg3);
        break;
      }
      case ATTACH_USER_MDP: {
        VPUMDProcessor* p = (VPUMDProcessor*)msg->arg1;
        MDAttributes* attr = (MDAttributes*)msg->arg2;
        attach_user_mdprocessor(p, attr);
        break;
      }
      case DETACH_USER_MDP: {
        VPUMDProcessor* p = (VPUMDProcessor*)msg->arg1;
        detach_user_mdprocessor(p);
        do_notify(msg->arg2, msg->arg3);
        break;
      }
      case RECORD_RATE_CHANGE: {
        VPUMDProcessor* p = (VPUMDProcessor*)msg->arg1;
        bool low_frame_rate_mode = (bool)msg->arg2;
        if (!low_frame_rate_mode) {
          if (cameramp4 && cameramp4->is_low_frame_rate())
            video_save = true;  // low->normal, save file immediately
        } else {
          //...
        }
        if (cameramp4) {
          cameramp4->set_low_frame_rate(low_frame_rate_mode, time_val);
        }
        if (!mv_dispatcher.Empty())
          p->set_low_frame_rate(low_frame_rate_mode ? 1 : 0);
        break;
      }
      case GEN_IDR_FRAME: {
        int encoder_id = (int)msg->arg1;
        H264Encoder* encoder =
            (encoder_id == H264_ENCODER_ID) ? h264_encoder : scale_h264_encoder;
        if (encoder)
          encoder->set_force_encode_idr(true);
        break;
      }
      default:
          // assert(0);
          ;
    }
    if (START_TSTRANSFER == msg->GetID())
      free(msg->arg1);  // todo, use static char array?
#ifdef TRACE_FUNCTION
    printf("[%d x %d] ", common_config.width, common_config.height);
    PRINTF("msg_ret : %d\n\n", msg_ret);
#endif
    if (msg_ret < 0) {
      delete msg;
      assert(0);
      return msg_ret;
    }
  }
  if (msg)
    delete msg;

  // fd < 0 is normal for usb when pulling out usb connection
  if (fd < 0)
    return 0;

  if (h264_encoder) {
    struct timeval camstamp = time_val;
    int64_t usec = 0;
    EncodedPacket* pkt = NULL;
    int encode_ret = 0;

    if (start_timestamp.tv_sec != 0 || start_timestamp.tv_usec != 0) {
      usec = (camstamp.tv_sec - start_timestamp.tv_sec) * 1000000 +
             (camstamp.tv_usec - start_timestamp.tv_usec);
    } else {
      start_timestamp = time_val;
#ifdef STRESS_TEST
      av_log(NULL, AV_LOG_ERROR,
             "usb_type : %d, 111 camstamp[%d , %d], start_timestamp[%d , %d]\n",
             usb_type, camstamp.tv_sec, camstamp.tv_usec,
             start_timestamp.tv_sec, start_timestamp.tv_usec);
#endif
    }
    recorded_sec = usec / 1000000;

    if (cameramp4 && record_time_notify) {
      record_time_notify(this, recorded_sec);
    }

    pkt = new EncodedPacket();
    if (!pkt) {
      printf("alloc EncodedPacket failed\n");
      assert(0);
      return -1;
    }
    DataBuffer_t input_buf;
    input_buf.vir_addr = (uint8_t*)virt;
    input_buf.phy_fd = fd;
    input_buf.handle = buf_hnd;
    input_buf.buf_size = size;

    pkt->is_phy_buf = true;
    if (usb_type == USB_TYPE_H264) {
      encode_ret =
          h264_encoder->encode_encoded_data(&input_buf, &pkt->av_pkt, true);
    } else {
#if defined(STRESS_TEST) || defined(MUL_ENCODERS_DEBUG)
      int64_t t1 = av_gettime_relative();
      int64_t interval_coming = 0;
      int level = AV_LOG_WARNING;
      if (0 != t_pre_data_coming) {
        interval_coming = t1 - t_pre_data_coming;
        if (interval_coming * common_config.stream_frame_rate > 1000000) {
          level = AV_LOG_ERROR;
        }
      }
      av_log(NULL, level,
             "~(video-%d)~before encode_video_frame, now time <%lld ms>, data "
             "interval: <%lld> ms\n",
             device_id, t1 / 1000, interval_coming / 1000);
      t_pre_data_coming = t1;
#endif

#ifdef USE_WATERMARK
      if (parameter_get_video_mark() && watermark_info->type > 0) {
        h264_encode_control(MPP_ENC_SET_OSD_DATA_CFG,
                            &watermark_info->mpp_osd_data);
      }
#endif
      encode_ret = h264_encoder->encode_video_frame(
          &input_buf, &output_buf, &mv_buf, &pkt->av_pkt, true);
#if defined(STRESS_TEST) || defined(MUL_ENCODERS_DEBUG)
      av_log(
          NULL, level,
          "~(video-%d)~Done. Encoding frame[%d x %d] video takes <%lld> ms\n",
          device_id, common_config.width, common_config.height,
          (av_gettime_relative() - t1) / 1000);
#endif
    }
    if (0 == encode_ret) {
      pkt->type = VIDEO_PACKET;
      pkt->time_val = camstamp;
#ifdef STRESS_TEST
      if (cameramp4 && cameramp4->is_low_frame_rate())
        av_log(NULL, AV_LOG_ERROR,
               "2222 camstamp[%d , %d], start_timestamp[%d , %d]\n",
               camstamp.tv_sec, camstamp.tv_usec, start_timestamp.tv_sec,
               start_timestamp.tv_usec);
#endif
#ifdef STRESS_TEST
      AutoPrintInterval interval("Dispatch to muxer");
// av_log(NULL, AV_LOG_ERROR, "!! before dispatch\n");
#endif
      h264aac_pkt_dispatcher.Dispatch(pkt);
#ifdef STRESS_TEST
// av_log(NULL, AV_LOG_ERROR, "!! after Dispatch\n");
#endif

      if (!(pkt->av_pkt.flags & AV_PKT_FLAG_KEY))
        mv_dispatcher.Dispatch(mv_buf.vir_addr, virt, size);

      if (cameramp4 && cameramp4->is_low_frame_rate()) {
#ifdef STRESS_TEST
        av_log(NULL, AV_LOG_ERROR, ", pkt lbr_time_val[%d , %d]\n",
               pkt->lbr_time_val.tv_sec, pkt->lbr_time_val.tv_usec);
#endif
        usec = (pkt->lbr_time_val.tv_sec - start_timestamp.tv_sec) * 1000000 +
               (pkt->lbr_time_val.tv_usec - start_timestamp.tv_usec);
        recorded_sec = usec / 1000000;
#ifdef STRESS_TEST
        PRINTF("low bit rate, recorded_sec: %d\n", recorded_sec);
#endif
      }
    }
    pkt->unref();
  }

  if (scale_h264_encoder && usb_type != USB_TYPE_H264) {
    EncodedPacket* pkt =
        scale_h264_encoder->encode_one_frame(fd, &common_config);
    if (pkt) {
      pkt->time_val = time_val;
      scale_h264_pkt_dispatcher.Dispatch(pkt);
      // camerats->push_packet(pkt);
      pkt->unref();
    }
  }

  if (cameramp4 && !del_recoder_mp4) {
    if (recorded_sec >= parameter_get_recodetime() || video_save) {
      // h264aac_encoder_unref();
      mp4_encode_stop_current_job();
      // if (0 > h264aac_encoder_ref()) {
      //  return -1;
      //}
      // cameramp4->set_encoder(h264_encoder, aac_encoder);
      // restart immediately
      if (0 > mp4_encode_start_job()) {
        return -1;
      }
      h264_encoder->set_force_encode_idr(true);
    }
  }

#ifdef TRACE_FUNCTION
  printf("usb_type: %d, [%d x %d] ", usb_type, common_config.width,
         common_config.height);
  PRINTF_FUNC_OUT;
#endif
  return 0;
}

int EncodeHandler::h264_encode_control(int cmd, void* param) {
  int ret = 0;
  if (!h264_encoder) {
    // h264_encoder is not ready
    return 0;
  }
  ret = h264_encoder->encode_control(cmd, param);
  if (ret != 0) {
    printf("EncodeHandler::h264_encode_control cmd 0x%08x param %p", cmd,
           param);
    return ret;
  }

  return 0;
}
