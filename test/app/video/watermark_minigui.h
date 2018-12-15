#ifndef __WATER_MARK_MINIGUI_H__
#define __WATER_MARK_MINIGUI_H__

#include <minigui/common.h>

typedef struct _WATERMARK_RECT
{
    Uint32 x;
    Uint32 y;
    Uint32 w;       //not 16-byte alignment
    Uint32 h;       //not 16-byte alignment
    Uint32 width;   //16-byte alignment
    Uint32 height;  //16-byte alignment
} WATERMARK_RECT;

#define PALETTE_TABLE_LEN 256

extern Uint32 YUV444_PALETTE_TABLE[PALETTE_TABLE_LEN];

void update_rectBmp(WATERMARK_RECT srcRect, WATERMARK_RECT dstRect, Uint8* dst);
void get_logo_data(Uint8* dst, Uint32 logoIndex, WATERMARK_RECT rect, Uint32 flag, Uint16 rgb565_pixel);
void watermark_minigui_setHWND(HWND wnd);
void watermark_minigui_setHDC(HDC dc);
#endif
