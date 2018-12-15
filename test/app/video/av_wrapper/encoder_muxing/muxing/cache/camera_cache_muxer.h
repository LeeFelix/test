//by wangh
#ifndef CAMERACACHEMUXER_H
#define CAMERACACHEMUXER_H

#include "../camera_muxer.h"

//memory cache
class CameraCacheMuxer : public CameraMuxer //seller
{
protected:
    CameraCacheMuxer();
    std::list<EncodedPacket*>* back_list;

public:
    ~CameraCacheMuxer();
    int init();
    virtual void flush_packet_list(CameraMuxer* consumer);
    virtual EncodedPacket* pop_packet();
    inline void set_encoder(BaseEncoder* video_encoder,
                            BaseEncoder* audio_encoder)
    {
        UNUSED(video_encoder);
        UNUSED(audio_encoder);
    }
    CREATE_FUNC(CameraCacheMuxer)
};

#endif // CAMERACACHEMUXER_H
