#ifndef CAMERAFILECACHEMUXER_H
#define CAMERAFILECACHEMUXER_H

#include "camera_cache_muxer.h"

class CameraFileCacheMuxer : public CameraCacheMuxer
{
private:
    int file_fd;
    int cache_time_len; //sec
    CameraFileCacheMuxer();

public:
    ~CameraFileCacheMuxer();
    int init();
    void SetTimeLength(int sec)
    {
        cache_time_len = sec;
    }
    CREATE_FUNC(CameraFileCacheMuxer)
};

#endif // CAMERAFILECACHEMUXER_H
