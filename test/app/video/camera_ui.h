#ifndef __CAMERA_UI_H__
#define __CAMERA_UI_H__
#include <stdbool.h>
#include "common.h"

static BITMAP bmp;
static BITMAP bmp_dakai;

//---------------------
static HMENU creatmenuPlay();
static HMENU creatmenu();
static HMENU creatmenucamera();
static HMENU createpmenufileDELECT();
static HMENU createpmenufileMP_camera();
static HMENU createpmenufileMP();
static HMENU createpmenufileTIME();
static HMENU createpmenufileCAR();
static HMENU createpmenufileSETTING();
static HMENU createpmenufileRECORD();
static HMENU createpmenufileMARK();
#if ENABLE_MD_IN_MENU
static HMENU createpmenufileDETECT();
#endif
static HMENU createpmenufileEXPOSAL();
static HMENU createpmenufileBRIGHT();
static HMENU createpmenufileAUTORECORD();
static HMENU createpmenufileLANGUAGE();
static HMENU createpmenufileFREQUENCY();

//
void ui_deinit_init_camera(HWND hWnd,char i,char j);
int sdcardformat_back(void *arg, int param);
void ui_set_white_balance(int i);
void ui_set_exposure_compensation(int i);
void cmd_IDM_frequency(char i);
void changemode(HWND hWnd, int mode);
void cmd_IDM_CAR(HWND hWnd, char i);
void startrec(HWND hWnd);
void stoprec(HWND hWnd);
void cmd_IDM_DEBUG_VIDEO_BIT_RATE(HWND hWnd, unsigned int val);

extern void get_timeBmp_data(Uint8* dst, BITMAP* bmp, Uint16 textColor);

int getSD(void);

#ifdef __cplusplus
extern "C" {
#endif
void start_motion_detection(HWND ui_hnd);
void stop_motion_detection();
#ifdef __cplusplus
}
#endif

#endif
