// by wangh
#include <iostream>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}
#include "../encoder/base_encoder.h"
#include "cache/camera_cache_muxer.h"
#include "camera_muxer.h"

static AVStream* add_output_stream(AVFormatContext* output_format_context,
                                   AVStream* input_stream) {
  AVCodecContext* output_codec_context = NULL;
  AVStream* output_stream = NULL;

  output_stream =
      avformat_new_stream(output_format_context, input_stream->codec->codec);
  if (!output_stream) {
    printf("Call avformat_new_stream function failed\n");
    return NULL;
  }
  output_stream->time_base = input_stream->codec->time_base;
  if (0 > avcodec_copy_context(output_stream->codec, input_stream->codec)) {
    fprintf(
        stderr,
        "Failed to copy context from input to output stream codec context\n");
  }
  output_stream->codec->codec_tag = 0;
  output_codec_context = output_stream->codec;

  if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER) {
    output_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }
  return output_stream;
}

int CameraMuxer::init_muxer_processor(MuxProcessor* process) {
  int ret = 0;
  AVFormatContext* oc = NULL;
  AVStream* video_stream = v_encoder->get_avstream();
  AVStream* audio_stream = a_encoder ? a_encoder->get_avstream() : NULL;
  avformat_network_init();
  // av_log_set_level(AV_LOG_TRACE);
  /* allocate the output media context */
  ret = avformat_alloc_output_context2(&oc, NULL, getOutFormat(), NULL);
  if (!oc) {
    fprintf(stderr, "avformat_alloc_output_context2 failed for %s, ret: %d\n",
            getOutFormat(), ret);
    return -1;
  }
  process->av_format_context = oc;
  if (video_stream) {
    AVStream* st = add_output_stream(oc, video_stream);
    if (!st) {
      return -1;
    }
    process->video_avstream = st;
  }
  if (audio_stream) {
    AVStream* st = add_output_stream(oc, audio_stream);
    if (!st) {
      return -1;
    }
    process->audio_avstream = st;
  }
  return 0;
}

void CameraMuxer::deinit_muxer_processor(AVFormatContext* oc) {
  if (oc) {
    for (unsigned int i = 0; i < oc->nb_streams; i++) {
      AVStream* st = oc->streams[i];
      if (st) {
        avcodec_close(st->codec);
      }
    }
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
      /* Close the output file. */
      avio_closep(&oc->pb);
    }
    avformat_free_context(oc);
  }
}

int CameraMuxer::muxer_write_header(AVFormatContext* oc, char* url) {
  int av_ret = 0;
  av_dump_format(oc, 0, url, 1);
  if (!(oc->oformat->flags & AVFMT_NOFILE)) {
    av_ret = avio_open(
        &oc->pb, url,
        AVIO_FLAG_WRITE | (direct_piece ? AVIO_FLAG_RK_DIRECT_PIECE : 0));
    if (av_ret < 0) {
      char str[AV_ERROR_MAX_STRING_SIZE] = {0};
      av_strerror(av_ret, str, AV_ERROR_MAX_STRING_SIZE);
      fprintf(stderr, "Could not open '%s': %s\n", url, str);
      return -1;
    }
  } else {
    if (url)
      strncpy(oc->filename, url, strlen(url));
  }
  av_ret = avformat_write_header(oc, NULL);
  if (av_ret < 0) {
    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(av_ret, str, AV_ERROR_MAX_STRING_SIZE);
    fprintf(stderr, "Error occurred when opening output file: %s\n", str);
    return -1;
  }
  return 0;
}

static int write_frame(AVFormatContext* fmt_ctx,
                       const AVRational* time_base,
                       AVStream* st,
                       AVPacket* pkt) {
  /* rescale output packet timestamp values from codec to stream timebase */
  av_packet_rescale_ts(pkt, *time_base, st->time_base);
  pkt->stream_index = st->index;

  /* Write the compressed frame to the media file. */
  //    log_packet(fmt_ctx, pkt);
  // return av_interleaved_write_frame(fmt_ctx, pkt);
  return av_write_frame(fmt_ctx, pkt);
}

