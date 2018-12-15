#if USE_WATERMARK

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "watermark.h"
#include "ui_resolution.h"

static Uint32 isTransparent = 0;
static Uint16 transparent_color = 0;

static Uint32 alignment_16_byte(Uint32 len)
{
    return ((len + 15) & ~15);
}

//Set transparent color of watermark image
static void set_transparent_color(Uint32 enable, Uint16 rgb565Pixel)
{
    isTransparent = enable;
    transparent_color = rgb565Pixel;
}

static void mpp_osd_data_init(WATERMARK_INFO* watermark_info)
{
    watermark_info->data_buffer.vir_addr = (uint8_t*)watermark_info->watermark_data.buffer;
    watermark_info->data_buffer.phy_fd = watermark_info->watermark_data.fd;
    watermark_info->data_buffer.handle = (void*)(&watermark_info->watermark_data.handle);
    watermark_info->data_buffer.buf_size = watermark_info->watermark_data.size;
    watermark_info->mpp_osd_data.buf = (MppBuffer)(&watermark_info->data_buffer);

    //show time
    if(watermark_info->type & WATERMARK_TIME) {
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].enable = 1;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].inverse = 0;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].start_mb_x = watermark_info->coord_info.time_bmp.x/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].start_mb_y = watermark_info->coord_info.time_bmp.y/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].num_mb_x = watermark_info->coord_info.time_bmp.width/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].num_mb_y = watermark_info->coord_info.time_bmp.height/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].buf_offset = watermark_info->osd_data_offset.time_data_offset;
        watermark_info->mpp_osd_data.num_region++;
    }

    //show image
    if(watermark_info->type & WATERMARK_LOGO){
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].enable = 1;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].inverse = 0;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].start_mb_x = watermark_info->coord_info.logo_bmp.x/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].start_mb_y = watermark_info->coord_info.logo_bmp.y/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].num_mb_x = watermark_info->coord_info.logo_bmp.width/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].num_mb_y = watermark_info->coord_info.logo_bmp.height/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].buf_offset = watermark_info->osd_data_offset.logo_data_offset;
        watermark_info->mpp_osd_data.num_region++;
    }

    //show model
    if(watermark_info->type & WATERMARK_MODEL){
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].enable = 1;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].inverse = 0;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].start_mb_x = watermark_info->coord_info.model_bmp.x/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].start_mb_y = watermark_info->coord_info.model_bmp.y/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].num_mb_x = watermark_info->coord_info.model_bmp.width/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].num_mb_y = watermark_info->coord_info.model_bmp.height/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].buf_offset = watermark_info->osd_data_offset.model_data_offset;
        watermark_info->mpp_osd_data.num_region++;
    }

    //show speed
    if(watermark_info->type & WATERMARK_SPEED){
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].enable = 1;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].inverse = 0;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].start_mb_x = watermark_info->coord_info.speed_bmp.x/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].start_mb_y = watermark_info->coord_info.speed_bmp.y/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].num_mb_x = watermark_info->coord_info.speed_bmp.width/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].num_mb_y = watermark_info->coord_info.speed_bmp.height/16;
        watermark_info->mpp_osd_data.region[watermark_info->mpp_osd_data.num_region].buf_offset = watermark_info->osd_data_offset.speed_data_offset;
        watermark_info->mpp_osd_data.num_region++;
    }
}

void watermark_init(int videoWidth, int videoHeight, WATERMARK_INFO* watermark_info)
{
    Uint32 len;

    len = watermark_info->coord_info.time_bmp.width * watermark_info->coord_info.time_bmp.height
            + watermark_info->coord_info.logo_bmp.width * watermark_info->coord_info.logo_bmp.height
            + watermark_info->coord_info.model_bmp.width * watermark_info->coord_info.model_bmp.height
            + watermark_info->coord_info.speed_bmp.width * watermark_info->coord_info.speed_bmp.height;

    watermark_info->osd_data_offset.time_data_offset = 0;
    watermark_info->osd_data_offset.logo_data_offset = watermark_info->coord_info.time_bmp.width * watermark_info->coord_info.time_bmp.height;
    watermark_info->osd_data_offset.model_data_offset = watermark_info->osd_data_offset.logo_data_offset
                                                        + watermark_info->coord_info.logo_bmp.width * watermark_info->coord_info.logo_bmp.height;
    watermark_info->osd_data_offset.speed_data_offset = watermark_info->osd_data_offset.model_data_offset
                                                        + watermark_info->coord_info.model_bmp.width * watermark_info->coord_info.model_bmp.height;

    memset(&watermark_info->watermark_data, 0, sizeof(struct video_ion));
    watermark_info->watermark_data.client = -1;
    watermark_info->watermark_data.fd = -1;
    if (video_ion_alloc_rational(&watermark_info->watermark_data, len/16, 16, 1, 1) == -1) {
        printf("%s alloc watermark_data err, no memory!\n", __func__);
        memset(watermark_info, 0, sizeof(WATERMARK_INFO));
        return;
    }

    //Initialize to transparent color
    memset(watermark_info->watermark_data.buffer, PALETTE_TABLE_LEN - 1, watermark_info->watermark_data.size);

    mpp_osd_data_init(watermark_info);

    if(watermark_info->type & WATERMARK_LOGO){
        set_transparent_color(1, 0x0800);
        get_logo_data((Uint8*)watermark_info->watermark_data.buffer + watermark_info->osd_data_offset.logo_data_offset, watermark_info->logoIndex,
                        watermark_info->coord_info.logo_bmp, isTransparent, transparent_color);
    }
}

