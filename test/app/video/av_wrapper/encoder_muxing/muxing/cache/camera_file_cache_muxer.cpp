#include "camera_file_cache_muxer.h"

#if 0
static void* fileCacheProcess(void *arg) {
    CameraFileCacheMuxer *muxer = (CameraFileCacheMuxer*)arg;
    assert(muxer);
    int file_fd = -1;
    std::mutex *exit_mtx = NULL;
    std::condition_variable *exit_cond = NULL;
    Message<CameraMuxer::MUXER_MSG_ID> *msg = NULL;
    std::list<EncodedPacket*> *packet_list = NULL;
    char *uri = NULL;
    int ret = 0;
#ifdef DEBUG
    assert(encoder);
    printf("%s run muxer_process\n", muxer->class_name);
#endif
    while (muxer->write_enable) {
        EncodedPacket *pkt = NULL;
        msg = muxer->getMessage();
        if (msg) {
            CameraMuxer::MUXER_MSG_ID id = msg->GetID();
            printf("%s msg id: %d\n\n", muxer->class_name, id);
            switch (id) {
            case CameraMuxer::MUXER_START:
                uri = (char*)msg->arg1;
                if (NO_ERROR != init_muxer(&processor, encoder->getEncoderFmtCtx(),
                                           muxer->getOutFormat())) {
                    goto thread_end;
                }
                printf("uri: %s\n", uri);
                assert(video_avstream);
                assert(audio_avstream);
                if (NO_ERROR != muxer_write_header(processor.av_format_context, uri)) {
                    goto thread_end;
                }
                free(uri); uri = NULL;
                CameraCacheMuxer *cacheMuxer = muxer->get_prior_seller();
                if (cacheMuxer) {
                    while (pkt = cacheMuxer->pop_packet()) {
                        //flush all, even exit msg coming
                        if (pkt == &muxer->empty_packet)
                            continue;
                        ret = write_free_packet(&processor, encoder->encode_config, pkt);
                    }
                    delete cacheMuxer;
                }
                break;
            case CameraMuxer::MUXER_NORMAL_EXIT:
            case CameraMuxer::MUXER_FLUSH_EXIT:
                muxer->set_packet_list(NULL, &packet_list);
                exit_mtx = (std::mutex*)msg->arg1;
                exit_cond = (std::condition_variable*)msg->arg2;
                assert(packet_list);
                if (id == CameraMuxer::MUXER_NORMAL_EXIT)
                {
                    std::unique_lock<std::mutex> lk(*exit_mtx);
                    exit_cond->notify_one();
                    exit_mtx = NULL;
                    exit_cond = NULL;
                }
                goto loop_end;
            case CameraMuxer::MUXER_IMMEDIATE_EXIT:
            {
                std::mutex *pmtx = (std::mutex*)msg->arg1;
                std::condition_variable *pcond = (std::condition_variable*)msg->arg2;
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
        ret = write_free_packet(&processor, encoder->encode_config, pkt);
    }
loop_end:
    if (packet_list) {
        while (!packet_list->empty()) {
            EncodedPacket *pkt = packet_list->front();
            packet_list->pop_front();
            if (pkt == &muxer->empty_packet)
                continue;
            ret = write_free_packet(&processor, encoder->encode_config, pkt);
        }
        delete packet_list;
    }
    if (av_format_context && NO_ERROR != muxer_write_tailer(av_format_context)) {
        //how fix this
    }
    if (exit_mtx) {
        //MUXER_FLUSH_EXIT, wait for all packets complete
        std::unique_lock<std::mutex> lk(*exit_mtx);
        exit_cond->notify_one();
    }
    sync();
thread_end:
    if (uri) {
        free(uri);
    }
    if (msg) {
        delete msg;
    }
    if (av_format_context) {
        deinit_muxer(av_format_context);
    }
    muxer->remove_wtid(pthread_self());
    pthread_detach(pthread_self());
    printf("exit thread 0x%x\n", pthread_self());
    pthread_exit(NULL);
}
#endif

CameraFileCacheMuxer::CameraFileCacheMuxer()
{
#ifdef DEBUG
    snprintf(class_name, sizeof(class_name), "CameraCacheMuxer");
#endif
    snprintf(format, sizeof(format), "cvr_cache");
    file_fd = -1;
}

CameraFileCacheMuxer::~CameraFileCacheMuxer() {
//....
}
int CameraFileCacheMuxer::init() {
    return CameraMuxer::init();
}