int CameraMuxer::muxer_write_packet(AVFormatContext* ctx,
                                    AVStream* st,
                                    AVPacket* pkt) {
  // if (pkt->flags & AV_PKT_FLAG_KEY) {
  //    av_log(NULL, AV_LOG_ERROR, "write key packet~~~\n");
  //}
  AVCodecContext* c = st->codec;
  int ret = write_frame(ctx, &c->time_base, st, pkt);
  if (ret < 0) {
    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(ret, str, AV_ERROR_MAX_STRING_SIZE);
    fprintf(stderr, "Error while writing frame: %s\n", str);
  }
  return ret;
}

int CameraMuxer::muxer_write_tailer(AVFormatContext* oc) {
  int av_ret = av_write_trailer(oc);
  if (av_ret < 0) {
    // need close the file?? or retry outside TODO
    fprintf(stderr, "muxer_write_tailer failed!!!\n");
    return -1;
  }
  if (oc) {
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
      /* Close the output file. */
      avio_closep(&oc->pb);
    }
  }
  return 0;
}

static void dump_pkt_buf(AVPacket* pkt, int line) {
  int i = pkt->side_data_elems;
  printf("line: %d, --- pkt buf: %p\n    data: %p,size:%d\n", line, pkt->buf,
         pkt->data, pkt->size);
  printf("    side_data_elems: %d;[ ", i);
  while (i--) {
    printf("%p ", pkt->side_data[pkt->side_data_elems - i]);
  }
  printf("]\n\n");
}

// write packet to file and free the pkt
int CameraMuxer::muxer_write_free_packet(MuxProcessor* process,
                                         EncodedPacket* pkt) {
  int ret = 0;
  AVPacket avpkt;
  if (!process->got_first_video && pkt->type == VIDEO_PACKET) {
    process->v_first_timeval = pkt->time_val;
    process->got_first_video = true;
  } else if (!process->got_first_audio && pkt->type == AUDIO_PACKET) {
    process->audio_first_index = pkt->audio_index;
    process->got_first_audio = true;
  }

  if (pkt->is_phy_buf) {
    assert(!pkt->av_pkt.buf);
    avpkt = pkt->av_pkt;
  } else {
    av_init_packet(&avpkt);
    assert(pkt->av_pkt.buf);
    if (0 != av_packet_ref(&avpkt, &pkt->av_pkt)) {
      fprintf(stderr, "av_packet_ref failed, %s:%d\n", __func__, __LINE__);
      goto fail;
    }
  }
  // got packet, write it to file
  if (pkt->type == VIDEO_PACKET) {
    video_encode_config& econfig = v_encoder->encode_config;
    struct timeval tval = {0};
    if (pkt->lbr_time_val.tv_sec != 0 || pkt->lbr_time_val.tv_usec != 0) {
      tval = pkt->lbr_time_val;
      PRINTF_FUNC_LINE;
    } else {
      tval = pkt->time_val;
      // PRINTF_FUNC_LINE;
    }
    int64_t usec = (tval.tv_sec - process->v_first_timeval.tv_sec) * 1000000LL +
                   (tval.tv_usec - process->v_first_timeval.tv_usec);
    // printf("usec: %lld, %d:%ld - %d:%ld\n", usec, tval.tv_sec, tval.tv_usec,
    //       process->v_first_timeval.tv_sec, process->v_first_timeval.tv_usec);
    // assert(usec >= 0);
    if (usec < 0) {
#ifdef DEBUG
      printf("usec: %lld, %ld:%d - %ld:%d\n", usec, tval.tv_sec, tval.tv_usec,
             process->v_first_timeval.tv_sec, process->v_first_timeval.tv_usec);
#endif
      usec = 0;
    }
    int64_t pts = usec * (int64_t)econfig.stream_frame_rate / 1000000LL;
    if (pts <= process->pre_video_pts) {
      pts = process->pre_video_pts + 1;
    }
#ifdef DEBUG
    if (pts - process->pre_video_pts > 5) {
      printf(
          "input buffer come late for <%lld> frame. pts: %lld, "
          "pre_video_pts: %lld\n\n",
          pts - process->pre_video_pts - 1, pts, process->pre_video_pts);
    }
#endif
    avpkt.dts = avpkt.pts = pts;
    process->pre_video_pts = pts;
    assert(pts >= 0);
    ret = muxer_write_packet(process->av_format_context,
                             process->video_avstream, &avpkt);
  } else if (pkt->type == AUDIO_PACKET) {
    assert(a_encoder);
    video_encode_config& econfig = a_encoder->encode_config;
    avpkt.pts = (pkt->audio_index - process->audio_first_index) *
                (int64_t)econfig.input_nb_samples;
    avpkt.pts -= 2 * econfig.input_nb_samples;
    avpkt.dts = avpkt.pts;
    ret = muxer_write_packet(process->av_format_context,
                             process->audio_avstream, &avpkt);
  } else {
    assert(0);
  }
  av_packet_unref(&avpkt);
fail:
  pkt->unref();
  return ret;
}

