//by wangh
#include "muxing/camera_muxer.h"
#include "packet_dispatcher.h"

void PacketDispatcher::Dispatch(EncodedPacket* pkt)
{
    std::lock_guard<std::mutex> lk(list_mutex);
    //assert(!muxers.empty());
    if (pkt->is_phy_buf) {
        bool sync = true;
        for (std::list<CameraMuxer*>::iterator i = handlers.begin(); i != handlers.end(); i++) {
            CameraMuxer* muxer = *i;
            if (muxer->use_data_async()) {
                sync = false;
                break;
            }
        }
        if (!sync) {
            //need copy buffer
            pkt->copy_av_packet();
        }
    }

    for (std::list<CameraMuxer*>::iterator i = handlers.begin(); i != handlers.end(); i++) {
        CameraMuxer* muxer = *i;
        muxer->push_packet(pkt);
    }
    //pkt->unref();
}