void watermark_deinit(WATERMARK_INFO* watermark_info)
{
    if (!watermark_info) {
        return;
    }

    if(watermark_info->watermark_data.buffer)
    {
        video_ion_free(&watermark_info->watermark_data);
        watermark_info->watermark_data.buffer = NULL;
    }
}

int watermark_set_parameters(int videoWidth, int videoHeight, WATERMARK_LOGO_INDEX logoIndex, int type, WATERMARK_COORD_INFO coordInfo, WATERMARK_INFO* watermark_info){
    memset(watermark_info, 0, sizeof(WATERMARK_INFO));

    if(type <= 0)
        return -1;

    if((coordInfo.time_bmp.x < 0) || (coordInfo.time_bmp.y < 0)
        || (coordInfo.time_bmp.x + coordInfo.time_bmp.width > videoWidth)
        || (coordInfo.time_bmp.y + coordInfo.time_bmp.height > videoHeight)){
        printf("%s: time coordinate cross-border\n", __func__);
        return -1;
    }

    if((coordInfo.logo_bmp.x < 0) || (coordInfo.logo_bmp.y < 0)
        || (coordInfo.logo_bmp.x + coordInfo.logo_bmp.width > videoWidth)
        || (coordInfo.logo_bmp.y + coordInfo.logo_bmp.height > videoHeight)){
        printf("%s: logo coordinate cross-border\n", __func__);
        return -1;
    }

    if((coordInfo.model_bmp.x < 0) || (coordInfo.model_bmp.y < 0)
        || (coordInfo.model_bmp.x + coordInfo.model_bmp.width > videoWidth)
        || (coordInfo.model_bmp.y + coordInfo.model_bmp.height > videoHeight)){
        printf("%s: model coordinate cross-border\n", __func__);
        return -1;
    }

    if((coordInfo.speed_bmp.x < 0) || (coordInfo.speed_bmp.y < 0)
        || (coordInfo.speed_bmp.x + coordInfo.speed_bmp.width > videoWidth)
        || (coordInfo.speed_bmp.y + coordInfo.speed_bmp.height > videoHeight)){
        printf("%s: speed coordinate cross-border\n", __func__);
        return -1;
    }

    watermark_info->type = type;
    watermark_info->logoIndex = logoIndex;

    if(type & WATERMARK_TIME)
    {
        watermark_info->coord_info.time_bmp.x = coordInfo.time_bmp.x;
        watermark_info->coord_info.time_bmp.y = coordInfo.time_bmp.y;
        watermark_info->coord_info.time_bmp.w = coordInfo.time_bmp.w;
        watermark_info->coord_info.time_bmp.h = coordInfo.time_bmp.h;
        watermark_info->coord_info.time_bmp.width = coordInfo.time_bmp.width;
        watermark_info->coord_info.time_bmp.height = coordInfo.time_bmp.height;
    }

    if(type & WATERMARK_LOGO)
    {
        watermark_info->coord_info.logo_bmp.x = coordInfo.logo_bmp.x;
        watermark_info->coord_info.logo_bmp.y = coordInfo.logo_bmp.y;
        watermark_info->coord_info.logo_bmp.w = coordInfo.logo_bmp.w;
        watermark_info->coord_info.logo_bmp.h = coordInfo.logo_bmp.h;
        watermark_info->coord_info.logo_bmp.width = coordInfo.logo_bmp.width;
        watermark_info->coord_info.logo_bmp.height = coordInfo.logo_bmp.height;
    }

    if(type & WATERMARK_MODEL)
    {
        watermark_info->coord_info.model_bmp.x = coordInfo.model_bmp.x;
        watermark_info->coord_info.model_bmp.y = coordInfo.model_bmp.y;
        watermark_info->coord_info.model_bmp.w = coordInfo.model_bmp.w;
        watermark_info->coord_info.model_bmp.h = coordInfo.model_bmp.h;
        watermark_info->coord_info.model_bmp.width = coordInfo.model_bmp.width;
        watermark_info->coord_info.model_bmp.height = coordInfo.model_bmp.height;
    }

    if(type & WATERMARK_SPEED)
    {
        watermark_info->coord_info.speed_bmp.x = coordInfo.speed_bmp.x;
        watermark_info->coord_info.speed_bmp.y = coordInfo.speed_bmp.y;
        watermark_info->coord_info.speed_bmp.w = coordInfo.speed_bmp.w;
        watermark_info->coord_info.speed_bmp.h = coordInfo.speed_bmp.h;
        watermark_info->coord_info.speed_bmp.width = coordInfo.speed_bmp.width;
        watermark_info->coord_info.speed_bmp.height = coordInfo.speed_bmp.height;
    }

    return 0;
}