static int muxer_init(CameraMuxer* muxer,
                      char** puri,
                      MuxProcessor& processor) {
  CameraCacheMuxer* cacheMuxer = NULL;
  char* uri = *puri;
  if (0 != muxer->init_muxer_processor(&processor)) {
    return -1;
  }
  printf("uri: %s\n", uri);
#ifdef FFMPEG_DEBUG
  assert(processor.video_avstream);
#endif
  if (0 != muxer->muxer_write_header(processor.av_format_context, uri)) {
    return -1;
  }
  free(uri);
  *puri = NULL;
  cacheMuxer = muxer->get_prior_seller();
  if (cacheMuxer) {
    EncodedPacket* pkt = NULL;
    while (pkt = cacheMuxer->pop_packet()) {
      // flush all, even exit msg coming
      if (pkt == &muxer->empty_packet)
        continue;
      int ret = muxer->muxer_write_free_packet(&processor, pkt);
    }
    delete cacheMuxer;
  }
  return 0;
}

static void* muxer_process(void* arg) {
  CameraMuxer* muxer = (CameraMuxer*)arg;
  std::mutex* exit_mtx = NULL;
  std::condition_variable* exit_cond = NULL;
  Message<CameraMuxer::MUXER_MSG_ID>* msg = NULL;
  std::list<EncodedPacket*>* packet_list = NULL;
  char* uri = NULL;
  int ret = 0;
  MuxProcessor processor = {0};
  processor.pre_video_pts = -1;
  processor.got_first_audio = false;
  processor.got_first_video = false;

  PRINTF("%s run muxer_process\n", muxer->class_name);
  while (muxer->write_enable) {
    EncodedPacket* pkt = NULL;
    msg = muxer->getMessage();
    if (msg) {
      CameraMuxer::MUXER_MSG_ID id = msg->GetID();
      PRINTF("muxer msg id: %d\n\n", id);
      switch (id) {
        case CameraMuxer::MUXER_START:
          uri = (char*)msg->arg1;
          if (0 > muxer_init(muxer, &uri, processor)) {
            muxer->write_enable = false;
            goto thread_end;
          }
          break;
        case CameraMuxer::MUXER_NORMAL_EXIT:
        case CameraMuxer::MUXER_FLUSH_EXIT:
          muxer->set_packet_list(NULL, &packet_list);
          exit_mtx = (std::mutex*)msg->arg1;
          exit_cond = (std::condition_variable*)msg->arg2;
          assert(packet_list);
          if (id == CameraMuxer::MUXER_NORMAL_EXIT) {
            std::unique_lock<std::mutex> lk(*exit_mtx);
            exit_cond->notify_one();
            exit_mtx = NULL;
            exit_cond = NULL;
          }
          goto loop_end;
        case CameraMuxer::MUXER_IMMEDIATE_EXIT: {
          std::mutex* pmtx = (std::mutex*)msg->arg1;
          std::condition_variable* pcond = (std::condition_variable*)msg->arg2;
          std::unique_lock<std::mutex> lk(*pmtx);
          pcond->notify_one();
        }
          goto loop_end;
        default:
          assert(0);
          break;
      }
      delete msg;
      msg = NULL;
    }
    pkt = muxer->wait_pop_packet();
    if (pkt == &muxer->empty_packet)
      continue;
    ret = muxer->muxer_write_free_packet(&processor, pkt);
  }
loop_end:
  if (packet_list) {
    while (!packet_list->empty()) {
      EncodedPacket* pkt = packet_list->front();
      packet_list->pop_front();
      if (pkt == &muxer->empty_packet)
        continue;
      ret = muxer->muxer_write_free_packet(&processor, pkt);
    }
    delete packet_list;
  }
  if (processor.av_format_context &&
      0 != muxer->muxer_write_tailer(processor.av_format_context)) {
    // how fix this!?
  }
  if (exit_mtx) {
    // MUXER_FLUSH_EXIT, wait for all packets complete
    std::unique_lock<std::mutex> lk(*exit_mtx);
    exit_cond->notify_one();
  }

thread_end:
  if (uri) {
    free(uri);
  }
  if (msg) {
    delete msg;
  }
  if (processor.av_format_context) {
    muxer->deinit_muxer_processor(processor.av_format_context);
  }
  muxer->remove_wtid(pthread_self());
  pthread_detach(pthread_self());
  PRINTF("exit muxer thread 0x%x\n", pthread_self());
  pthread_exit(NULL);
}

