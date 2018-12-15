//by wangh
#ifndef CAMERA_MP4_H
#define CAMERA_MP4_H

#include "../camera_muxer.h"

class CameraMp4 : public CameraMuxer
{
private:
    CameraMp4();
    bool low_frame_rate_mode;
    struct timeval lbr_pre_timeval;

public:
    ~CameraMp4();
    bool is_low_frame_rate();
    void set_low_frame_rate(bool val, struct timeval start_timeval);
    void reset_lbr_time();
    void push_packet(EncodedPacket* pkt);
    CREATE_FUNC(CameraMp4)
};

#endif // CAMERA_MP4_H