//模拟调用watermark_set_parameters设置水印参数
int watermark_setcfg(int videoWidth, int videoHeight, WATERMARK_INFO* watermark_info){
    WATERMARK_LOGO_INDEX logoIndex = -1;
    WATERMARK_COORD_INFO coordInfo;

    int type = WATERMARK_TIME | WATERMARK_LOGO;

    memset(&coordInfo, 0, sizeof(WATERMARK_COORD_INFO));
    if(videoWidth >= 2560) {
        logoIndex = LOGO1440P;
        coordInfo.logo_bmp.x = WATERMARK1440p_IMG_X;
        coordInfo.logo_bmp.w = WATERMARK1440p_IMG_W;
        coordInfo.logo_bmp.h = WATERMARK1440p_IMG_H;

        coordInfo.time_bmp.y = WATERMARK_TIME_Y;
        coordInfo.time_bmp.w = WATERMARK_TIME_W * 3;
        coordInfo.time_bmp.h = WATERMARK_TIME_H * 3;
    } else if(videoWidth >= 1920 && videoWidth < 2560) {
        logoIndex = LOGO1080P;
        coordInfo.logo_bmp.x = WATERMARK1080p_IMG_X;
        coordInfo.logo_bmp.w = WATERMARK1080p_IMG_W;
        coordInfo.logo_bmp.h = WATERMARK1080p_IMG_H;

        coordInfo.time_bmp.y = WATERMARK_TIME_Y;
        coordInfo.time_bmp.w = WATERMARK_TIME_W * 2;
        coordInfo.time_bmp.h = WATERMARK_TIME_H * 2;
    } else if(videoWidth >= 1280 && videoWidth < 1920) {
        logoIndex = LOGO720P;
        coordInfo.logo_bmp.x = WATERMARK720p_IMG_X;
        coordInfo.logo_bmp.w = WATERMARK720p_IMG_W;
        coordInfo.logo_bmp.h = WATERMARK720p_IMG_H;

        coordInfo.time_bmp.y = WATERMARK_TIME_Y;
        coordInfo.time_bmp.w = WATERMARK_TIME_W * 3/2; //240
        coordInfo.time_bmp.h = WATERMARK_TIME_H * 3/2; //24
    } else if(videoWidth >= 640 && videoWidth < 1280) {
        logoIndex = LOGO480P;
        coordInfo.logo_bmp.x = WATERMARK480p_IMG_X;
        coordInfo.logo_bmp.w = WATERMARK480p_IMG_W;
        coordInfo.logo_bmp.h = WATERMARK480p_IMG_H;

        coordInfo.time_bmp.y = WATERMARK_TIME_Y/2;
        coordInfo.time_bmp.w = WATERMARK_TIME_W;
        coordInfo.time_bmp.h = WATERMARK_TIME_H;
    } else if(videoWidth >= 480 && videoWidth < 640) {
        logoIndex = LOGO360P;
        coordInfo.logo_bmp.x = WATERMARK360p_IMG_X;
        coordInfo.logo_bmp.w = WATERMARK360p_IMG_W;
        coordInfo.logo_bmp.h = WATERMARK360p_IMG_H;

        coordInfo.time_bmp.y = WATERMARK_TIME_Y/2;
        coordInfo.time_bmp.w = WATERMARK_TIME_W;
        coordInfo.time_bmp.h = WATERMARK_TIME_H;
    } else if(videoWidth >= 320 && videoWidth < 480) {
        logoIndex = LOGO240P;
        coordInfo.logo_bmp.x = WATERMARK240p_IMG_X;
        coordInfo.logo_bmp.w = WATERMARK240p_IMG_W;
        coordInfo.logo_bmp.h = WATERMARK240p_IMG_H;

        coordInfo.time_bmp.y = WATERMARK_TIME_Y/2;
        coordInfo.time_bmp.w = WATERMARK_TIME_W;
        coordInfo.time_bmp.h = WATERMARK_TIME_H;
    } else {
        return -1;
    }

    coordInfo.logo_bmp.width = alignment_16_byte(coordInfo.logo_bmp.w);
    coordInfo.logo_bmp.height = alignment_16_byte(coordInfo.logo_bmp.h);

    coordInfo.time_bmp.width = alignment_16_byte(coordInfo.time_bmp.w);
    coordInfo.time_bmp.height = alignment_16_byte(coordInfo.time_bmp.h);

    coordInfo.time_bmp.x = coordInfo.logo_bmp.x - (coordInfo.time_bmp.width - coordInfo.logo_bmp.width);
    coordInfo.logo_bmp.y = coordInfo.time_bmp.y + coordInfo.time_bmp.height;

    return watermark_set_parameters(videoWidth, videoHeight, logoIndex, type, coordInfo, watermark_info);
}
#endif