CameraMuxer::CameraMuxer() {
  v_encoder = nullptr;
  a_encoder = nullptr;
  write_tid = 0;
  write_enable = false;
  video_packet_num = 0;
  packet_list = NULL;
  run_func = muxer_process;
  format[0] = 0;
  direct_piece = false;
  video_sample_rate = 0;
  exit_id = MUXER_FLUSH_EXIT;
  max_video_num = 0;
#if 0  // def DEBUG
    packet_total_size = 0;
    snprintf(class_name, sizeof(class_name), "CameraMuxer");
#endif
  empty_packet.resetRefcount();
  av_init_packet(&empty_packet.av_pkt);
  prior_seller = NULL;
  no_async = false;
  memset(&global_processor, 0, sizeof(global_processor));
}

CameraMuxer::~CameraMuxer() {
  printf("func: %s, delete %p\n", __func__, this);
  write_enable = false;
  assert(!global_processor.av_format_context);
  sync_jobs_complete();
  free_packet_list();
  // assert(msg_queue.Empty());
  msg_queue.clear();
  CameraCacheMuxer* cacheMuxer = get_prior_seller();
  if (cacheMuxer) {
    EncodedPacket* pkt = NULL;
    while (pkt = cacheMuxer->pop_packet()) {
      // flush all, even exit msg coming
      if (pkt == &empty_packet)
        continue;
      pkt->unref();
    }
    delete cacheMuxer;
  }
}

int CameraMuxer::init() {
  run_func = muxer_process;
  return 0;
}

void CameraMuxer::free_packet_list() {
  std::unique_lock<std::mutex> lk(packet_list_mutex);
  if (packet_list) {
    if (!packet_list->empty()) {
      int size = packet_list->size();
      printf("in %s, remain %d packets\n", __func__, size);
      for (int i = 0; i < size; i++) {
        EncodedPacket* pkt = packet_list->front();
        packet_list->pop_front();
        if (pkt != &empty_packet) {
          pkt->unref();
#if 0  // def DEBUG
                    packet_total_size -= pkt->av_pkt.size;
                    printf("packet_total_size: %d\n", packet_total_size);
#endif
        }
      }
    }
    delete packet_list;
    packet_list = NULL;
  }
}

