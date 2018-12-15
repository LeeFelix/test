//by wangh
#include "camera_cache_muxer.h"

CameraCacheMuxer::CameraCacheMuxer()
{
#ifdef DEBUG
    snprintf(class_name, sizeof(class_name), "CameraCacheMuxer");
#endif
    snprintf(format, sizeof(format), "");
    run_func = NULL;
    back_list = NULL;
    exit_id = MUXER_IMMEDIATE_EXIT;
}

CameraCacheMuxer::~CameraCacheMuxer()
{
    if (back_list) {
        while (!back_list->empty()) {
            EncodedPacket* pkt = back_list->front();
            back_list->pop_front();
            pkt->unref();
        }
    }
}

int CameraCacheMuxer::init()
{
    run_func = NULL;
    return 0;
}

void CameraCacheMuxer::flush_packet_list(CameraMuxer* consumer)
{
    set_packet_list(NULL, &back_list);
    consumer->set_prior_seller(this);
}

EncodedPacket* CameraCacheMuxer::pop_packet()
{
    EncodedPacket* ret = NULL;
    if (back_list && !back_list->empty()) {
        ret = back_list->front();
        back_list->pop_front();
    }
    return ret;
}
