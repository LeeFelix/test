#ifndef __WATER_MARK_H__
#define __WATER_MARK_H__

#include "watermark_minigui.h"
#include <mpp/rk_mpi.h>
#include <mpp/rk_mpi_cmd.h>

#include "cvr_ffmpeg_shared.h"
#include "video_ion_alloc.h"

//2560x1440
#define WATERMARK1440p_IMG_W    192
#define WATERMARK1440p_IMG_H    64
#define WATERMARK1440p_IMG_X   ((2560 - WATERMARK1440p_IMG_W - 32) & 0xfffffff0)

//1920x1080
#define WATERMARK1080p_IMG_W    144
#define WATERMARK1080p_IMG_H    48
#define WATERMARK1080p_IMG_X   ((1920 - WATERMARK1080p_IMG_W - 32)  & 0xfffffff0)

//1280x720
#define WATERMARK720p_IMG_W     96
#define WATERMARK720p_IMG_H     32
#define WATERMARK720p_IMG_X    ((1280 - WATERMARK720p_IMG_W - 32)  & 0xfffffff0)

//640x480
#define WATERMARK480p_IMG_W     72
#define WATERMARK480p_IMG_H     24
#define WATERMARK480p_IMG_X    ((640 - WATERMARK480p_IMG_W - 16)  & 0xfffffff0)

//480x320
#define WATERMARK360p_IMG_W     48
#define WATERMARK360p_IMG_H     16
#define WATERMARK360p_IMG_X    ((480 - WATERMARK360p_IMG_W - 16)  & 0xfffffff0)

//320x240
#define WATERMARK240p_IMG_W     48
#define WATERMARK240p_IMG_H     16
#define WATERMARK240p_IMG_X    ((320 - WATERMARK240p_IMG_W - 16)  & 0xfffffff0)
/* End 16-byte alignment */

#define WATERMARK_TIME 0x00000001
#define WATERMARK_LOGO 0x00000010
#define WATERMARK_MODEL 0x00000100
#define WATERMARK_SPEED 0x00001000

typedef enum {
  LOGO240P = 1,
  LOGO360P,
  LOGO480P,
  LOGO720P,
  LOGO1080P,
  LOGO1440P,
} WATERMARK_LOGO_INDEX;

typedef struct _WATERMARK_COORD_INFO{
    WATERMARK_RECT time_bmp;
    WATERMARK_RECT logo_bmp;
    WATERMARK_RECT model_bmp;
    WATERMARK_RECT speed_bmp;
} WATERMARK_COORD_INFO;

typedef struct _OSD_DATA_OFFSET_INFO{
    Uint32 time_data_offset;
    Uint32 logo_data_offset;
    Uint32 model_data_offset;
    Uint32 speed_data_offset;
} OSD_DATA_OFFSET_INFO;

typedef struct _WATERMARK_INFO
{
    int type;
    Uint32 logoIndex;
    OSD_DATA_OFFSET_INFO osd_data_offset;
    WATERMARK_COORD_INFO coord_info;

    struct video_ion watermark_data;
    DataBuffer_t data_buffer;
    MppEncOSDData mpp_osd_data;
} WATERMARK_INFO;

void watermark_init(int videoWidth, int videoHeight, WATERMARK_INFO* watermark_info);
void watermark_deinit(WATERMARK_INFO* watermark_info);
int watermark_set_parameters(int videoWidth, int videoHeight, WATERMARK_LOGO_INDEX logoIndex, int type, WATERMARK_COORD_INFO coordInfo, WATERMARK_INFO* watermark_info);

int watermark_setcfg(int videoWidth, int videoHeight, WATERMARK_INFO* watermark_info);

#endif