void CameraMuxer::push_packet(EncodedPacket* pkt) {
  std::unique_lock<std::mutex> lk(packet_list_mutex);
  if (max_video_num > 0) {
    if (!write_enable)
      return;
    if (video_packet_num > max_video_num) {
      int i = 0;
#ifdef DEBUG
      if (strlen(format)) {
        // too many packets, write file is so slow
        printf(
            "write packet is so slow, %s, paket_list size: %d, video num: %d\n",
            class_name, packet_list->size(), video_packet_num);
      }
#endif
      // we need drop one video packet at front, util the next video packet
      while (!packet_list->empty()) {
        // printf("video->packet_list->size: %d\n", video->packet_list->size());
        EncodedPacket* front_pkt = packet_list->front();
        PacketType type = front_pkt->type;
        if (type == VIDEO_PACKET &&
            (front_pkt->av_pkt.flags & AV_PKT_FLAG_KEY)) {
          i++;
        }
        if (i < 2) {
          // printf("type: %d, front_pkt: %p\n", type, front_pkt);
          packet_list->pop_front();
          if (front_pkt->type == VIDEO_PACKET) {
            video_packet_num--;
#if 0  // def DEBUG
                        packet_total_size -= front_pkt->av_pkt.size;
                        printf("packet_total_size: %d\n", packet_total_size);
#endif
          }
          front_pkt->unref();
        } else {
          break;
        }
      }
    }
    pkt->ref();  // add one ref in muxer's packet_list
    if (pkt->type == VIDEO_PACKET) {
      video_packet_num++;
#if 0  // def DEBUG
            packet_total_size += pkt->av_pkt.size;
            printf("packet_total_size: %d\n", packet_total_size);
#endif
    }
    packet_list->push_back(pkt);
    // printf("video->video_packet_num: %d, video->packet_list->size: %d, pkt:
    // %p\n",
    //       video->video_packet_num, video->packet_list->size(), pkt);
    packet_list_cond.notify_one();
  } else {
    pkt->ref();  // add one ref
    int ret = muxer_write_free_packet(&global_processor, pkt);
  }
}

EncodedPacket* CameraMuxer::wait_pop_packet() {
  std::unique_lock<std::mutex> lk(packet_list_mutex);
  EncodedPacket* pkt = NULL;
  assert(packet_list);
  if (packet_list->empty()) {
    packet_list_cond.wait(lk);
  }
  pkt = packet_list->front();
  packet_list->pop_front();
  if (pkt->type == VIDEO_PACKET) {
    video_packet_num--;
#if 0  // def DEBUG
        packet_total_size -= pkt->av_pkt.size;
        printf("packet_total_size: %d\n", packet_total_size);
#endif
  }
  return pkt;
}

Message<CameraMuxer::MUXER_MSG_ID>* CameraMuxer::getMessage() {
  if (msg_queue.Empty()) {
    return NULL;
  } else {
    return msg_queue.PopFront();
  }
}

// called before start a new process thread
int CameraMuxer::init_uri(char* uri, int rate) {
  video_sample_rate = rate;
  assert(video_packet_num == 0);
  assert(msg_queue.Empty());
  if (!no_async) {
    if (!packet_list) {
      packet_list = new std::list<EncodedPacket*>();
      if (!packet_list) {
        printf("alloc packet_list failed, no memory!\n");
        return -1;
      }
    } else {
      assert(packet_list->empty());
    }
  }
  if (uri) {  // if uri is not null, means we will create thread to do muxer
    char* uri_str = (char*)malloc(strlen(uri) + 1);
    if (!uri_str) {
      std::cerr << "malloc 128B failed, no memory!" << std::endl;
      return -1;
    }
    sprintf(uri_str, "%s", uri);
    Message<MUXER_MSG_ID>* msg =
        Message<MUXER_MSG_ID>::create(MUXER_START, HIGH_PRIORITY);
    if (nullptr == msg) {
      printf("alloc Message failed, no memory!\n");
      free(uri_str);
      return -1;
    }
    msg->arg1 = uri_str;
    msg_queue.Push(msg);
  }
  max_video_num = no_async ? -1 : rate * 2;  // 2 seconds
  return 0;
}

