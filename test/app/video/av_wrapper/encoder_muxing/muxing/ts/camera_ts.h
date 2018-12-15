//by wangh
#ifndef CAMERA_TS_H
#define CAMERA_TS_H

#include "../camera_muxer.h"

class CameraTs: public CameraMuxer
{
private:
    CameraTs();
public:
    ~CameraTs();
    int init_uri(char *uri, int rate);
    void stop_current_job();
    CREATE_FUNC(CameraTs)
};

#endif // CAMERA_TS_H