void CameraMuxer::set_packet_list(std::list<EncodedPacket*>* list,
                                  std::list<EncodedPacket*>** backup) {
  std::unique_lock<std::mutex> lk(packet_list_mutex);
  if (backup) {
    *backup = packet_list;
  }
  packet_list = list;
  video_packet_num = 0;
}

int CameraMuxer::start_new_job(pthread_attr_t* attr) {
  std::unique_lock<std::mutex> lk(w_tidlist_mutex);
  // already pushed start msg
  assert(!msg_queue.Empty() && write_tid == 0);  // should not call too wayward

  if (no_async) {
    Message<MUXER_MSG_ID>* msg = getMessage();
    char* uri = NULL;
    assert(msg && msg->GetID() == MUXER_START);
    global_processor.pre_video_pts = -1;
    global_processor.got_first_audio = false;
    global_processor.got_first_video = false;
    uri = (char*)msg->arg1;
    delete msg;
    if (0 > muxer_init(this, &uri, global_processor)) {
      if (uri) {
        free(uri);
      }
      deinit_muxer_processor(global_processor.av_format_context);
      global_processor.av_format_context = NULL;
      return -1;
    }
  } else {
    pthread_t tid = -1;
    if (pthread_create(&tid, attr, run_func, (void*)this)) {
      printf("%s create writefile pthread err\n", __func__);
      return -1;
    }
    write_enable = true;
    PRINTF("created muxer tid: 0x%x\n", tid);
    write_tid = tid;
    w_tid_list.push_back(write_tid);
  }

  return 0;
}

void CameraMuxer::remove_wtid(pthread_t tid) {
  std::unique_lock<std::mutex> lk(w_tidlist_mutex);
  w_tid_list.remove(tid);
}

void CameraMuxer::stop_current_job() {
  {
    std::unique_lock<std::mutex> lk(w_tidlist_mutex);
    if (!write_tid) {
      if (no_async) {
        AVFormatContext* av_format_context = global_processor.av_format_context;
        if (av_format_context) {
          if (0 != muxer_write_tailer(av_format_context)) {
            // how fix this
          }
          deinit_muxer_processor(av_format_context);
        }
        memset(&global_processor, 0, sizeof(global_processor));
        global_processor_cond.notify_one();
      }
      return;
    }
  }
  PRINTF("exit_id : %d\n", exit_id);
  Message<MUXER_MSG_ID>* msg =
      Message<MUXER_MSG_ID>::create(exit_id, MID_PRIORITY);
  if (nullptr == msg) {
    printf("alloc Message failed, no memory!\n");
    return;
  }
  std::mutex mtx;
  std::condition_variable cond;
  msg->arg1 = &mtx;
  msg->arg2 = &cond;
  {
    {
      std::unique_lock<std::mutex> lk(w_tidlist_mutex);
      write_tid = 0;
    }
    {
      std::unique_lock<std::mutex> _lk(mtx);
      msg_queue.Push(msg);
      packet_list_mutex.lock();
      packet_list->push_back(&empty_packet);
      packet_list_cond.notify_one();
      packet_list_mutex.unlock();
      cond.wait(_lk);
    }
  }
}

void CameraMuxer::sync_jobs_complete() {
  {
    std::unique_lock<std::mutex> lk(w_tidlist_mutex);
    if (no_async) {
      if (global_processor.av_format_context) {
        global_processor_cond.wait(lk);
      }
      return;
    }
  }
  while (1) {
    w_tidlist_mutex.lock();
    if (!w_tid_list.empty()) {
      pthread_t tid = 0;
      tid = w_tid_list.front();
      PRINTF("b, tid: 0x%x\n", tid);
      // video->wf_tid_list->pop_front();
      w_tidlist_mutex.unlock();
      if (tid != 0) {
        pthread_join(tid, NULL);
      }
      PRINTF("video write thread num: %d\n", w_tid_list.size());
    } else {
      w_tidlist_mutex.unlock();
      PRINTF("video write thread num 00\n");
      break;
    }
  }
}
