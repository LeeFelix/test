/*
** $Id: helloworld.c 793 2010-07-28 03:36:29Z dongjunjie $
**
** Listing 1.1
**
** helloworld.c: Sample program for MiniGUI Programming Guide
**      The first MiniGUI application.
**
** Copyright (C) 2004 ~ 2009 Feynman Software.
**
** License: GPL
*/

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <linux/watchdog.h>

#include <time.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#ifdef _LANG_ZHCN
#include "camera_ui_res_cn.h"
#elif defined _LANG_ZHTW
#include "camera_ui_res_tw.h"
#else
#include "camera_ui_res_en.h"
#endif

#include "camera_ui.h"
#include "camera_ui_def.h"
#include "ueventmonitor/ueventmonitor.h"
#include "ueventmonitor/usb_sd_ctrl.h"
#include "wifi_management.h"
#include "parameter.h"
#include "fs_manage/fs_manage.h"
#include "fs_manage/fs_fileopera.h"
#include "fs_manage/fs_sdcard.h"

#include "common.h"
#include "videoplay.h"
#include <dpp/dpp_frame.h>

#include "audioplay.h"
#include <assert.h>

//#include <minigui/fbvideo.h>
#include <pthread.h>

#include "ui_resolution.h"

#include "watermark.h"
#include "video.h"
#include "example/user.h"
#include "videoplay.h"
#include "gsensor.h"

#ifdef PROTOCOL_GB28181
#include "gb28181/rk_gb28181.h"
#endif

#ifdef MSG_FWK
#include "libfwk_controller/fwk_controller.h"
#include "libfwk_glue/fwk_glue.h"
#endif
//#define USE_WATCHDOG

#define BLOCK_PREV_NUM 1   // min
#define BLOCK_LATER_NUM 1  // min

#define MODE_RECORDING 0
#define MODE_PHOTO 1
#define MODE_EXPLORER 2
#define MODE_PREVIEW 3
#define MODE_PLAY 4
#define MODE_USBDIALOG 5
#define MODE_USBCONNECTION 6
//USB
#define USB_MODE 1
//
#define BATTERY_CUSHION 3
#define USE_KEY_STOP_USB_DIS_SHUTDOWN

static int test_replay = 0;
static char testpath[256];
static dsp_ldw_out adas_out;
static short screenoff_time;

//battery stable
static int last_battery = 1;

static BITMAP A_bmap[3];

RECT msg_rcAB = {A_IMG_X, A_IMG_Y, A_IMG_X + A_IMG_W, A_IMG_Y + A_IMG_H};

RECT adas_rc = {adas_X, adas_Y, adas_W, 400};

RECT msg_rcMove = {MOVE_IMG_X, MOVE_IMG_Y, MOVE_IMG_X + MOVE_IMG_W, MOVE_IMG_Y + MOVE_IMG_H};


RECT USB_rc = {USB_IMG_X, USB_IMG_Y, USB_IMG_X + USB_IMG_W,
                      USB_IMG_Y + USB_IMG_H};

RECT msg_rcBtt = {BATT_IMG_X, BATT_IMG_Y, BATT_IMG_X + BATT_IMG_W,
                         BATT_IMG_Y + BATT_IMG_H};

static BITMAP batt_bmap[5];

RECT msg_rcSD = {TF_IMG_X, TF_IMG_Y, TF_IMG_X + TF_IMG_W,
                        TF_IMG_Y + TF_IMG_H};

static BITMAP tf_bmap[2];

RECT msg_rcMode = {MODE_IMG_X, MODE_IMG_Y, MODE_IMG_X + MODE_IMG_W,
                          MODE_IMG_Y + MODE_IMG_H};

static BITMAP mode_bmap[4];

RECT msg_rcResolution = {RESOLUTION_IMG_X, RESOLUTION_IMG_Y,
                                RESOLUTION_IMG_X + RESOLUTION_IMG_W,
                                RESOLUTION_IMG_Y + RESOLUTION_IMG_H};

static BITMAP resolution_bmap[2];

static BITMAP move_bmap[2];

static BITMAP topbk_bmap;

RECT msg_rcTime = {REC_TIME_X, REC_TIME_Y, REC_TIME_X + REC_TIME_W,
                          REC_TIME_Y + REC_TIME_H};

RECT msg_rcRecimg = {REC_IMG_X, REC_IMG_Y, REC_IMG_X + REC_IMG_W,
                            REC_IMG_Y + REC_IMG_H};
static BITMAP recimg_bmap;

RECT msg_rcMic = {MIC_IMG_X, MIC_IMG_Y, MIC_IMG_X + MIC_IMG_W,
                         MIC_IMG_Y + MIC_IMG_H};

static BITMAP mic_bmap[2];

RECT msg_rcSDCAP = {TFCAP_STR_X, TFCAP_STR_Y, TFCAP_STR_X + TFCAP_STR_W,
                           TFCAP_STR_Y + TFCAP_STR_H};

RECT msg_rcSYSTIME = {SYSTIME_STR_X, SYSTIME_STR_Y,
                             SYSTIME_STR_X + SYSTIME_STR_W,
                             SYSTIME_STR_Y + SYSTIME_STR_H};

RECT msg_rcFILENAME = {FILENAME_STR_X, FILENAME_STR_Y,
                              FILENAME_STR_X + FILENAME_STR_W,
                              FILENAME_STR_Y + FILENAME_STR_H};

RECT msg_rcWifi = {WIFI_IMG_X, WIFI_IMG_Y, WIFI_IMG_X + WIFI_IMG_W,
                         WIFI_IMG_Y + WIFI_IMG_H};

static BITMAP wifi_bmap[5];

RECT msg_rcWatermarkTime = {WATERMARK_TIME_X, WATERMARK_TIME_Y,
                                   WATERMARK_TIME_X + WATERMARK_TIME_W,
                                   WATERMARK_TIME_Y + WATERMARK_TIME_H};

RECT msg_rcWatermarkImg = {WATERMARK_IMG_X,
                                   WATERMARK_IMG_Y,
                                  WATERMARK_IMG_X + WATERMARK_IMG_W,
                                  WATERMARK_IMG_Y + WATERMARK_IMG_H};
BITMAP watermark_bmap[7];

static BITMAP play_bmap;
static pthread_mutex_t set_mutex;
static int adasFlag = 0;

static bool takephoto = false;

static bool usewatchdog = false;
static int fd_wtd;

// menu
static BITMAP bmp_menu_mp[2];
static BITMAP bmp_menu_time[2];
static BITMAP bmp_menu_car[2];
static BITMAP bmp_menu_setting[2];
static BITMAP png_menu_debug[2];
static BITMAP folder_bmap;
static BITMAP back_bmap;
static BITMAP usb_bmap;
static BITMAP filetype_bmap[FILE_TYPE_MAX];

//------------------------------------
static const char* DSP_IDC[2] = {" IDC ",
                                  " 畸变校正 "};
static const char* DSP_3DNR[2] = {" 3DNR ",
                                  //	"night-vision enhance",
                                  " 夜视增强 "};
static const char* fontcamera_ui[2] = {" Front ", " 前置 "};
static const char* backcamera_ui[2] = {" Back ", " 后置 "};

static const char* reboot[2] = {" reboot ", " reboot "};
static const char* recovery_debug[2] = {" recovery ", " recovery拷机  "};
static const char* awake[2] = {" awake ", " 一级待机唤醒 "};
static const char* standby[2] = {" standby ", " 二级待机 "};
static const char* mode_change[2] = {" mode change ", " 模式切换 "};
static const char* video_debug[2] = {" Video ", " video(回放视频) "};
static const char* beg_end_video[2] = {" beg_end_video ", " 开始/结束录像 "};

static const char* photo_debug[2] = {" photo ", " 录像拍照 "};
static const char* temp_debug[2] = {" temp control ", " 温度控制 "};
static const char* DSP_LDW[2] = {" ADAS_LDW ",
                                 //	"lane departure warning",
                                 " 车道偏离预警 "};

static const char* MP[2] = {
    //	"  MP ",
    "Resolution", "清晰度"};
static const char* Record_Time[2] = {
    //	" Record_Time ",
    "Video length", " 录像时长 "};
static const char* Time_OneMIN[2] = {" 1min ", " 1分钟 "};
static const char* Time_TMIN[2] = {" 3min ", " 3分钟 "};
static const char* Time_FMIN[2] = {" 5min ", " 5分钟 "};
static const char* OFF[2] = {" OFF ", " 关闭 "};
static const char* ON[2] = {" ON ", " 打开 "};
static const char* Record_Mode[2] = {
    //	" Record_Mode ",
    "Record Mode", " 录像模式 "};
static const char* Front_Record[2] = {
    //	" Front_Record ",
    "Front video", " 前镜头录像 "};
static const char* Back_Record[2] = {
    //	" Back_Record ",
    "Rear video", " 后镜头录像 "};
static const char* Double_Record[2] = {
    //	" Double_Record ",
    "Double video", " 双镜头录像 "};
static const char* setting[2] = {" Setting ", " 设置 "};
static const char* debug[2] = {" DEBUG ", " DEBUG "};

static const char* WB[2] = {" White Balance ", " 白平衡 "};
static const char* exposal[2] = {" Exposure ", " 曝光度 "};
#if ENABLE_MD_IN_MENU
static const char* auto_detect[2] = {" motion detection ", " 移动侦测 "};
#endif
static const char* water_mark[2] = {" Date Stamp ", " 日期标签 "};
static const char* record[2] = {" Record Audio ", " 录影音频 "};
static const char* auto_record[2] = {" Boot record ", " 开机录像 "};
static const char* language[2] = {" language ", " 语言选择 "};
static const char* frequency[2] = {" frequency ", " 频率 "};
static const char* format[2] = {" Format ", " 格式化 "};
static const char* setdate[2] = {" Date Set ", " 设置日期 "};
static const char* usbmode[2] = {" USB MODE ", " USB MODE "};
static const char* fw_ver[2] = {" Version ", " 固件版本 "};
static const char* recovery[2] = {" Recovery ", " 恢复出厂设置 "};
static const char* Br_Auto[2] = {" auto ", " 自动 "};
static const char* Br_Sun[2] = {" Daylight ", " 日光 "};
static const char* Br_Flu[2] = {" fluorescence ", " 荧光 "};
static const char* Br_Cloud[2] = {" cloudysky ", " 阴天 "};
static const char* Br_tun[2] = {" tungsten ", " 灯光 "};
static const char* Del_Yes[2] = {" yes ", " 是 "};
static const char* Del_No[2] = {" no ", " 否 "};
static const char* DELECT[2] = {"DELECT", "删除"};
static const char* HzNum_50[2] = {" 50HZ ", " 50HZ "};

static const char* UsbMsc[2] = {" msc ", " msc "};
static const char* UsbAdb[2] = {" adb ", " adb "};

static const char* HzNum_60[2] = {" 60HZ ", " 60HZ "};

static const char* LAN_ENG[2] = {" English ", " English "};
static const char* LAN_CHN[2] = {" 简体中文 ", " 简体中文 "};

static const char* ExposalData_0[2] = {" -3 ", " -3 "};

static const char* ExposalData_1[2] = {" -2 ", " -2 "};

static const char* ExposalData_2[2] = {" -1 ", " -1 "};

static const char* ExposalData_3[2] = {"  0 ", "  0 "};

static const char* ExposalData_4[2] = {"  1 ", "  1 "};

static const char* MPVed_720P[2] = {" 720P ", " 720P "};
static const char* MPVed_1M[2] = {" 1M ", " 1M "};
static const char* MPVed_2M[2] = {" 2M ", " 2M "};
static const char* MPVed_3M[2] = {" 3M ", " 3M "};

static const char* MPVed_1080P[2] = {" 1080P ", " 1080P "};

static const char* MPCam_1M[2] = {" 1M ", " 1M "};

static const char* MPCam_2M[2] = {" 2M ", " 2M "};

static const char* MPCam_3M[2] = {" 3M ", " 3M "};
static const char* backlight[2] = {" Bright ", " 亮度 "};

static const char* collision[2] = {" collision detect ", " 碰撞检测 "};
static const char* Collision_0[2] = {" close ", " 关闭 "};
static const char* Collision_1[2] = {" low ", " 低 "};
static const char* Collision_2[2] = {" middle ", " 中 "};
static const char* Collision_3[2] = {" hight ", " 高 "};
static const char* leavecarrec[2] = {" leave car rec ", " 停车录像 "};
static const char* Leavecarrec_Off[2] = {" OFF ", " 关闭 "};
static const char* Leavecarrec_On[2] = {" ON ", " 打开 "};
static const char* screenoffset[2] = {" Auto Off Screen ", " 屏幕自动关闭 "};
static const char* vbit_rate_vals[7] = {" 1 ", " 2 ",  " 4 ", " 6 ",
                                        " 8 ", " 10 ", " 12 "};
static const char* vbit_rate_debug[2] = {" video bit rate per pixel ",
                                         " 视频单像素码率 "};
static const char* wifi[2] = {" WIFI ",
                                         " WIFI "};

//----------------------
static int battery = 0;

static char sdcard = 0;
static char move = 1;
static int cap;
static char cam_ab = 0;
static int SetMode = 0;
static int SetCameraMP = 0;
static int vd = 0;
static HMENU hmnuFor3DNR;
static HMENU hmnuForLDW;
static HMENU hwndforpre;

static HWND hWndUSB;
static HWND hMainWndForM;
static HWND hMainWnd;
static HWND explorer_wnd;
static HWND setdate_wnd;
static HMENU explorer_menu;
static HMENU explorer_menu_list;
static HMENU hmnuForFRE;
static HMENU hmnuForLAN;
static HMENU hmnuForAutoRE;
static HMENU hmnuForBr;
static HMENU hmnuForEx;
static HMENU hmnuForRe;
static HMENU hmnuForMark;
static HMENU hmnuForDe;
static HMENU hmnuMain;
static HMENU hmnuForMODE;
static HMENU hmnuMaincamera;
static HMENU hmnuMainplay;
static HWND hWndForProc;
static HWND hMainWndForPlay;
static HLVITEM hItemSelectedForback;
static HMENU hmnuForUSB;
static HMENU hmnuForFWVER;
static HMENU hmnuForCOLLI;
static HMENU hmnuForLEAVEREC;
static HMENU hmnuForAutoOffScreen;

//-------------------------
static int video_rec_state = 0;
static int video_play_state = 0;
static DWORD bkcolor;
static int sec;
static char* explorer_path = 0;
static pthread_t explorer_update_tid = 0;
static int explorer_update_run;
static int explorer_update_reupdate = 0;

#define FILENAME_LEN 128
static char previewname[FILENAME_LEN];
static struct file_list* filelist;
static struct file_node* currentfile;
static int totalfilenum = 0;
static int currentfilenum = 0;
static int preview_isvideo = 0;
static int videoplay = 0;
// static int adasflagtooff=0;
struct FlagForUI FlagForUI_ca;

// test code start
static void* keypress_sound = NULL;
static void* capture_sound = NULL;
// test code end
static struct ui_frame frontcamera[4] = {0}, backcamera[4] = {0};
static int firstrunmenu = 0;
// debug
static int reboot_time = 0;
static int recovery_time = 0;
static int awake_time = 0;
static int standby_time = 0;
static int modechange_time = 0;
static int video_time = 0;
static int beg_end_video_time = 0;
static int photo_time = 0;
// int modechange_pre_flage=0;
//sd
int param_sd =0;

/*SHUTDOWN PARAMS*/
static int gshutdown = 0;

static int key_lock = 0;

//
void (*wififormat_callback)(void) = NULL;

static void initrec(HWND hWnd);
static void deinitrec(HWND hWnd);
void changemode(HWND hWnd, int mode);
void exitvideopreview(void);
void loadingWaitBmp(HWND hWnd);
void startrec(HWND hWnd);
void stoprec(HWND hWnd);
void unloadingWaitBmp(HWND hWnd);

void hdmi_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  SendMessage(hMainWnd, MSG_HDMI, (WPARAM)msg0, (LPARAM)msg1);
}

void camere_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  PostMessage(hMainWnd, MSG_CAMERA, (WPARAM)msg0, (LPARAM)msg1);
}

void gsensor_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  switch (cmd)
  {
    case CMD_COLLISION:
      if((SetMode == MODE_RECORDING) && (video_rec_state == 1)) {
        video_record_blocknotify(BLOCK_PREV_NUM, BLOCK_LATER_NUM);
        video_record_savefile();
      }
      break;
  }
}

void videoplay_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  switch (cmd)
  {
    case CMD_VIDEOPLAY_EXIT:
      PostMessage(hMainWnd, MSG_VIDEOPLAY, (WPARAM)NULL, 
	              EVENT_VIDEOPLAY_EXIT);
      break;
    case CMD_VIDEOPLAY_UPDATETIME:
      SendMessage(hMainWnd, MSG_VIDEOPLAY, (WPARAM)msg0,
                  EVENT_VIDEOPLAY_UPDATETIME);
      break;
  }
}

void user_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  switch (cmd)
  {
    case CMD_USER_RECORD_RATE_CHANGE:
      PostMessage(hMainWnd, MSG_RECORD_RATE_CHANGE, (WPARAM)msg0, (LPARAM)msg1);
	    break;
	  case CMD_USER_MDPROCESSOR:
      SendMessage(hMainWnd, MSG_ATTACH_USER_MDPROCESSOR, (WPARAM)msg0, (LPARAM)msg1);
      break;
	  case CMD_USER_MUXER:
      PostMessage(hMainWnd, MSG_ATTACH_USER_MUXER, (WPARAM)msg0, (LPARAM)msg1);
      break;
  }
}

void rec_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  switch (cmd)
  {
    case CMD_UPDATETIME:
      PostMessage(hMainWnd, MSG_VIDEOREC, (WPARAM)msg0, EVENT_VIDEOREC_UPDATETIME);
	    break;
	  case CMD_ADAS:
      PostMessage(hMainWnd, MSG_ADAS, (WPARAM)msg0, (LPARAM)msg1);
      break;
	  case CMD_PHOTOEND:
      SendNotifyMessage(hMainWnd, MSG_PHOTOEND, (WPARAM)msg0, (LPARAM)msg1);
      break;
  }
}

void sd_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  switch (cmd)
  {
    case SD_NOTFIT:
      PostMessage(hMainWnd, MSG_SDNOTFIT, (WPARAM)msg0, (LPARAM)msg1);
	  break;
	case SD_MOUNT_FAIL:
      PostMessage(hMainWnd, MSG_SDMOUNTFAIL, (WPARAM)msg0, (LPARAM)msg1);
      break;
	case SD_CHANGE:
      PostMessage(hMainWnd, MSG_SDCHANGE, (WPARAM)msg0, (LPARAM)msg1);
      break;
  }
}

void usb_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
  PostMessage(hMainWnd, MSG_USBCHAGE, (WPARAM)msg0, (LPARAM)msg1);
}

void batt_event_callback(int cmd, void *msg0, void *msg1)
{
  //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);

  switch (cmd)
  {
    case CMD_UPDATE_CAP:
      PostMessage(hMainWnd, MSG_BATTERY, (WPARAM)msg0, (LPARAM)msg1);
      break;
    case CMD_LOW_BATTERY:
      PostMessage(hMainWnd, MSG_KEYLONGPRESS, SCANCODE_SHUTDOWN, (LPARAM)msg1);
      break;
    case CMD_DISCHARGE:
      PostMessage(hMainWnd, MSG_BATTERY, (WPARAM)msg0, (LPARAM)msg1);
      PostMessage(hMainWnd, MSG_USB_DISCONNECT, SCANCODE_SHUTDOWN, (LPARAM)msg1);
      break;

  }
}


void mic_onoff(HWND hWnd, int onoff) {
  parameter_save_video_audio(onoff);
  video_record_setaudio(onoff);
  InvalidateRect(hWnd, &msg_rcMic, FALSE);
}
int getSD(void) {
  return sdcard;
}

/*SHUTDOWN FUNCTION*/
static int shutdown_deinit(HWND hWnd)
{
	int ret = 0;

	if ((gshutdown == 1)||(gshutdown == 2)) {
		gshutdown = 2; //tell usb disconnect shutdown
		return -1;
	}
	gshutdown = 1;

	//stop other
	deinitrec(hWnd);

	system("poweroff");

	return 0;
}

/*usb disconenct shutdown process
 */
#ifdef USE_KEY_STOP_USB_DIS_SHUTDOWN
int stop_usb_disconnect_poweroff()
{
	gshutdown = 3;
	return 0;
}
#endif
int usb_disconnect_poweroff(void *arg)
{
	int ret, i;
	HWND hWnd = (HWND)arg;

	for (i = 0; i < 10; i++) {
		if (charging_status_check()) {
			gshutdown = 0;
			printf("%s[%d]:vbus conenct,cancel shutdown\n",__func__, __LINE__);
			return 0;
		}
		if (gshutdown == 2)//powerkey or low battery shutdown
			break;
#ifdef USE_KEY_STOP_USB_DIS_SHUTDOWN
		if (gshutdown == 3) {
			//key stop usb disconnect shutdown
			gshutdown = 0;
			printf("%s[%d]:key stop usb disconnect shutdown\n",__func__, __LINE__);
			return 0;
		}
#endif
		printf("%s[%d]:shutdown wait---%d\n",__func__, __LINE__, i);
		sleep(1);
	}
	printf("%s[%d]:gshutdown=%d,shutdown...\n",__func__, __LINE__, gshutdown);
	deinitrec(hWnd);

	if (charging_status_check()) {
		gshutdown = 0;
		printf("%s[%d]:vbus conenct,cancel shutdown\n",__func__, __LINE__);
		video_record_deinit();
		printf("init rec---\n");
		initrec(hWnd);
		if (video_rec_state == 0 && sdcard == 1)
			startrec(hWnd);
		return 0;
	}

	system("poweroff");
}

void *usb_disconnect_process(void *arg)
{
	printf ("usb_disconnect_process\n");
    usb_disconnect_poweroff(arg);
    pthread_exit(NULL);
}

pthread_t run_usb_disconnect(HWND hWnd)
{
    pthread_t tid;
	printf ("run_usb_disconnect\n");
    if(pthread_create(&tid, NULL, usb_disconnect_process, (void *)hWnd)) {
        printf("Create run_usb_disconnect thread failure\n");
        return -1;
    }

    return tid;
}

static int shutdown_usb_disconnect(HWND hWnd)
{
	if (gshutdown == 1)
		return -1;
	gshutdown = 1;

	run_usb_disconnect(hWnd);
	return 0;	
}

/*
 *Get usb state interface
 */
static int get_usb_state()
{
	int fd, size;
	char path[] = "sys/class/android_usb/android0/state";
	char usb_state[15] = {0};

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf ("%s[%d]:Open %s failure\n",__func__, __LINE__, path);
		return -1;
	}

	size = read(fd, usb_state, 15);
	if (size < 0) {
		printf ("%s[%d]:Read %s failure\n",__func__, __LINE__, path);
		close(fd);
		return -1;
	}

	if (!strncmp(usb_state, "DISCONNECTED", 12)) {
		printf ("%s[%d]:---%s\n",__func__, __LINE__, usb_state);
		return 0;
	} else {
		printf ("%s[%d]:---%s\n",__func__, __LINE__, usb_state);
		return 1;
	}
}

static void prompt(HWND hDlg) {
  time_t t;
  struct tm tm;
  struct timeval tv;
  int year = SendDlgItemMessage(hDlg, IDL_YEAR, CB_GETSPINVALUE, 0, 0);
  int mon = SendDlgItemMessage(hDlg, IDL_MONTH, CB_GETSPINVALUE, 0, 0);
  int mday = SendDlgItemMessage(hDlg, IDL_DAY, CB_GETSPINVALUE, 0, 0);
  int hour = SendDlgItemMessage(hDlg, IDC_HOUR, CB_GETSPINVALUE, 0, 0);
  int min = SendDlgItemMessage(hDlg, IDC_MINUTE, CB_GETSPINVALUE, 0, 0);
  int sec = SendDlgItemMessage(hDlg, IDL_SEC, CB_GETSPINVALUE, 0, 0);
  printf("%d--%d--%d--%d--%d--%d\n", year, mon, mday, hour, min, sec);
  tm.tm_year = year - 1900;
  tm.tm_mon = mon - 1;
  tm.tm_mday = mday;
  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_sec = sec;
  setDateTime(&tm);
}
static int MyDateBoxProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam) {
  int i;
  time_t t;
  //	char bufff[512] = "";
  struct tm* tm;
  switch (message) {
    case MSG_INITDIALOG: {
      HWND hCurFocus;
      HWND hNewFocus;

      time(&t);
      tm = localtime(&t);
      hCurFocus = GetFocusChild(hDlg);
      SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINFORMAT, 0, (LPARAM) "%04d");
      SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINRANGE, 1900, 2100);
      SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINVALUE, tm->tm_year + 1900,
                         0);
      SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINPACE, 1, 1);

      SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
      SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINRANGE, 1, 12);
      SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINVALUE, tm->tm_mon + 1, 0);
      SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINPACE, 1, 2);

      SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
      SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINRANGE, 0, 30);
      SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINVALUE, tm->tm_mday, 0);
      SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINPACE, 1, 3);

      SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
      SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINRANGE, 0, 23);
      SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINVALUE, tm->tm_hour, 0);
      SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINPACE, 1, 4);

      SendDlgItemMessage(hDlg, IDC_MINUTE, CB_SETSPINFORMAT, 0,
                         (LPARAM) "%02d");
      SendDlgItemMessage(hDlg, IDC_MINUTE, CB_SETSPINRANGE, 0, 59);
      SendDlgItemMessage(hDlg, IDC_MINUTE, CB_SETSPINVALUE, tm->tm_min, 0);
      SendDlgItemMessage(hDlg, IDC_MINUTE, CB_SETSPINPACE, 1, 5);

      SendDlgItemMessage(hDlg, IDL_SEC, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
      SendDlgItemMessage(hDlg, IDL_SEC, CB_SETSPINRANGE, 0, 59);
      SendDlgItemMessage(hDlg, IDL_SEC, CB_SETSPINVALUE, tm->tm_sec, 0);
      SendDlgItemMessage(hDlg, IDL_SEC, CB_SETSPINPACE, 1, 6);
      hNewFocus = GetNextDlgTabItem(hDlg, hCurFocus, FALSE);
      SetNullFocus(hCurFocus);
      SetFocus(hNewFocus);
      return 0;
    }
    case MSG_COMMAND: {
      break;
    }
    case MSG_KEYDOWN: {
      switch (wParam) {
        case SCANCODE_ESCAPE:
          EndDialog(hDlg, wParam);
          break;
        case SCANCODE_MENU:
          EndDialog(hDlg, wParam);
          break;
        case SCANCODE_ENTER: {
          prompt(hDlg);
          EndDialog(hDlg, wParam);
          break;
        }
      }
      break;
    }
  }

  return DefaultDialogProc(hDlg, message, wParam, lParam);
}

// param == -1, format fail; param == 0, format success
int sdcardformat_back(void* arg, int param) {
  HWND hWnd = (HWND)arg;
  printf("%s\n", __func__);
  if (wififormat_callback != NULL) {
    wififormat_callback();
    FlagForUI_ca.formatflag = 0;
    PostMessage(hWnd, MSG_SDCHANGE, 0, 1);
    return;
  }
  if (param < 0)
    PostMessage(hWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FAIL);
  else 
    PostMessage(hWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FINISH);
}

int sdcardformat_notifyback(void* arg, int param) {
  HWND hWnd = (HWND)arg;
  printf("%s\n", __func__);
  if (wififormat_callback != NULL) {
    wififormat_callback();
    FlagForUI_ca.formatflag = 0;
    PostMessage(hWnd, MSG_SDCHANGE, 0, 1);
    return;
  }
  if (param < 0)
    PostMessage(hWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FAIL);
  else {
    PostMessage(hWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FINISH);
    PostMessage(hWnd, MSG_SDCHANGE, 0, 1);
  }
}

static void proc_MSG_FS_INITFAIL(HWND hWnd, int param);
int sdcardmount_back(void* arg, int param) {
  HWND hWnd = (HWND)arg;
  printf("sdcardmount_back param = %d\n", param);

  if(param == 0){
    PostMessage(hWnd, MSG_REPAIR, 0, 1);
  } else if(param == 1) {
    PostMessage(hWnd, MSG_FSINITFAIL, 0, 1);
  }
}

//-----------------

static int UsbSDProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_INITDIALOG: {
			HWND hFocus = GetDlgDefPushButton(hDlg);
			hWndUSB = hDlg;

			if (hFocus)
				SetFocus(hFocus);
			return 0;
		}
		case MSG_COMMAND: {
			switch (wParam) {
				case IDUSB: {
					printf("  UsbSDProc :case IDUSB:IDUSB  \n");
					if (sdcard == 1) {
						//	printf("UsbSDProc:");
						cvr_usb_sd_ctl(USB_CTRL, 1);
						EndDialog(hDlg, wParam);
						// hWndUSB = 0;
						changemode(hMainWnd, MODE_USBCONNECTION);
					} else if (sdcard == 0) {
						//	EndDialog(hDlg, wParam);
						if (parameter_get_video_lan() == 1)
							MessageBox(hDlg, "SD卡不存在!!!", "警告!!!", MB_OK);
						else if (parameter_get_video_lan() == 0)
							MessageBox(hDlg, "no SD card insertted!!!", "Warning!!!", MB_OK);
					}

					break;
				}
				case USBBAT: {
					printf("  UsbSDProc :case USBBAT  \n");
					cvr_usb_sd_ctl(USB_CTRL, 0);
					EndDialog(hDlg, wParam);
					hWndUSB = 0;
					changemode(hMainWnd, MODE_RECORDING);
					break;
				}
			}
			break;
		}
	}

	return DefaultDialogProc(hDlg, message, wParam, lParam);
}

//-------sys log
static int getResultFromSystemCall(const char* pCmd, char* pResult, int size) {
  int fd[2];
  if (pipe(fd)) {
    return -1;
  }
  // prevent content in stdout affect result
  fflush(stdout);
  // hide stdout
  int bak_fd = dup(STDOUT_FILENO);
  int new_fd = dup2(fd[1], STDOUT_FILENO);
  // the output of `pCmd` is write into fd[1]
  system(pCmd);
  printf("system!\n");
  read(fd[0], pResult, size - 1);
  printf("read!\n");
  pResult[strlen(pResult) - 1] = 0;

  // resume stdout
  dup2(bak_fd, new_fd);
  printf("--dup2!\n");

  return 0;
}

void ui_set_white_balance(int i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_white_balance(i);
  parameter_save_wb(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_set_exposure_compensation(int i) {
  pthread_mutex_lock(&set_mutex);
  video_record_set_exposure_compensation(i);
  parameter_save_ex(i);
  pthread_mutex_unlock(&set_mutex);
}

void ui_deinit_init_camera(HWND hWnd, char i, char j) {
  pthread_mutex_lock(&set_mutex);
  video_record_get_front_resolution(frontcamera, 4);
  video_record_get_back_resolution(backcamera, 4);
  if (i >= 0 && i < 4) {
    printf("parameter_save_video_frontcamera_all:%d %d %d %d\n", i,
           frontcamera[i].width, frontcamera[i].height, frontcamera[i].fps);
    parameter_save_video_frontcamera_all(
        i, frontcamera[i].width, frontcamera[i].height, frontcamera[i].fps);

    if ((frontcamera[i].width == 1280) && (frontcamera[i].height == 720)) {
      printf("parameter_save_vcamresolution(0)\n");
      parameter_save_vcamresolution(0);
    } else if ((frontcamera[i].width == 1920) &&
               (frontcamera[i].height == 1080)) {
      printf("parameter_save_vcamresolution(1)\n");
      parameter_save_vcamresolution(1);
    }
  }

  if (j >= 0 && j < 4) {
    parameter_save_video_backcamera_all(
        j, backcamera[j].width, backcamera[j].height, backcamera[j].fps);
  }

  printf("stoprec(hWnd)\n");
  deinitrec(hWnd);
  printf("InvalidateRect(hWnd, &msg_rcResolution, FALSE);\n");
  InvalidateRect(hWnd, &msg_rcResolution, FALSE);
  printf("video_record_deinit\n");
  video_record_deinit();
  printf("initrec\n");
  initrec(hWnd);
  printf("pthread_mutex_unlock(&set_mutex)\n");
  pthread_mutex_unlock(&set_mutex);
}

void cmd_IDM_SETDATE(void) {
  DLGTEMPLATE DlgMyDate = {WS_VISIBLE,
                           WS_EX_NONE,
                           (WIN_W - 350) / 2,
                           (WIN_H - 110) / 2,
                           350,
                           110,
                           SetDate,
                           0,
                           0,
                           14,
                           NULL,
                           0};

  CTRLDATA CtrlMyDate[] = {
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 100, 8, 150, 30, IDC_STATIC,
       "日期设置", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 0, 45, 350, 2, IDC_STATIC,
       "-----", 0},
      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 60, 48,
       80, 20, IDL_YEAR, "", 0},

      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 140, 50, 20, 20, IDC_STATIC,
       "年", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 160,
       48, 40, 20, IDL_MONTH, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 200, 50, 20, 20, IDC_STATIC,
       "月", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 220,
       48, 40, 20, IDL_DAY, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 260, 50, 20, 20, IDC_STATIC,
       "日", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 75, 78,
       40, 20, IDC_HOUR, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 115, 80, 20, 20, IDC_STATIC,
       "时", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 135,
       78, 40, 20, IDC_MINUTE, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 175, 80, 20, 20, IDC_STATIC,
       "分", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 195,
       78, 40, 20, IDL_SEC, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 235, 80, 20, 20, IDC_STATIC,
       "秒", 0},
  };

  CTRLDATA CtrlMyDate_en[] = {
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 100, 8, 150, 30, IDC_STATIC,
       "Date Set", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 0, 45, 350, 2, IDC_STATIC,
       "-----", 0},
      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 60, 48,
       80, 20, IDL_YEAR, "", 0},

      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 140, 50, 20, 20, IDC_STATIC,
       "Y", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 160,
       48, 40, 20, IDL_MONTH, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 200, 50, 20, 20, IDC_STATIC,
       "M", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 220,
       48, 40, 20, IDL_DAY, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 260, 50, 20, 20, IDC_STATIC,
       "D", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 75, 78,
       40, 20, IDC_HOUR, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 115, 80, 20, 20, IDC_STATIC,
       "H", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 135,
       78, 40, 20, IDC_MINUTE, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 175, 80, 20, 20, IDC_STATIC,
       "MIN", 0},

      {CTRL_COMBOBOX,
       WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 195,
       78, 40, 20, IDL_SEC, "", 0},
      {"static", WS_CHILD | SS_CENTER | WS_VISIBLE, 235, 80, 20, 20, IDC_STATIC,
       "SEC", 0},
  };

  if (parameter_get_video_lan() == 1)
    DlgMyDate.controls = CtrlMyDate;
  else if (parameter_get_video_lan() == 0)
    DlgMyDate.controls = CtrlMyDate_en;
  DialogBoxIndirectParam(&DlgMyDate, HWND_DESKTOP, MyDateBoxProc, 0L);
}

static void ui_save_vcamresolution_photo(HWND hWnd, char i) {
  parameter_save_vcamresolution_photo(i);
  ui_deinit_init_camera(hWnd, -1, -1);
}

static void cmd_IDM_RECOVERY(HWND hWnd) {
  if (MessageBox(hWnd, "确定要恢复出厂设置?", "警告", MB_YESNO | MB_DEFBUTTON2) == IDYES) {
    parameter_recover();
    runapp("busybox reboot");
  }
}

static void cmd_IDM_FWVER(HWND hWnd) {
  if (parameter_get_video_lan() == 0)
    MessageBox(hWnd, parameter_get_firmware_version(), "FW Version", MB_OK);
  else if (parameter_get_video_lan() == 1)
    MessageBox(hWnd, parameter_get_firmware_version(), "固件版本", MB_OK);
}

void cmd_IDM_CAR(HWND hWnd, char i) {
  parameter_save_abmode(i);
  InvalidateRect(hWnd, &msg_rcAB, FALSE);
  ui_deinit_init_camera(hWnd, -1, -1);
}

static void cmd_IDM_IDC(HWND hWnd, char i) {
  parameter_save_video_idc(i);
}

static void cmd_IDM_3DNR(HWND hWnd, char i) {
  parameter_save_video_3dnr(i);
}

static void cmd_IDM_LDW(HWND hWnd, WPARAM wParam) {
  switch (wParam) {
    case IDM_LDWOFF:
      parameter_save_video_ldw(0);
      ui_deinit_init_camera(hWnd, -1, -1);
      FlagForUI_ca.adasflagtooff = 1;
      InvalidateRect(hWnd, &adas_rc, FALSE);
      break;
    case IDM_LDWON:
      parameter_save_video_ldw(1);
      ui_deinit_init_camera(hWnd, -1, -1);
      break;
    default:
      break;
  }
}

static void cmd_IDM_detect(HWND hWnd, WPARAM wParam, LPARAM lParam) {
  printf("cmd_IDM_detect, wParam: %ld, lParam: %ld\n", wParam, lParam);
  switch (wParam) {
    case IDM_detectOFF:
      if (!lParam && parameter_get_video_de() == 0)
        return;
      stop_motion_detection();
      if (!lParam)
        parameter_save_video_de(0);
      InvalidateRect(hWnd, &msg_rcMove, FALSE);
      break;
    case IDM_detectON:
      if (!lParam && parameter_get_video_de() == 1)
        return;
      if (!lParam)
        parameter_save_video_de(1);
      start_motion_detection(hWnd);
      InvalidateRect(hWnd, &msg_rcMove, FALSE);
      break;
    default:
      break;
  }
}

static void cmd_IDM_language(HWND hWnd, char i) {
  parameter_save_video_lan(i);
  SetSystemLanguage(hWnd, i);
}

void cmd_IDM_frequency(char i) {
  parameter_save_video_fre(i);
  video_record_set_power_line_frequency(i);
}

static void cmd_IDM_YES_DELETE(void) {
  if (parameter_get_video_lan() == 0)
    MessageBox(hMainWndForPlay, "You choose YES!!", "DELETE", MB_OK);
  else if (parameter_get_video_lan() == 1)
    MessageBox(hMainWndForPlay, "你选择是!!", "删除", MB_OK);
}

static void cmd_IDM_NO_DELETE(void) {
  if (parameter_get_video_lan() == 0)
    MessageBox(hMainWndForPlay, "You choose NO!!", "DELETE", MB_OK);
  else if (parameter_get_video_lan() == 1)
    MessageBox(hMainWndForPlay, "你选择否!!", "删除", MB_OK);
}
//if format fail, retry format.
static void cmd_IDM_RE_FORMAT(HWND hWnd) {
  // just need judge mmcblk0
  if (fs_manage_sd_exist(SDCARD_DEVICE) != 0) {
    loadingWaitBmp(hWnd);
    fs_manage_format_sdcard(sdcardformat_back, (void*)hWnd, 1);
  } else {
    if (parameter_get_video_lan() == 1)
      MessageBox(hWnd, "SD卡不存在!!", "警告!!!", MB_OK);
    else if (parameter_get_video_lan() == 0)
      MessageBox(hWnd, "no SD card insertted!!!", "Warning!!!", MB_OK);
  }

  return;
}


static void cmd_IDM_FORMAT(HWND hWnd) {
  int res = 0;
  char resforsys[100] = {0};
  int mesg = 0;
  if (sdcard == 1) {
    if (parameter_get_video_lan() == 0) {
      mesg = MessageBox(hWnd, "disk formatting", "Warning!!!", MB_YESNO | MB_DEFBUTTON2);
    } else if (parameter_get_video_lan() == 1) {
      mesg = MessageBox(hWnd, "磁盘格式化", "警告!!!", MB_YESNO | MB_DEFBUTTON2);
    }
    if (mesg == IDYES) {
      if (sdcard == 1) {
        loadingWaitBmp(hWnd);
        fs_manage_format_sdcard(sdcardformat_back, (void*)hWnd, 0);
      } else if (sdcard == 0) {
        if (parameter_get_video_lan() == 1)
          MessageBox(hWnd, "SD卡不存在!!", "警告!!!", MB_OK);
        else if (parameter_get_video_lan() == 0)
          MessageBox(hWnd, "no SD card insertted!!!", "Warning!!!", MB_OK);
      }
    }
  } else if (sdcard == 0) {
    if (parameter_get_video_lan() == 1)
      MessageBox(hWnd, "SD卡不存在!!", "警告!!!", MB_OK);
    else if (parameter_get_video_lan() == 0)
      MessageBox(hWnd, "no SD card insertted!!!", "Warning!!!", MB_OK);
  }
}

static void cmd_IDM_BACKLIGHT(WPARAM wParam) {
  switch (wParam) {
    case IDM_BACKLIGHT_L:
      parameter_save_video_backlt(LCD_BACKLT_L);
      rk_fb_set_lcd_backlight(LCD_BACKLT_L);
      break;
    case IDM_BACKLIGHT_M:
      parameter_save_video_backlt(LCD_BACKLT_M);
      rk_fb_set_lcd_backlight(LCD_BACKLT_M);
      break;
    case IDM_BACKLIGHT_H:
      parameter_save_video_backlt(LCD_BACKLT_H);
      rk_fb_set_lcd_backlight(LCD_BACKLT_H);
      break;
    default:
      break;
  }
}

static void cmd_IDM_DEBUG_PHOTO_ON(HWND hWnd) {
  if (video_rec_state == 0 && sdcard == 1)
    startrec(hWnd);
  parameter_save_debug_photo(1);
}

void cmd_IDM_DEBUG_VIDEO_BIT_RATE(HWND hWnd, unsigned int val) {
  val = (val-1) * 2;
  if (val == 0)
      val = 1;
  if (val == parameter_get_bit_rate_per_pixel())
    return;
  if (video_rec_state)
    stoprec(hWnd);
  printf("change bit rate per pixel to < %u >\n", val);
  parameter_save_bit_rate_per_pixel(val);
  video_record_reset_bitrate();
}

static void cmd_IDM_USB(void) {
  parameter_save_video_usb(0);
  android_usb_config_ums();
  printf("usb mode \n");
}

static void cmd_IDM_ADB(void) {
  parameter_save_video_usb(1);
  android_usb_config_adb();
  printf("adb mode \n");
}

static void cmd_IDM_DEBUG_PHOTO_OFF(HWND hWnd) {
  if (video_rec_state != 0)
    stoprec(hWnd);
  parameter_save_debug_photo(0);
}

static void cmd_IDM_DEBUG_TEMP_ON(HWND hWnd) {
  parameter_save_debug_temp(1);
}

static void cmd_IDM_DEBUG_TEMP_OFF(HWND hWnd) {
  parameter_save_debug_temp(0);
}

//---------------MSG_COMMAND消息处理函数---------------------
static int commandEvent(HWND hWnd, WPARAM wParam, LPARAM lParam) {
  struct ui_frame ui_720P = UI_FRAME_720P;
  struct ui_frame ui_1080P = UI_FRAME_1080P;
  switch (wParam) {
    case IDM_FONT_1:
      ui_deinit_init_camera(hWnd, 0, -1);
      break;
    case IDM_FONT_2:
      ui_deinit_init_camera(hWnd, 1, -1);
      break;
    case IDM_FONT_3:
      ui_deinit_init_camera(hWnd, 2, -1);
      break;
    case IDM_FONT_4:
      ui_deinit_init_camera(hWnd, 3, -1);
      break;
    case IDM_BACK_1:
      ui_deinit_init_camera(hWnd, -1, 0);
      break;
    case IDM_BACK_2:
      ui_deinit_init_camera(hWnd, -1, 1);
      break;
    case IDM_BACK_3:
      ui_deinit_init_camera(hWnd, -1, 2);
      break;
    case IDM_BACK_4:
      ui_deinit_init_camera(hWnd, -1, 3);
      break;
    case IDM_BOOT_OFF:
      parameter_save_debug_reboot(0);
      break;
    case IDM_RECOVERY_OFF:
      parameter_save_debug_recovery(0);
      break;
    case IDM_AWAKE_1_OFF:
      parameter_save_debug_awake(0);
      break;
    case IDM_STANDBY_2_OFF:
      parameter_save_debug_standby(0);
      break;
    case IDM_MODE_CHANGE_OFF:
      parameter_save_debug_modechange(0);
      break;
    case IDM_DEBUG_VIDEO_OFF:
      parameter_save_debug_video(0);
      break;
    case IDM_BEG_END_VIDEO_OFF:
      parameter_save_debug_beg_end_video(0);
      break;
    case IDM_DEBUG_PHOTO_OFF:
      cmd_IDM_DEBUG_PHOTO_OFF(hWnd);
      break;
    case IDM_DEBUG_TEMP_ON:
      cmd_IDM_DEBUG_TEMP_ON(hWnd);
      break;
    case IDM_DEBUG_TEMP_OFF:
	  cmd_IDM_DEBUG_TEMP_OFF(hWnd);
      break;
    case IDM_BOOT_ON:
      parameter_save_debug_reboot(1);
      break;
    case IDM_RECOVERY_ON:
      parameter_save_debug_recovery(1);
      break;
    case IDM_AWAKE_1_ON:
      parameter_save_debug_awake(1);
      break;
    case IDM_STANDBY_2_ON:
      parameter_save_debug_standby(1);
      break;
    case IDM_MODE_CHANGE_ON:
      parameter_save_debug_modechange(1);
      break;
    case IDM_DEBUG_VIDEO_ON:
      parameter_save_debug_video(1);
      break;
    case IDM_BEG_END_VIDEO_ON:
      parameter_save_debug_beg_end_video(1);
      break;
    case IDM_DEBUG_PHOTO_ON:
      cmd_IDM_DEBUG_PHOTO_ON(hWnd);
      break;
    case IDM_DEBUG_VIDEO_BIT_RATE1:
    case IDM_DEBUG_VIDEO_BIT_RATE2:
    case IDM_DEBUG_VIDEO_BIT_RATE4:
    case IDM_DEBUG_VIDEO_BIT_RATE6:
    case IDM_DEBUG_VIDEO_BIT_RATE8:
    case IDM_DEBUG_VIDEO_BIT_RATE10:
    case IDM_DEBUG_VIDEO_BIT_RATE12:
      cmd_IDM_DEBUG_VIDEO_BIT_RATE(hWnd, wParam - IDM_DEBUG_VIDEO_BIT_RATE);
      break;
    case IDM_1M_ph:
      ui_save_vcamresolution_photo(hWnd, 0);
      break;
    case IDM_2M_ph:
      ui_save_vcamresolution_photo(hWnd, 1);
      break;
    case IDM_3M_ph:
      ui_save_vcamresolution_photo(hWnd, 2);
      break;
    case IDM_RECOVERY:
      cmd_IDM_RECOVERY(hWnd);
      break;
    case IDM_FWVER:
      cmd_IDM_FWVER(hWnd);
      break;
    case IDM_USB:
      cmd_IDM_USB();
      break;
    case IDM_ADB:
      cmd_IDM_ADB();
      break;
    case IDM_ABOUT_CAR:
      break;
    case IDM_ABOUT_TIME:
      break;
    case IDM_1MIN:
      parameter_save_recodetime(60);
      break;
    case IDM_3MIN:
      parameter_save_recodetime(180);
      break;
    case IDM_5MIN:
      parameter_save_recodetime(300);
      break;
    case IDM_CAR1:
      cmd_IDM_CAR(hWnd, 0);
      break;
    case IDM_CAR2:
      cmd_IDM_CAR(hWnd, 1);
      break;
    case IDM_CAR3:
      cmd_IDM_CAR(hWnd, 2);
      break;
    case IDM_IDCOFF:
      cmd_IDM_IDC(hWnd, 0);
      break;
    case IDM_IDCON:
      cmd_IDM_IDC(hWnd, 1);
      break;
    case IDM_3DNROFF:
      cmd_IDM_3DNR(hWnd, 0);
      break;
    case IDM_3DNRON:
      cmd_IDM_3DNR(hWnd, 1);
      break;
    case IDM_LDWOFF:
      cmd_IDM_LDW(hWnd, wParam);
      break;
    case IDM_LDWON:
      cmd_IDM_LDW(hWnd, wParam);
      break;
    case IDM_bright1:
      ui_set_white_balance(0);
      break;
    case IDM_bright2:
      ui_set_white_balance(1);
      break;
    case IDM_bright3:
      ui_set_white_balance(2);
      break;
    case IDM_bright4:
      ui_set_white_balance(3);
      break;
    case IDM_bright5:
      ui_set_white_balance(4);
      break;
    case IDM_exposal1:
      ui_set_exposure_compensation(0);
      break;
    case IDM_exposal2:
      ui_set_exposure_compensation(1);
      break;
    case IDM_exposal3:
      ui_set_exposure_compensation(2);
      break;
    case IDM_exposal4:
      ui_set_exposure_compensation(3);
      break;
    case IDM_exposal5:
      ui_set_exposure_compensation(4);
      break;
    case IDM_detectON:
    case IDM_detectOFF:
#if ENABLE_MD_IN_MENU
      cmd_IDM_detect(hWnd, wParam, lParam);
#endif
      break;
    case IDM_markOFF:
      parameter_save_video_mark(0);
      InvalidateRect (hWnd, &msg_rcWatermarkTime, TRUE);
      InvalidateRect (hWnd, &msg_rcWatermarkImg, TRUE);
      break;
    case IDM_markON:
      parameter_save_video_mark(1);
      break;
    case IDM_recordOFF:
      mic_onoff(hWnd, 0);
      break;
    case IDM_recordON:
      mic_onoff(hWnd, 1);
      break;
    case IDM_AUTOOFFSCREENON:
      screenoff_time = 15;
      parameter_save_screenoff_time(15);  // 15S
      break;
    case IDM_AUTOOFFSCREENOFF:
      screenoff_time = -1;
      parameter_save_screenoff_time(-1);
      break;
    case IDM_autorecordOFF:
      parameter_save_video_autorec(0);
      break;
    case IDM_autorecordON:
      parameter_save_video_autorec(1);
	  break;
    case IDM_WIFIOFF:
      parameter_save_wifi_en(0);
	  wifi_management_stop();
      break;
    case IDM_WIFION:
      parameter_save_wifi_en(1);
	  wifi_management_start();
      break;
    case IDM_langEN:
      cmd_IDM_language(hWnd, 0);
      break;
    case IDM_langCN:
      cmd_IDM_language(hWnd, 1);
      break;
    case IDM_50HZ:
      cmd_IDM_frequency(CAMARE_FREQ_50HZ);
      break;
    case IDM_60HZ:
      cmd_IDM_frequency(CAMARE_FREQ_60HZ);
      break;
    case IDM_YES_DELETE:
      cmd_IDM_YES_DELETE();
      break;
    case IDM_NO_DELETE:
      cmd_IDM_NO_DELETE();
      break;
    case IDM_1M:
      SetCameraMP = 0;
      break;
    case IDM_2M:
      SetCameraMP = 1;
      break;
    case IDM_3M:
      SetCameraMP = 2;
      break;
    case IDM_FORMAT:
      cmd_IDM_FORMAT(hWnd);
      break;
    case IDM_SETDATE:
      cmd_IDM_SETDATE();
      break;
    case IDM_BACKLIGHT_L:
      cmd_IDM_BACKLIGHT(wParam);
      break;
    case IDM_BACKLIGHT_M:
      cmd_IDM_BACKLIGHT(wParam);
      break;
    case IDM_BACKLIGHT_H:
      cmd_IDM_BACKLIGHT(wParam);
      break;
    case IDM_ABOUT_SETTING:
      break;
    case IDM_COLLISION_NO:
      parameter_save_collision_level(COLLI_CLOSE);
      break;
    case IDM_COLLISION_L:
      parameter_save_collision_level(COLLI_LEVEL_L);
      break;
    case IDM_COLLISION_M:
      parameter_save_collision_level(COLLI_LEVEL_M);
      break;
    case IDM_COLLISION_H:
      parameter_save_collision_level(COLLI_LEVEL_H);
      break;
    case IDM_LEAVEREC_OFF:
      parameter_save_leavecarrec(0);
      break;
    case IDM_LEAVEREC_ON:
      parameter_save_leavecarrec(1);
      break;
    case IDM_QUIT:
      DestroyMainWindow(hWnd);
      PostQuitMessage(hWnd);
      return 0;
  }

  return 1;
}

//----碰撞检测菜单实现-----------
static HMENU createpmenufileCOLLISION() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)collision[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)collision[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForCOLLI = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_collision_level() == COLLI_CLOSE) ? MFS_CHECKED
                                                               : MFS_UNCHECKED;
  mii.id = IDM_COLLISION_NO;
  mii.typedata = (DWORD)Collision_0[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Collision_0[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_collision_level() == COLLI_LEVEL_L)
                  ? MFS_CHECKED
                  : MFS_UNCHECKED;
  mii.id = IDM_COLLISION_L;
  mii.typedata = (DWORD)Collision_1[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Collision_1[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 3, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_collision_level() == COLLI_LEVEL_M)
                  ? MFS_CHECKED
                  : MFS_UNCHECKED;
  mii.id = IDM_COLLISION_M;
  mii.typedata = (DWORD)Collision_2[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Collision_2[i];

  InsertMenuItem(hmnu, 4, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 5, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_collision_level() == COLLI_LEVEL_H)
                  ? MFS_CHECKED
                  : MFS_UNCHECKED;
  mii.id = IDM_COLLISION_H;
  mii.typedata = (DWORD)Collision_3[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Collision_3[i];

  InsertMenuItem(hmnu, 6, TRUE, &mii);

  return hmnu;
}

//----停车录像菜单实现-----------
static HMENU createpmenufileLEAVECARREC() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)leavecarrec[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)leavecarrec[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForLEAVEREC = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_leavecarrec() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_LEAVEREC_OFF;
  mii.typedata = (DWORD)Leavecarrec_Off[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Leavecarrec_Off[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_leavecarrec() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_LEAVEREC_ON;
  mii.typedata = (DWORD)Leavecarrec_On[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Leavecarrec_On[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

//----频率菜单实现-----------
static HMENU createpmenufileFREQUENCY() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)frequency[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)frequency[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForFRE = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_fre() == CAMARE_FREQ_50HZ) ? MFS_CHECKED
                                                              : MFS_UNCHECKED;
  mii.id = IDM_50HZ;
  mii.typedata = (DWORD)HzNum_50[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)HzNum_50[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_fre() == CAMARE_FREQ_60HZ) ? MFS_CHECKED
                                                              : MFS_UNCHECKED;
  mii.id = IDM_60HZ;
  mii.typedata = (DWORD)HzNum_60[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)HzNum_60[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}
//----背光亮度菜单实现-----------
static HMENU createpmenufileBACKLIGHT() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;
  HMENU hmnuForBACKLT;

  char BackLight_1[2][20] = {" low ", " 低 "};
  char BackLight_2[2][20] = {" middle ", " 中 "};
  char BackLight_3[2][20] = {" hight ", " 高 "};

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)backlight[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)backlight[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForBACKLT = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_backlt() == LCD_BACKLT_L) ? MFS_CHECKED
                                                             : MFS_UNCHECKED;
  mii.id = IDM_BACKLIGHT_L;
  mii.typedata = (DWORD)BackLight_1[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)BackLight_1[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_backlt() == LCD_BACKLT_M) ? MFS_CHECKED
                                                             : MFS_UNCHECKED;
  mii.id = IDM_BACKLIGHT_M;
  mii.typedata = (DWORD)BackLight_2[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)BackLight_2[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 3, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_backlt() == LCD_BACKLT_H) ? MFS_CHECKED
                                                             : MFS_UNCHECKED;
  mii.id = IDM_BACKLIGHT_H;
  mii.typedata = (DWORD)BackLight_3[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)BackLight_3[i];

  InsertMenuItem(hmnu, 4, TRUE, &mii);

  return hmnu;
}
static HMENU createpmenufileUSB() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)usbmode[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)usbmode[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForUSB = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_usb() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_USB;
  mii.typedata = (DWORD)UsbMsc[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)UsbMsc[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_usb() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_ADB;
  mii.typedata = (DWORD)UsbAdb[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)UsbAdb[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

//------------语言选择菜单实现-----------------
static HMENU createpmenufileLANGUAGE() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)language[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)language[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForLAN = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_lan() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_langEN;
  mii.typedata = (DWORD)LAN_ENG[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)LAN_ENG[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_lan() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_langCN;
  mii.typedata = (DWORD)LAN_CHN[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)LAN_CHN[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

//---------------屏幕自动关闭菜单实现-------------------
static HMENU createpmenufileWIFIONOFF() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)wifi[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)wifi[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_wifi_en() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_WIFIOFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_wifi_en() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_WIFION;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

//---------------屏幕自动关闭菜单实现-------------------
static HMENU createpmenufileAUTOOFFSCREEN() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)screenoffset[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)screenoffset[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForAutoOffScreen = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_screenoff_time() == -1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_AUTOOFFSCREENOFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_screenoff_time() > 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_AUTOOFFSCREENON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

//---------------开机录像菜单实现-------------------
static HMENU createpmenufileAUTORECORD() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)auto_record[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)auto_record[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForAutoRE = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_video_autorec() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_autorecordOFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_video_autorec() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_autorecordON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

static HMENU createpmenufileIDC() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)DSP_IDC[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)DSP_IDC[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuFor3DNR = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_idc() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_IDCOFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;
  mii.state = 0;
  mii.id = 0;
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_idc() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_IDCON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

//---------3DNR-------------------------
static HMENU createpmenufile3DNR() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)DSP_3DNR[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)DSP_3DNR[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuFor3DNR = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_3dnr() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_3DNROFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_3dnr() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_3DNRON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

// font camera
static HMENU createpmenufileFONTCAMERA() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;
  int cnt = 0;
  char mpstr[10] = {0};
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)fontcamera_ui[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)fontcamera_ui[i];

  hmnu = CreatePopupMenu(&mii);

  if (!video_record_get_front_resolution(frontcamera, 4)) {
    if ((frontcamera[0].width > 0) && (frontcamera[0].height > 0)) {
      memset(&mpstr, 0, 10);
      sprintf(mpstr, "%d * %d", frontcamera[0].width, frontcamera[0].height);
      memset(&mii, 0, sizeof(MENUITEMINFO));
      mii.type = MFT_RADIOCHECK;
      mii.state =
          (parameter_get_video_fontcamera() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
      mii.id = IDM_FONT_1;
      mii.typedata = (DWORD)mpstr;

      for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = mpstr;

      InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    }
    if ((frontcamera[1].width > 0) && (frontcamera[1].height > 0)) {
      if (cnt == 1) {
        mii.type = MFT_SEPARATOR;  //类型
        mii.state = 0;
        mii.id = 0;  // ID
        mii.typedata = 0;
        InsertMenuItem(hmnu, cnt++, TRUE, &mii);
      }

      memset(&mpstr, 0, 10);
      sprintf(mpstr, "%d * %d", frontcamera[1].width, frontcamera[1].height);
      memset(&mii, 0, sizeof(MENUITEMINFO));
      mii.type = MFT_RADIOCHECK;
      mii.state =
          (parameter_get_video_fontcamera() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
      mii.id = IDM_FONT_2;
      mii.typedata = (DWORD)mpstr;

      for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = mpstr;

      InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    }
    if ((frontcamera[2].width > 0) && (frontcamera[2].height > 0)) {
      if (cnt >= 1) {
        mii.type = MFT_SEPARATOR;  //类型
        mii.state = 0;
        mii.id = 0;  // ID
        mii.typedata = 0;
        InsertMenuItem(hmnu, cnt++, TRUE, &mii);
      }

      memset(&mpstr, 0, 10);
      sprintf(mpstr, "%d * %d", frontcamera[2].width, frontcamera[2].height);
      memset(&mii, 0, sizeof(MENUITEMINFO));
      mii.type = MFT_RADIOCHECK;
      mii.state =
          (parameter_get_video_fontcamera() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
      mii.id = IDM_FONT_3;
      mii.typedata = (DWORD)mpstr;

      for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = mpstr;

      InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    }
    if ((frontcamera[3].width > 0) && (frontcamera[3].height > 0)) {
      if (cnt >= 1) {
        mii.type = MFT_SEPARATOR;  //类型
        mii.state = 0;
        mii.id = 0;  // ID
        mii.typedata = 0;
        InsertMenuItem(hmnu, cnt++, TRUE, &mii);
      }

      memset(&mpstr, 0, 10);
      sprintf(mpstr, "%d * %d", frontcamera[3].width, frontcamera[3].height);

      memset(&mii, 0, sizeof(MENUITEMINFO));
      mii.type = MFT_RADIOCHECK;
      mii.state =
          (parameter_get_video_fontcamera() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
      mii.id = IDM_FONT_4;
      mii.typedata = (DWORD)mpstr;

      for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = mpstr;

      InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    }
  } else {
    //			hmnu = NULL;
  }
  return hmnu;
}

static HMENU createpmenufileBACKCAMERA() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;
  int cnt = 0;
  char mpstr[10] = {0};
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)backcamera_ui[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)backcamera_ui[i];

  hmnu = CreatePopupMenu(&mii);

  if (!video_record_get_back_resolution(backcamera, 4)) {
    if ((backcamera[0].width > 0) && (backcamera[0].height > 0)) {
      memset(&mpstr, 0, 10);
      sprintf(mpstr, "%d * %d", backcamera[0].width, backcamera[0].height);
      memset(&mii, 0, sizeof(MENUITEMINFO));
      mii.type = MFT_RADIOCHECK;
      mii.state =
          (parameter_get_video_backcamera() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
      mii.id = IDM_BACK_1;
      mii.typedata = (DWORD)mpstr;

      for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = mpstr;

      InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    }
    if ((backcamera[1].width > 0) && (backcamera[1].height > 0)) {
      if (cnt == 1) {
        mii.type = MFT_SEPARATOR;  //类型
        mii.state = 0;
        mii.id = 0;  // ID
        mii.typedata = 0;
        InsertMenuItem(hmnu, cnt++, TRUE, &mii);
      }

      memset(&mpstr, 0, 10);
      sprintf(mpstr, "%d * %d", backcamera[1].width, backcamera[1].height);
      memset(&mii, 0, sizeof(MENUITEMINFO));
      mii.type = MFT_RADIOCHECK;
      mii.state =
          (parameter_get_video_backcamera() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
      mii.id = IDM_BACK_2;
      mii.typedata = (DWORD)mpstr;

      for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = mpstr;

      InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    }
    if ((backcamera[2].width > 0) && (backcamera[2].height > 0)) {
      if (cnt >= 1) {
        mii.type = MFT_SEPARATOR;  //类型
        mii.state = 0;
        mii.id = 0;  // ID
        mii.typedata = 0;
        InsertMenuItem(hmnu, cnt++, TRUE, &mii);
      }

      memset(&mpstr, 0, 10);
      sprintf(mpstr, "%d * %d", backcamera[2].width, backcamera[2].height);
      memset(&mii, 0, sizeof(MENUITEMINFO));
      mii.type = MFT_RADIOCHECK;
      mii.state =
          (parameter_get_video_backcamera() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
      mii.id = IDM_BACK_3;
      mii.typedata = (DWORD)mpstr;

      for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = mpstr;

      InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    }
    if ((backcamera[3].width > 0) && (backcamera[3].height > 0)) {
      if (cnt >= 1) {
        mii.type = MFT_SEPARATOR;  //类型
        mii.state = 0;
        mii.id = 0;  // ID
        mii.typedata = 0;
        InsertMenuItem(hmnu, cnt++, TRUE, &mii);
      }

      memset(&mpstr, 0, 10);
      sprintf(mpstr, "%d * %d", backcamera[3].width, backcamera[3].height);

      memset(&mii, 0, sizeof(MENUITEMINFO));
      mii.type = MFT_RADIOCHECK;
      mii.state =
          (parameter_get_video_backcamera() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
      mii.id = IDM_BACK_4;
      mii.typedata = (DWORD)mpstr;

      for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = mpstr;

      InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    }
  } else {
    //			hmnu = NULL;
  }

  return hmnu;
}

// REBOOT
static HMENU createpmenufileREBOOT() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)reboot[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)reboot[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_reboot() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_BOOT_OFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_reboot() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_BOOT_ON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}
static HMENU createpmenufileRECOVERY() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)recovery_debug[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)recovery_debug[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_debug_recovery() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_RECOVERY_OFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_debug_recovery() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_RECOVERY_ON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}
static HMENU createpmenufileAWAKE() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)awake[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)awake[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_awake() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_AWAKE_1_OFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_awake() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_AWAKE_1_ON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

static HMENU createpmenufileSTANDBY() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)standby[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)standby[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_debug_standby() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_STANDBY_2_OFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_debug_standby() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_STANDBY_2_ON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

static HMENU createpmenufileMODECHANGE() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)mode_change[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)mode_change[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_debug_modechange() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_MODE_CHANGE_OFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_debug_modechange() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_MODE_CHANGE_ON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}
static HMENU createpmenufileDEBUGVIDEO() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)video_debug[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)video_debug[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_video() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_DEBUG_VIDEO_OFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_video() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_DEBUG_VIDEO_ON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}
static HMENU createpmenufileBEGENDVIDEO() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)beg_end_video[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)beg_end_video[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_debug_beg_end_video() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_BEG_END_VIDEO_OFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_debug_beg_end_video() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_BEG_END_VIDEO_ON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}
static HMENU createpmenufileDEBUGPHOTO() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)photo_debug[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)photo_debug[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_photo() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_DEBUG_PHOTO_OFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_photo() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_DEBUG_PHOTO_ON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

static HMENU createpmenufileDEBUGTEMP() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)temp_debug[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)temp_debug[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_temp() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_DEBUG_TEMP_OFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_debug_temp() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_DEBUG_TEMP_ON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

inline static void add_seperator_line(HMENU hmnu, MENUITEMINFO* mii, int* cnt) {
  mii->type = MFT_SEPARATOR;  //类型
  mii->state = 0;
  mii->id = 0;  // ID
  mii->typedata = 0;
  InsertMenuItem(hmnu, *cnt, TRUE, mii);
  *cnt += 1;
}

#define ADD_SUB_VBR_MENU(INDEX, VAL)                                       \
  memset(&mii, 0, sizeof(MENUITEMINFO));                                   \
  mii.type = MFT_RADIOCHECK;                                               \
  mii.state = (parameter_get_bit_rate_per_pixel() == VAL) ? MFS_CHECKED    \
                                                          : MFS_UNCHECKED; \
  mii.id = IDM_DEBUG_VIDEO_BIT_RATE + INDEX + 1;                           \
  mii.typedata = (DWORD)vbit_rate_vals[INDEX];                             \
  mii.str[0] = (char*)vbit_rate_vals[INDEX];                               \
  InsertMenuItem(hmnu, cnt++, TRUE, &mii)

static HMENU createpmenufileDEBUGVideoBitRate() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;
  int cnt = 0;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)vbit_rate_debug[0];

  for (i = 0; i < 2; i++)
    mii.str[i] = (char*)vbit_rate_debug[i];

  hmnu = CreatePopupMenu(&mii);

  for (i = 0; i < sizeof(vbit_rate_vals) / sizeof(vbit_rate_vals[0]); i++) {
    int val = atoi(vbit_rate_vals[i]);
    add_seperator_line(hmnu, &mii, &cnt);
    ADD_SUB_VBR_MENU(i, val);
  }

  return hmnu;
}

//---------3DNR-------------------------
static HMENU createpmenufileLDW() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)DSP_LDW[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)DSP_LDW[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForLDW = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_ldw() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_LDWOFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_ldw() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_LDWON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

//?-------------------白平衡菜单实现-------------
static HMENU createpmenufileWB() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)WB[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)WB[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForBr = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_wb() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_bright1;
  mii.typedata = (DWORD)Br_Auto[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Br_Auto[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_wb() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_bright2;
  mii.typedata = (DWORD)Br_Sun[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Br_Sun[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 3, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_wb() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_bright3;
  mii.typedata = (DWORD)Br_Flu[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Br_Flu[i];

  InsertMenuItem(hmnu, 4, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 5, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_wb() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_bright4;
  mii.typedata = (DWORD)Br_Cloud[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Br_Cloud[i];

  InsertMenuItem(hmnu, 6, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 7, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_wb() == 4) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_bright5;
  mii.typedata = (DWORD)Br_tun[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Br_tun[i];

  InsertMenuItem(hmnu, 8, TRUE, &mii);

  return hmnu;
}
//-----曝光度-------------------------
static HMENU createpmenufileEXPOSAL() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)exposal[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)exposal[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForEx = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_ex() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_exposal1;
  mii.typedata = (DWORD)ExposalData_0[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ExposalData_0[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_ex() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_exposal2;
  mii.typedata = (DWORD)ExposalData_1[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ExposalData_1[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 3, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_ex() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_exposal3;
  mii.typedata = (DWORD)ExposalData_2[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ExposalData_2[i];

  InsertMenuItem(hmnu, 4, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 5, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_ex() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_exposal4;
  mii.typedata = (DWORD)ExposalData_3[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ExposalData_3[i];

  InsertMenuItem(hmnu, 6, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 7, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_ex() == 4) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_exposal5;
  mii.typedata = (DWORD)ExposalData_4[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ExposalData_4[i];

  InsertMenuItem(hmnu, 8, TRUE, &mii);

  return hmnu;
}
//------------录音----------------------------
static HMENU createpmenufileRECORD() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)record[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)record[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForRe = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_audio() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_recordOFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_audio() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_recordON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}
//---------时间水印-------------------------
static HMENU createpmenufileMARK() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)water_mark[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)water_mark[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForMark = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_mark() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_markOFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_mark() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_markON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

#if ENABLE_MD_IN_MENU
//--------------------移动侦测---------------------
static HMENU createpmenufileDETECT() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)auto_detect[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)auto_detect[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForDe = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_de() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_detectOFF;
  mii.typedata = (DWORD)OFF[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)OFF[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_video_de() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_detectON;
  mii.typedata = (DWORD)ON[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)ON[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}
#endif

static HMENU createpmenufileMP_photo() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)MP[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MP[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_vcamresolution_photo() == 0) ? MF_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_1M_ph;
  mii.typedata = (DWORD)MPVed_1M[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MPVed_1M[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_vcamresolution_photo() == 1) ? MF_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_2M_ph;
  mii.typedata = (DWORD)MPVed_2M[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MPVed_2M[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 3, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state =
      (parameter_get_vcamresolution_photo() == 2) ? MF_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_3M_ph;
  mii.typedata = (DWORD)MPVed_3M[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MPVed_3M[i];

  InsertMenuItem(hmnu, 4, TRUE, &mii);

  return hmnu;
}
//------------清晰度菜单--------
static HMENU createpmenufileMP() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;
  int cnt = 0;
  int j = 0;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)MP[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MP[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_FONTCAMERA;
  mii.typedata = (DWORD)fontcamera_ui[0];
  mii.hsubmenu = createpmenufileFONTCAMERA();
  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)fontcamera_ui[i];
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_BACKCAMERA;
  mii.typedata = (DWORD)backcamera_ui[0];
  mii.hsubmenu = createpmenufileBACKCAMERA();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)backcamera_ui[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  return hmnu;
}

//------------清晰度菜单--------

//-----------camera模式清晰度菜单--------
static HMENU createpmenufileMP_camera() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)MP[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MP[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = MF_CHECKED;
  mii.id = IDM_1M;
  mii.typedata = (DWORD)MPCam_1M[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MPCam_1M[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  ;
  mii.state = MFS_UNCHECKED;
  mii.id = IDM_2M;
  mii.typedata = (DWORD)MPCam_2M[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MPCam_2M[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 3, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  ;
  mii.state = MFS_UNCHECKED;
  mii.id = IDM_3M;
  mii.typedata = (DWORD)MPCam_3M[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MPCam_3M[i];

  InsertMenuItem(hmnu, 4, TRUE, &mii);

  return hmnu;
}
//-------------录像时长菜单-----------
static HMENU createpmenufileTIME() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)Record_Time[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Record_Time[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_recodetime() == 60) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_1MIN;
  mii.typedata = (DWORD)Time_OneMIN[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Time_OneMIN[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_recodetime() == 180) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_3MIN;
  mii.typedata = (DWORD)Time_TMIN[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Time_TMIN[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 3, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_recodetime() == 300) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_5MIN;
  mii.typedata = (DWORD)Time_FMIN[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Time_FMIN[i];

  InsertMenuItem(hmnu, 4, TRUE, &mii);
#if 0
	mii.type=MFT_SEPARATOR;//类型	
	mii.state=0;
	mii.id=0;//ID	
	mii.typedata=0;
	InsertMenuItem(hmnu,5,TRUE,&mii);

	memset(&mii,0,sizeof(MENUITEMINFO));
	mii.type=MFT_RADIOCHECK;
	mii.state= (parameter_get_recodetime()==3) ? MFS_CHECKED : MFS_UNCHECKED;
	mii.id=IDM_OFF;
	mii.typedata=(DWORD)OFF[0];

	for (i = 0; i < LANGUAGE_NUM; i++)
		mii.str[i] = (char *)OFF[i];

	InsertMenuItem(hmnu,6,TRUE,&mii);
#endif

  return hmnu;
}
//-----------------------录像模式菜单-------------------------
static HMENU createpmenufileCAR() {
  int i;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)Record_Mode[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Record_Mode[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_abmode() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_CAR1;
  mii.typedata = (DWORD)Front_Record[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Front_Record[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_abmode() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_CAR2;
  mii.typedata = (DWORD)Back_Record[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Back_Record[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 3, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_RADIOCHECK;
  mii.state = (parameter_get_abmode() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
  mii.id = IDM_CAR3;
  mii.typedata = (DWORD)Double_Record[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Double_Record[i];

  InsertMenuItem(hmnu, 4, TRUE, &mii);

  return hmnu;
}

// DEBUG
static HMENU createpmenufileDEBUG() {
  int i;
  int cnt = 0;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)debug[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)debug[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_BOOT;
  mii.typedata = (DWORD)reboot[0];
  mii.hsubmenu = createpmenufileREBOOT();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)reboot[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_RECOVERY_DEBUG;
  mii.typedata = (DWORD)recovery_debug[0];
  mii.hsubmenu = createpmenufileRECOVERY();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)recovery_debug[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_AWAKE_1;
  mii.typedata = (DWORD)awake[0];
  mii.hsubmenu = createpmenufileAWAKE();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)awake[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_STANDBY_2;
  mii.typedata = (DWORD)standby[0];
  mii.hsubmenu = createpmenufileSTANDBY();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)standby[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_MODE_CHANGE;
  mii.typedata = (DWORD)mode_change[0];
  mii.hsubmenu = createpmenufileMODECHANGE();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)mode_change[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_DEBUG_VIDEO;
  mii.typedata = (DWORD)video_debug[0];
  mii.hsubmenu = createpmenufileDEBUGVIDEO();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)video_debug[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_BEG_END_VIDEO;
  mii.typedata = (DWORD)beg_end_video[0];
  mii.hsubmenu = createpmenufileBEGENDVIDEO();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)beg_end_video[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_DEBUG_PHOTO;
  mii.typedata = (DWORD)photo_debug[0];
  mii.hsubmenu = createpmenufileDEBUGPHOTO();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)photo_debug[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_DEBUG_TEMP;
  mii.typedata = (DWORD)temp_debug[0];
  mii.hsubmenu = createpmenufileDEBUGTEMP();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)temp_debug[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  add_seperator_line(hmnu, &mii, &cnt);
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_DEBUG_VIDEO_BIT_RATE;
  mii.typedata = (DWORD)vbit_rate_debug[0];
  mii.hsubmenu = createpmenufileDEBUGVideoBitRate();

  for (i = 0; i < 2; i++)
    mii.str[i] = (char*)vbit_rate_debug[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  return hmnu;
}
//----------------设置菜单----------------------
static HMENU createpmenufileSETTING() {
  int i;
  int cnt = 0;
  HMENU hmnu;
  HMENU hmnuforHead;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)setting[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)setting[i];

  hmnu = CreatePopupMenu(&mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_IDC;
  mii.typedata = (DWORD)DSP_IDC[0];
  mii.hsubmenu = createpmenufileIDC();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)DSP_IDC[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;
  mii.state = 0;
  mii.id = 0;
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  // debug
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_3DNR;
  mii.typedata = (DWORD)DSP_3DNR[0];
  mii.hsubmenu = createpmenufile3DNR();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)DSP_3DNR[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_LDW;
  mii.typedata = (DWORD)DSP_LDW[0];
  mii.hsubmenu = createpmenufileLDW();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)DSP_LDW[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  //
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_bright;
  mii.typedata = (DWORD)WB[0];
  mii.hsubmenu = createpmenufileWB();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)WB[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_exposal;
  mii.typedata = (DWORD)exposal[0];
  mii.hsubmenu = createpmenufileEXPOSAL();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)exposal[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

#if ENABLE_MD_IN_MENU
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_detect;
  mii.typedata = (DWORD)auto_detect[0];
  mii.hsubmenu = createpmenufileDETECT();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)auto_detect[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);
#endif

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_mark;
  mii.typedata = (DWORD)water_mark[0];
  mii.hsubmenu = createpmenufileMARK();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)water_mark[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_record;
  mii.typedata = (DWORD)record[0];
  mii.hsubmenu = createpmenufileRECORD();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)record[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  // createpmenufileAUTORECORD
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_ABOUT_AUTORECORD;
  mii.typedata = (DWORD)auto_record[0];
  mii.hsubmenu = createpmenufileAUTORECORD();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)auto_record[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_ABOUT_LANGUAGE;
  mii.typedata = (DWORD)language[0];
  mii.hsubmenu = createpmenufileLANGUAGE();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)language[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_ABOUT_FREQUENCY;
  mii.typedata = (DWORD)frequency[0];
  mii.hsubmenu = createpmenufileFREQUENCY();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)frequency[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  //---------------------屏幕自动关闭--
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_AUTOOFFSCREEN;
  mii.typedata = (DWORD)screenoffset[0];
  mii.hsubmenu = createpmenufileAUTOOFFSCREEN();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)screenoffset[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  //---------------------WIFI开关--
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_WIFI;
  mii.typedata = (DWORD)wifi[0];
  mii.hsubmenu = createpmenufileWIFIONOFF();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)wifi[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  //---------------------背光亮度--
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_BACKLIGHT;
  mii.typedata = (DWORD)backlight[0];
  mii.hsubmenu = createpmenufileBACKLIGHT();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)backlight[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);
  //---------usb mode--------------
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDC_SETMODE;
  mii.typedata = (DWORD)usbmode[0];
  mii.hsubmenu = createpmenufileUSB();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)usbmode[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  //------------Format-------------------

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_FORMAT;
  mii.typedata = (DWORD)format[0];
  mii.hsubmenu = 0;

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)format[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);
  //--------------set date -------------
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_SETDATE;
  mii.typedata = (DWORD)setdate[0];
  mii.hsubmenu = 0;

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)setdate[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  //-----碰撞检测-----
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_COLLISION;
  mii.typedata = (DWORD)collision[0];
  mii.hsubmenu = createpmenufileCOLLISION();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)collision[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  //-----------------------
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_LEAVEREC;
  mii.typedata = (DWORD)leavecarrec[0];
  mii.hsubmenu = createpmenufileLEAVECARREC();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)leavecarrec[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  //---------recovery--------------
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_RECOVERY;
  mii.typedata = (DWORD)recovery[0];
  mii.hsubmenu = 0;

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)recovery[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  //---------fw ver--------------
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_FWVER;
  mii.typedata = (DWORD)fw_ver[0];
  mii.hsubmenu = 0;

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)fw_ver[i];

  InsertMenuItem(hmnu, cnt++, TRUE, &mii);

  return hmnu;
}
//-------Play模式下删除菜单
static HMENU createpmenufileDELECT() {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD)DELECT[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)DELECT[i];

  hmnu = CreatePopupMenu(&mii);
  hmnuForRe = hmnu;

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_YES_DELETE;
  mii.typedata = (DWORD)Del_Yes[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Del_Yes[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.state = 0;
  mii.id = IDM_NO_DELETE;
  mii.typedata = (DWORD)Del_No[0];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Del_No[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

static HMENU creatmenu(void)  //创建主
{
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;
  //	HDC hdcForFont;
  //	hdcForFont = GetDC(hMainWndForPlay);
  hmnu = CreateMenu();
  hmnuMain = hmnu;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_BITMAP;  //类型
  mii.state = 0;
  mii.id = IDM_ABOUT_MP;  // ID
  mii.typedata = (DWORD)MP[0];
  mii.uncheckedbmp = &bmp_menu_mp[0];
  mii.checkedbmp = &bmp_menu_mp[1];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MP[i];

  mii.hsubmenu = createpmenufileMP();
  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  mii.type = MFT_BITMAP;
  mii.state = 0;
  mii.id = IDM_ABOUT_TIME;
  mii.typedata = (DWORD)Record_Time[0];
  mii.uncheckedbmp = &bmp_menu_time[0];
  mii.checkedbmp = &bmp_menu_time[1];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Record_Time[i];

  mii.hsubmenu = createpmenufileTIME();
  InsertMenuItem(hmnu, 2, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 3, TRUE, &mii);

  mii.type = MFT_BITMAP;
  mii.state = 0;
  mii.id = IDM_ABOUT_CAR;
  mii.typedata = (DWORD)Record_Mode[0];
  mii.uncheckedbmp = &bmp_menu_car[0];
  mii.checkedbmp = &bmp_menu_car[1];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)Record_Mode[i];

  mii.hsubmenu = createpmenufileCAR();
  InsertMenuItem(hmnu, 4, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 5, TRUE, &mii);

  mii.type = MFT_BITMAP;
  mii.state = 0;
  mii.id = IDM_ABOUT_SETTING;
  mii.typedata = (DWORD)setting[0];
  mii.uncheckedbmp = &bmp_menu_setting[0];
  mii.checkedbmp = &bmp_menu_setting[1];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)setting[i];

  mii.hsubmenu = createpmenufileSETTING();
  InsertMenuItem(hmnu, 6, TRUE, &mii);

  // debug
  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 7, TRUE, &mii);

  mii.type = MFT_BITMAP;
  mii.state = 0;
  mii.id = IDM_ABOUT_DEBUG;
  mii.typedata = (DWORD)debug[0];
  mii.uncheckedbmp = &png_menu_debug[0];
  mii.checkedbmp = &png_menu_debug[1];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)debug[i];

  mii.hsubmenu = createpmenufileDEBUG();
  InsertMenuItem(hmnu, 8, TRUE, &mii);
  //
  return hmnu;
}

static HMENU creatmenu_photo(void)  //创建主
{
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;
  printf("creatmenu_photo\n");
  hmnu = CreateMenu();
  hmnuMain = hmnu;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_BITMAP;  //类型
  mii.state = 0;
  mii.id = IDM_ABOUT_MP;  // ID
  mii.typedata = (DWORD)MP[0];
  mii.uncheckedbmp = &bmp_menu_mp[0];
  mii.checkedbmp = &bmp_menu_mp[1];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MP[i];

  mii.hsubmenu = createpmenufileMP_photo();
  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  mii.type = MFT_BITMAP;
  mii.state = 0;
  mii.id = IDM_ABOUT_SETTING;
  mii.typedata = (DWORD)setting[0];
  mii.uncheckedbmp = &bmp_menu_setting[0];
  mii.checkedbmp = &bmp_menu_setting[1];

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)setting[i];

  mii.hsubmenu = createpmenufileSETTING();
  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

static HMENU creatmenucamera(void) {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;
  //	PMENUBAR menuC;
  //	HDC hdcFor;
  hmnu = CreateMenu();
  //	menuC = (PMENUBAR)hmnu;
  //	hdcFor = GetClientDC(menuC->hwnd);
  hmnuMaincamera = hmnu;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_BITMAP;  //类型
  mii.state = 0;
  mii.id = IDM_ABOUT_MP;  // ID
  mii.typedata = (DWORD)MP[0];
  mii.uncheckedbmp = &bmp_menu_mp[0];
  mii.checkedbmp = &bmp_menu_mp[1];
  mii.hsubmenu = createpmenufileMP_camera();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)MP[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);

  mii.type = MFT_SEPARATOR;  //类型
  mii.state = 0;
  mii.id = 0;  // ID
  mii.typedata = 0;
  InsertMenuItem(hmnu, 1, TRUE, &mii);

  mii.type = MFT_BITMAP;
  mii.state = 0;
  mii.id = IDM_ABOUT_SETTING;
  mii.typedata = (DWORD)setting[0];
  mii.uncheckedbmp = &bmp_menu_setting[0];
  mii.checkedbmp = &bmp_menu_setting[1];
  mii.hsubmenu = createpmenufileSETTING();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)setting[i];

  InsertMenuItem(hmnu, 2, TRUE, &mii);

  return hmnu;
}

static HMENU creatmenuPlay(void) {
  int i;
  HMENU hmnu;
  MENUITEMINFO mii;

  hmnu = CreateMenu();
  hmnuMainplay = hmnu;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_BITMAP;  //类型
  mii.state = 0;
  mii.id = IDM_DELETE;  // ID
  mii.typedata = (DWORD)DELECT[0];
  mii.uncheckedbmp = &bmp_menu_mp[0];
  mii.checkedbmp = &bmp_menu_mp[1];
  mii.hsubmenu = createpmenufileDELECT();

  for (i = 0; i < LANGUAGE_NUM; i++)
    mii.str[i] = (char*)DELECT[i];

  InsertMenuItem(hmnu, 0, TRUE, &mii);
  return hmnu;
}

static int compare_time(HLVITEM nItem1, HLVITEM nItem2, PLVSORTDATA sortData) {
  DWORD data1, data2;
  struct stat stat1, stat2;

  data1 = SendMessage(explorer_wnd, LVM_GETITEMADDDATA, 0, nItem1);
  data2 = SendMessage(explorer_wnd, LVM_GETITEMADDDATA, 0, nItem2);

  stat((char*)data1, &stat1);
  stat((char*)data2, &stat2);

  return (stat1.st_mtime - stat2.st_mtime);
}

static int compare_size(HLVITEM nItem1, HLVITEM nItem2, PLVSORTDATA sortData) {
  DWORD data1, data2;
  struct stat stat1, stat2;
  int size1, size2;

  data1 = SendMessage(explorer_wnd, LVM_GETITEMADDDATA, 0, nItem1);
  data2 = SendMessage(explorer_wnd, LVM_GETITEMADDDATA, 0, nItem2);

  stat((char*)data1, &stat1);
  stat((char*)data2, &stat2);

  if (S_ISREG(stat1.st_mode))
    size1 = stat1.st_size;
  else
    size1 = 0;

  if (S_ISREG(stat2.st_mode))
    size2 = stat2.st_size;
  else
    size2 = 0;

  return (size1 - size2);
}

static HMENU create_rightbutton_menu(void) {
  int i;
  HMENU hMenu;
  MENUITEMINFO mii;
  char* msg[] = {open_, copy, delete_, rename, properties};

  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.type = MFT_STRING;
  mii.id = 0;
  mii.typedata = (DWORD) "File";

  hMenu = CreatePopupMenu(&mii);

  for (i = 0; i < 5; i++) {
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = IDM_FILE + i;
    mii.state = 0;
    mii.typedata = (DWORD)msg[i];
    InsertMenuItem(hMenu, i, TRUE, &mii);
  }

  return hMenu;
  // return StripPopupHead(hMenu);
}

HWND getHwnd(void) {
  return explorer_wnd;
}
void lv_notify_process(HWND hwnd, int id, int code, DWORD addData) {
  if (code == LVN_KEYDOWN) {
    PLVNM_KEYDOWN down;
    int key;

    down = (PLVNM_KEYDOWN)addData;
    key = LOWORD(down->wParam);

    if (key == SCANCODE_REMOVE) {
      HLVITEM hlvi;
      hlvi = SendMessage(hwnd, LVM_GETSELECTEDITEM, 0, 0);
      if (hlvi) {
        if (MessageBox(hMainWnd, are_you_really_want_to_delete_this_file,
                       warning, MB_YESNO | MB_DEFBUTTON2) == IDYES) {
          // not really delete yet.
          SendMessage(hwnd, LVM_DELITEM, 0, hlvi);
        }
      }
    }
    if (key == SCANCODE_ENTER) {
    }
  }
  if (code == LVN_ITEMRUP) {
    PLVNM_ITEMRUP up;
    int x, y;

    up = (PLVNM_ITEMRUP)addData;
    x = LOSWORD(up->lParam);
    y = HISWORD(up->lParam);

    ClientToScreen(explorer_wnd, &x, &y);

    TrackPopupMenu(GetPopupSubMenu(explorer_menu),
                   TPM_LEFTALIGN | TPM_LEFTBUTTON, x, y, hMainWnd);
  }
  if (code == LVN_ITEMDBCLK) {
    HLVITEM hlvi = SendMessage(hwnd, LVM_GETSELECTEDITEM, 0, 0);
    if (hlvi > 0) {
      if (MessageBox(hMainWnd, Are_you_really_want_to_open_this_file, Question,
                     MB_YESNO) == IDYES) {
        MessageBox(hMainWnd, "Me too.", "Sorry", MB_OK);
      }
    }
  }
}

static void startrec0(HWND hWnd) {
  if (video_rec_state)
    return;

  InvalidateRect (hWnd, &msg_rcWatermarkTime, FALSE);
  InvalidateRect (hWnd, &msg_rcWatermarkImg, FALSE);

  // gc_sd_space();
  // test code start
  audio_play0("/usr/local/share/sounds/VideoRecord.wav");
  // test code end
  if (video_record_startrec() == 0) {
    video_rec_state = 1;
    FlagForUI_ca.video_rec_state_ui = video_rec_state;
    sec = 0;
  }
}

void startrec(HWND hWnd) {
  if (!set_motion_detection_trigger_value(true))
    return;
  startrec0(hWnd);
}

static void stoprec0(HWND hWnd) {
  if (!video_rec_state)
    return;
  video_rec_state = 0;
  FlagForUI_ca.video_rec_state_ui = video_rec_state;
  InvalidateRect(hWnd, &msg_rcTime, TRUE);
  InvalidateRect(hWnd, &msg_rcRecimg, TRUE);

  InvalidateRect (hWnd, &msg_rcWatermarkTime, TRUE);
  InvalidateRect (hWnd, &msg_rcWatermarkImg, TRUE);

  video_record_stoprec();
}

void stoprec(HWND hWnd) {
  if (!set_motion_detection_trigger_value(false))
    return;
  stoprec0(hWnd);
}

static void initrec(HWND hWnd) {
  struct ui_frame front = {parameter_get_video_frontcamera_width(),
                           parameter_get_video_frontcamera_height(),
                           parameter_get_video_frontcamera_fps()};

  struct ui_frame back = {parameter_get_video_backcamera_width(),
                          parameter_get_video_backcamera_height(),
                          parameter_get_video_backcamera_fps()};

  char photo_size = parameter_get_vcamresolution_photo();
  if (photo_size < 0 || photo_size >= 3)
    photo_size = 0;
  struct ui_frame photo[3] = {UI_FRAME_720P, UI_FRAME_1080P, UI_FRAME_1080P};

  memset(&adas_out, 0, sizeof(dsp_ldw_out));

  if (SetMode != MODE_PHOTO) {
    video_record_set_record_mode(true);
    video_record_init(&front, &back);
  } else {
    video_record_set_record_mode(false);
    video_record_init(&photo[photo_size], &photo[photo_size]);
  }

  user_entry();  // test code
  if (parameter_get_video_de() == 1)
    SendMessage(hWnd, MSG_COMMAND, IDM_detectON, 1);

  InvalidateRect(hWnd, NULL, FALSE);
}

static void deinitrec(HWND hWnd) {

  /*
   * FIXME: I do not think it is a good idea that
   * manage wifi related business in UI thread,
   * too trivial.
   */
  video_record_stop_ts_transfer(1);

  stop_motion_detection();
  user_exit();  // test code
  if (video_rec_state)
    stoprec(hWnd);
}

void startplayvideo(HWND hWnd, char* filename) {
  printf("%s filename = %s\n", __func__, filename);
  changemode(hWnd, MODE_PLAY);
  sprintf(testpath, "%s", filename);
  videoplay_init(filename);
}

void exitplayvideo(HWND hWnd) {
  videoplay_deinit();
  changemode(hWnd, MODE_PREVIEW);
  video_play_state = 0;
  if (sdcard == 0) {
    exitvideopreview();
    /*
    if (explorer_path == 0)
            explorer_path = calloc(1, 256);
    sprintf(explorer_path, "%s", SDCARD_PATH);
    explorer_update(explorer_wnd);
    */
  }
  InvalidateRect(hWnd, NULL, FALSE);
}

int explorer_getfiletype(char* filename) {
  int type = FILE_TYPE_UNKNOW;
  char* ext_name;
  if (!filename || filename[0] == '.') {
    return type;
  }
  ext_name = strrchr(filename, '.');
  if (ext_name) {
    ext_name++;

    if (strcmp(ext_name, "mp4") == 0) {
      type = FILE_TYPE_VIDEO;
    } else if (strcmp(ext_name, "jpg") == 0) {
      type = FILE_TYPE_PIC;
    }
  }

  return type;
}

int explorer_enter(HWND hWnd) {
  HLVITEM hItemSelected;
  HLVITEM hItem;
  LVITEM lvItem;

  hItemSelected = SendMessage(explorer_wnd, LVM_GETSELECTEDITEM, 0, 0);
  SendMessage(explorer_wnd, LVM_GETITEM, hItemSelected, (LPARAM)&lvItem);
  if (lvItem.itemData) {
    if (lvItem.dwFlags) {
      if (strcmp((char*)lvItem.itemData, ".") == 0) {
        sprintf(explorer_path, "%s", SDCARD_PATH);
      } else if (strcmp((char*)lvItem.itemData, "..") == 0) {
        char* seek = strrchr(explorer_path, '/');
        if (strcmp(explorer_path, SDCARD_PATH) > 0) {
          explorer_path[seek - explorer_path] = 0;
        }
      } else {
		strcat(explorer_path, "/");
		strcat(explorer_path, (const char *)lvItem.itemData);
      }
      explorer_update(explorer_wnd);
    } else {
      int filetype = explorer_getfiletype((char*)lvItem.itemData);
      if (filetype == FILE_TYPE_VIDEO || filetype == FILE_TYPE_PIC) {
        // char file[128];
        sprintf(testpath, "%s/%s", explorer_path, lvItem.itemData);
        test_replay = 1;
        startplayvideo(hWnd, testpath);
      }
    }
  }
}

int explorer_back(HWND hWnd) {
  char* seek = strrchr(explorer_path, '/');

  if (strcmp(explorer_path, SDCARD_PATH) > 0) {
    explorer_path[seek - explorer_path] = 0;
  }

  explorer_update(explorer_wnd);
}

int explorer_filedelete(HWND hWnd) {
  HLVITEM hItemSelected;
  HLVITEM hItem;
  LVITEM lvItem;
  int mesg = 0;
  hItemSelected = SendMessage(explorer_wnd, LVM_GETSELECTEDITEM, 0, 0);
  SendMessage(explorer_wnd, LVM_GETITEM, hItemSelected, (LPARAM)&lvItem);

  if (lvItem.dwFlags == 0) {
    if (lvItem.itemData) {
      if (parameter_get_video_lan() == 1)
        mesg = MessageBox(explorer_wnd, "删除此视频?", "提示", MB_YESNO | MB_DEFBUTTON2);
      else if (parameter_get_video_lan() == 0)
        mesg =
            MessageBox(explorer_wnd, "Delete this video ?", "Prompt", MB_YESNO | MB_DEFBUTTON2);
      if (mesg == IDYES) {
        // char file[256];
        // sprintf(file, "%s/%s", explorer_path, lvItem.itemData);
        // printf("file = %s\n", file);
        // fs_manage_remove(explorer_path, (char *)lvItem.itemData);
        SendMessage(explorer_wnd, LVM_DELITEM, 0, (LPARAM)hItemSelected);
        // remove(file);
      }
    }
  }
}

void* explorer_update_thread(void* arg) {
  LVSUBITEM subdata;
  LVITEM item;
  HWND hWnd = (HWND)arg;

  struct file_list* list;
  while (explorer_update_run) {
    if (explorer_update_reupdate == 0) {
      usleep(10000);
    } else {
      int i = 0;
      explorer_update_reupdate = 0;
      SendMessage(hWnd, LVM_DELALLITEM, 0, 0);
      // printf("%s 1, explorer_path = %s\n", __func__, explorer_path);
      list = fs_manage_getfilelist(explorer_path);
      if (list) {
        struct file_node* node_tmp;

        node_tmp = list->folder_tail;
        while (node_tmp && explorer_update_run && !explorer_update_reupdate) {
          // printf("%s i =%d\n", __func__, i);
          item.itemData = (DWORD)strdup(node_tmp->name);
          item.nItem = i;
          item.nItemHeight = 28;
          item.dwFlags = LVIF_FOLD;
          SendMessage(hWnd, LVM_ADDITEM, 0, (LPARAM)&item);

          subdata.nItem = i;

          subdata.subItem = 0;
          subdata.flags = 0;
          subdata.pszText = node_tmp->name;
          subdata.flags = LVFLAG_BITMAP;

          if ((strcmp(node_tmp->name, ".") == 0) ||
              (strcmp(node_tmp->name, "..") == 0)) {
            subdata.image = (DWORD)&back_bmap;
          } else {
            subdata.image = (DWORD)&folder_bmap;
          }

          subdata.nTextColor = PIXEL_blue;
          SendMessage(hWnd, LVM_FILLSUBITEM, 0, (LPARAM)&subdata);

          i++;
          node_tmp = node_tmp->pre;
          if ((i % 500) == 0)
            usleep(10000);
        }

        node_tmp = list->file_tail;
        while (node_tmp && explorer_update_run && !explorer_update_reupdate) {
          int filetype;
          // printf("%s i =%d\n", __func__, i);
          item.itemData = (DWORD)strdup(node_tmp->name);
          item.nItem = i;
          item.nItemHeight = 28;
          item.dwFlags = 0;
          SendMessage(hWnd, LVM_ADDITEM, 0, (LPARAM)&item);

          subdata.nItem = i;

          subdata.subItem = 0;
          subdata.flags = 0;
          subdata.pszText = node_tmp->name;
          subdata.flags = LVFLAG_BITMAP;
          filetype = explorer_getfiletype(node_tmp->name);

          subdata.nTextColor = PIXEL_black;
          subdata.image = (DWORD)&filetype_bmap[filetype];

          SendMessage(hWnd, LVM_FILLSUBITEM, 0, (LPARAM)&subdata);
          i++;
          node_tmp = node_tmp->pre;
          if ((i % 500) == 0)
            usleep(10000);
        }
        fs_manage_free_filelist(&list);
        printf("add file finish\n");
      }
    }
  }
  printf("%s out\n", __func__);
  pthread_exit(NULL);
}

int explorer_update(HWND hWnd) {
  printf("%s 1\n", __func__);
  explorer_update_reupdate = 1;
  printf("%s 2\n", __func__);

  return 0;
}

void create_explorer(HWND hWnd) {
  LVCOLUMN s1;
  int width;

  explorer_menu = create_rightbutton_menu();

  width = g_rcScr.right - g_rcScr.left;

  explorer_wnd = CreateWindowEx(
      CTRL_LISTVIEW, "List View",
      WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_BORDER, WS_EX_NONE,
      IDC_LISTVIEW, g_rcScr.left, g_rcScr.top + TOPBK_IMG_H, width,
      g_rcScr.bottom - TOPBK_IMG_H, hWnd, 0);

  SetNotificationCallback(explorer_wnd, lv_notify_process);

  s1.nCols = 0;
  s1.pszHeadText = File_name;
  s1.width = width - 10;
  s1.pfnCompare = NULL;
  s1.colFlags = 0;
  SendMessage(explorer_wnd, LVM_ADDCOLUMN, 0, (LPARAM)&s1);

  if (explorer_path == 0)
    explorer_path = calloc(1, 256);
  sprintf(explorer_path, "%s", SDCARD_PATH);
  explorer_update_reupdate = 0;
  explorer_update_run = 1;
  if (pthread_create(&explorer_update_tid, NULL, explorer_update_thread,
                     (void*)explorer_wnd)) {
    printf("%s pthread_create err\n", __func__);
  }
  explorer_update(explorer_wnd);

  SetFocusChild(explorer_wnd);
}

void destroy_explorer(void) {
  explorer_update_run = 0;
  if (explorer_update_tid)
    pthread_join(explorer_update_tid, NULL);
  explorer_update_tid = 0;
  DestroyMenu(explorer_menu);
  DestroyWindow(explorer_wnd);
  if (explorer_path) {
    free(explorer_path);
    explorer_path = 0;
  }
  explorer_menu = 0;
  explorer_wnd = 0;
}

static char* mk_time(char* buff) {
  time_t t;

  struct tm* tm;
  time(&t);
  tm = localtime(&t);
  sprintf(buff, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

  return buff;
}

void popmenu(HWND hWnd) {
  SendAsyncMessage(hWnd, MSG_MENUBARPAINT, 0, 0);
  TrackMenuBar(hWnd, 0);
}

void videopreview_getfilename(char* filename, struct file_node* filenode) {
  if (filenode == NULL)
    return;

  if (filenode->filetype == VIDEOFILE_TYPE)
    sprintf(filename, "%s/%s\0", VIDEOFILE_PATH, filenode->name);
  else if (filenode->filetype == LOCKFILE_TYPE)
    sprintf(filename, "%s/%s\0", VIDEOFILE_PATH, filenode->name);

  return;
}

void videopreview_show_decode(char* previewname,
                              char* videoname,
                              char* filename) {
  videoplay = 0;
  if (previewname == NULL)
    return;

  preview_isvideo = 0;
  sprintf(previewname, "%s(%d/%d)", videoname, currentfilenum, totalfilenum);
  printf("%s file = %s, previewname = %s\n", __func__, filename, previewname);
  if (strstr(previewname, ".mp4")) {
    printf("videoplay_decode_one_frame(%s)\n", filename);
    preview_isvideo = 1;
    videoplay = videoplay_decode_one_frame(filename);
    printf("videoplay= %d\n", videoplay);
    if (videoplay != 0) {
      printf("videoplay==-1\n");
      if (parameter_get_video_lan() == 1)
        MessageBox(hwndforpre, "视频错误!!!", "警告!!!", MB_OK);
      else if (parameter_get_video_lan() == 0)
        MessageBox(hwndforpre, "video error !!!", "Warning!!!", MB_OK);
    }
  } else if (strstr(previewname, ".jpg")) {
    printf("videoplay_decode_jpeg(%s)\n", filename);
    videoplay_decode_jpeg(filename);
  }

  return;
}

void entervideopreview(void) {
  int i;
  char filename[FILENAME_LEN];

  totalfilenum = 0;
  currentfilenum = 0;
  preview_isvideo = 0;
  printf("%s\n", __func__);
  if (sdcard == 0) {
    sprintf(previewname, "%s", "no sd card");
    return;
  }
  char cmd[] = "sync\0";
  runapp(cmd);  // sync when showing
  filelist = fs_manage_getmediafilelist();
  if (filelist == NULL) {
    sprintf(previewname, "(%d/%d)", currentfilenum, totalfilenum);
    return;
  }
  currentfile = filelist->file_tail;

  if (currentfile == NULL) {
    sprintf(previewname, "(%d/%d)", currentfilenum, totalfilenum);
    return;
  }

  while (currentfile) {
    currentfile = currentfile->pre;
    totalfilenum++;
  }
  if (totalfilenum)
    currentfilenum = 1;
  currentfile = filelist->file_tail;

  videopreview_getfilename(filename, currentfile);
  videopreview_show_decode(previewname, currentfile->name, filename);

  return;
}

void videopreview_refresh(HWND hWnd) {
  char filename[FILENAME_LEN];

  printf("%s\n", __func__);
  if (currentfile == NULL)
    return;

  videopreview_getfilename(filename, currentfile);
  videopreview_show_decode(previewname, currentfile->name, filename);
  InvalidateRect(hWnd, NULL, FALSE);

  return;
}

void videopreview_next(HWND hWnd) {
  char filename[FILENAME_LEN];

  printf("%s\n", __func__);
  if (currentfile == NULL)
    return;

  if (currentfile->pre == NULL) {
    currentfile = filelist->file_tail;
    if (currentfile == NULL)
      return;
    currentfilenum = 1;
  } else {
    currentfile = currentfile->pre;
    currentfilenum++;
  }
  videopreview_getfilename(filename, currentfile);
  videopreview_show_decode(previewname, currentfile->name, filename);
  InvalidateRect(hWnd, NULL, FALSE);

  return;
}

void videopreview_pre(HWND hWnd) {
  char filename[FILENAME_LEN];
  printf("%s\n", __func__);
  if (currentfile == NULL)
    return;

  if (currentfile->next == NULL) {
    currentfile = filelist->file_head;
    if (currentfile == NULL)
      return;
    currentfilenum = totalfilenum;
  } else {
    currentfile = currentfile->next;
    currentfilenum--;
  }
  if (currentfilenum < 0)
    currentfilenum = 0;

  videopreview_getfilename(filename, currentfile);
  videopreview_show_decode(previewname, currentfile->name, filename);
  InvalidateRect(hWnd, NULL, FALSE);

  return;
}

void videopreview_play(HWND hWnd) {
  char filename[FILENAME_LEN];
  printf("%s\n", __func__);
  if (currentfile == NULL)
    return;

  videopreview_getfilename(filename, currentfile);

  if (strstr(currentfile->name, ".mp4")) {
    printf("videoplay_decode_one_frame(%s)\n", filename);
    startplayvideo(hWnd, filename);
  } else {
    if (parameter_get_debug_video() == 1)
      video_time = VIDEO_TIME - 1;
    if (parameter_get_debug_modechange() == 1) {
      // modechange_pre_flage==1;
      modechange_time = MODECHANGE_TIME - 1;
    }
  }
}

int videopreview_delete(HWND hWnd) {
  char filename[FILENAME_LEN];
  struct file_node* current_filenode;
  int mesg = 0;
  if (currentfile == NULL)
    return;
  if (parameter_get_video_lan() == 1)
    mesg = MessageBox(hWnd, "删除此视频?", "提示", MB_YESNO | MB_DEFBUTTON2);
  else if (parameter_get_video_lan() == 0)
    mesg = MessageBox(hWnd, "Delete this video ?", "Prompt", MB_YESNO | MB_DEFBUTTON2);
  if (mesg == IDYES) {
    // char file[256];
    // sprintf(file, "%s/%s", explorer_path, lvItem.itemData);
    // printf("file = %s\n", file);
    current_filenode = currentfile;
    if (currentfile->pre == NULL) {
      currentfile = currentfile->next;
      currentfilenum--;
    } else {
      currentfile = currentfile->pre;
    }

    totalfilenum--;

    videopreview_getfilename(filename, current_filenode);
    fs_manage_remove(filename);

    if (currentfile == NULL) {
      sprintf(previewname, "(%d/%d)", currentfilenum, totalfilenum);
      printf("%spreviewname = %s\n", __func__, previewname);
      videoplay_set_fb_black();
      goto out;
    }
    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);
  out:
    InvalidateRect(hWnd, NULL, FALSE);

    return;
  }
}

void exitvideopreview(void) {
  printf("%s\n", __func__);
  fs_manage_free_mediafilelist();
  filelist = 0;
  currentfile = 0;

  totalfilenum = 0;
  currentfilenum = 0;
  preview_isvideo = 0;
  if (sdcard == 0) {
    sprintf(previewname, "%s", "no sd card");
  }
  videoplay_set_fb_black();
}

void changemode(HWND hWnd, int mode) {
  RECT msg_rcTopBk = {TOPBK_IMG_X, TOPBK_IMG_Y, TOPBK_IMG_X + g_rcScr.right,
                      TOPBK_IMG_Y + TOPBK_IMG_H};
  switch (SetMode) {
    case MODE_PHOTO:
    case MODE_RECORDING:
      if (mode == MODE_EXPLORER) {
        deinitrec(hWnd);
        video_record_deinit();
        SetMode = mode;
        DestroyMainWindowMenu(hWnd);
        create_explorer(hWnd);
        InvalidateRect(hWnd, &msg_rcTopBk, FALSE);
      } else if (mode == MODE_USBDIALOG) {
        deinitrec(hWnd);
        video_record_deinit();
        SetMode = mode;
        DestroyMainWindowMenu(hWnd);
      } else if (mode == MODE_PREVIEW) {
        deinitrec(hWnd);
        video_record_deinit();
        SetMode = mode;
        DestroyMainWindowMenu(hWnd);
        entervideopreview();
        InvalidateRect(hWnd, NULL, FALSE);
      } else if (mode == MODE_RECORDING || mode == MODE_PHOTO) {
        pthread_mutex_lock(&set_mutex);
        deinitrec(hWnd);
        video_record_deinit();
        SetMode = mode;
        initrec(hWnd);
        DestroyMainWindowMenu(hWnd);
        CreateMainWindowMenu(hWnd, creatmenu_photo());
        pthread_mutex_unlock(&set_mutex);
      }
      break;
    case MODE_EXPLORER:
      if (mode == MODE_RECORDING || mode == MODE_PHOTO) {
        pthread_mutex_lock(&set_mutex);
        SetMode = mode;
        destroy_explorer();
        CreateMainWindowMenu(hWnd, creatmenu());
        initrec(hWnd);

        InvalidateRect(hWnd, &msg_rcTopBk, FALSE);
        pthread_mutex_unlock(&set_mutex);
      } else if (mode == MODE_PLAY) {
        SetMode = mode;
        ShowWindow(explorer_wnd, SW_HIDE);
        InvalidateRect(hWnd, NULL, FALSE);
      } else if (mode == MODE_USBDIALOG) {
        SetMode = mode;
        destroy_explorer();
      }
      break;
    case MODE_PLAY:
      if (mode == MODE_EXPLORER) {
        ShowWindow(explorer_wnd, SW_SHOW);
        SetFocusChild(explorer_wnd);
        SetMode = mode;
      } else if (mode == MODE_USBDIALOG) {
        test_replay = 0;
        videoplay_exit();
        SetMode = mode;
        destroy_explorer();
      } else if (mode == MODE_PREVIEW) {
        SetMode = mode;
        InvalidateRect(hWnd, NULL, FALSE);
      }
      break;
    case MODE_USBDIALOG:
      if (mode == MODE_RECORDING || mode == MODE_PHOTO) {
        pthread_mutex_lock(&set_mutex);
        SetMode = mode;
        CreateMainWindowMenu(hWnd, creatmenu());
        initrec(hWnd);

        InvalidateRect(hWnd, &msg_rcTopBk, FALSE);
        pthread_mutex_unlock(&set_mutex);
      } else if (mode == MODE_USBCONNECTION) {
        SetMode = mode;
        InvalidateRect(hWnd, NULL, TRUE);
      }
      break;
    case MODE_USBCONNECTION:
      if (mode == MODE_RECORDING || mode == MODE_PHOTO) {
        pthread_mutex_lock(&set_mutex);
        SetMode = mode;
        CreateMainWindowMenu(hWnd, creatmenu());
        initrec(hWnd);
        pthread_mutex_unlock(&set_mutex);
      }
      break;
    case MODE_PREVIEW:
      if (mode == MODE_EXPLORER) {
        exitvideopreview();
        SetMode = mode;
        create_explorer(hWnd);
        InvalidateRect(hWnd, &msg_rcTopBk, FALSE);
      } else if (mode == MODE_PLAY) {
        SetMode = mode;
        InvalidateRect(hWnd, NULL, FALSE);
      } else if (mode == MODE_RECORDING || mode == MODE_PHOTO) {
        pthread_mutex_lock(&set_mutex);
        exitvideopreview();
        SetMode = mode;
        CreateMainWindowMenu(hWnd, creatmenu());
        initrec(hWnd);
        pthread_mutex_unlock(&set_mutex);
      }
      break;
  }
  FlagForUI_ca.setmode_ui = SetMode;
}
void loadingWaitBmp(HWND hWnd) {
  ANIMATION* anim = CreateAnimationFromGIF89aFile(
      HDC_SCREEN, "/usr/local/share/minigui/res/images/waitfor2.gif");
  if (anim == NULL) {
    printf("anim=NULL\n");
    return;
  }
  key_lock = 1;
  SetWindowAdditionalData(hWnd, (DWORD)anim);
  CreateWindow(CTRL_ANIMATION, "", WS_VISIBLE | ANS_AUTOLOOP, 190,
               (WIN_W - 98) / 2, (WIN_H - 98) / 2, 98, 98, hWnd, (DWORD)anim);
  SendMessage(GetDlgItem(hWnd, 190), ANM_STARTPLAY, 0, 0);

  return;
}

void unloadingWaitBmp(HWND hWnd)
{
  DWORD win_additionanl_data = GetWindowAdditionalData(hWnd);
  if (win_additionanl_data){
    DestroyAnimation((ANIMATION*)win_additionanl_data, TRUE);
    DestroyAllControls(hWnd);
  }
  key_lock = 0;
}

static void proc_MSG_USBCHAGE(HWND hWnd,
                              int message,
                              WPARAM wParam,
                              LPARAM lParam) {
#if USB_MODE
  CTRLDATA UsbCtrl[] = {
      {CTRL_BUTTON, WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | WS_TABSTOP,
       90, 80, 100, 40, IDUSB, " 磁  盘 ", 0},
      {CTRL_BUTTON, WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 90, 130, 100, 40,
       USBBAT, " 充  电 ", 0}};
  CTRLDATA UsbCtrl_en[] = {
      {CTRL_BUTTON, WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | WS_TABSTOP,
       90, 80, 100, 40, IDUSB, " Disk ", 0},
      {CTRL_BUTTON, WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 90, 130, 100, 40,
       USBBAT, " Charging ", 0}};
  DLGTEMPLATE UsbDlg = {WS_VISIBLE, WS_EX_NONE, 0, 0, 300,  300,
                        SetDate,    0,          0, 2, NULL, 0};
#endif
  int usb_sd_chage = (int)lParam;

  printf("MSG_USBCHAGE : usb_sd_chage = ( %d )\n", usb_sd_chage);

  if (usb_sd_chage == 1) {
#if USB_MODE
    changemode(hWnd, MODE_USBDIALOG);
    UsbDlg.w = g_rcScr.right - g_rcScr.left;
    UsbDlg.h = g_rcScr.bottom - g_rcScr.top;
    if (parameter_get_video_lan() == 1) {
      UsbCtrl[0].x = (UsbDlg.w - UsbCtrl[0].w) >> 1;
      UsbCtrl[0].y = (UsbDlg.h - 50) >> 1;

      UsbCtrl[1].x = (UsbDlg.w - UsbCtrl[1].w) >> 1;
      UsbCtrl[1].y = (UsbDlg.h + 50) >> 1;

      UsbDlg.controls = UsbCtrl;
    } else if (parameter_get_video_lan() == 0) {
      UsbCtrl_en[0].x = (UsbDlg.w - UsbCtrl_en[0].w) >> 1;
      UsbCtrl_en[0].y = (UsbDlg.h - 50) >> 1;

      UsbCtrl_en[1].x = (UsbDlg.w - UsbCtrl_en[1].w) >> 1;
      UsbCtrl_en[1].y = (UsbDlg.h + 50) >> 1;

      UsbDlg.controls = UsbCtrl_en;
    }

    DialogBoxIndirectParam(&UsbDlg, HWND_DESKTOP, UsbSDProc, 0L);
#else
printf("usb_sd_chage == 1:USB_MODE==0\n");
if (sdcard == 1) {
			cvr_usb_sd_ctl(USB_CTRL, 1);
		} else if (sdcard == 0) {
			//	EndDialog(hDlg, wParam);
			if (parameter_get_video_lan() == 1)
				MessageBox(hWnd, "SD卡不存在!!!", "警告!!!", MB_OK);
			else if (parameter_get_video_lan() == 0)
				MessageBox(hWnd, "no SD card insertted!!!", "Warning!!!", MB_OK);
		}
#endif
  } else if (usb_sd_chage == 0) {
#if USB_MODE
    if (hWndUSB) {
      cvr_usb_sd_ctl(USB_CTRL, 0);
      EndDialog(hWndUSB, wParam);
      InvalidateRect(hWnd, &USB_rc, TRUE);
      hWndUSB = 0;
      changemode(hWnd, MODE_RECORDING);
    }
#else
cvr_usb_sd_ctl(USB_CTRL, 0);
printf("usb_sd_chage == 0:USB_MODE==0\n");
#endif 
  }
}

static proc_record_SCANCODE_CURSORBLOCKRIGHT(HWND hWnd) {
#ifdef USE_CIF_CAMERA
  short inputid = parameter_get_cif_inputid();
  inputid = (inputid == 0) ? 1 : 0;
  parameter_save_cif_inputid(inputid);
  ui_deinit_init_camera(hWnd, -1, -1);
#endif

  video_record_inc_nv12_raw_cnt();
}

static void ui_takephoto(HWND hWnd) {
  if (sdcard == 1 && !takephoto) {
    takephoto = true;
    audio_play(capture_sound);
    loadingWaitBmp(hWnd);
    video_record_takephoto();
  }
}

static void proc_MSG_SDMOUNTFAIL(HWND hWnd) {
  int mesg = 0;

  printf("MSG_SDMOUNTFAIL\n");
  if (parameter_get_video_lan() == 1)
    mesg =
        MessageBox(hWnd, "加载sd卡失败，\n是否格式化sd卡?", "警告!!!", MB_YESNO | MB_DEFBUTTON2);
  else if (parameter_get_video_lan() == 0)
    mesg = MessageBox(hWnd, "sd card loading failed, \nformat the sd card ?",
                      "Warning!!!", MB_YESNO);
  if (mesg == IDYES) {
    loadingWaitBmp(hWnd);
    fs_manage_format_sdcard(sdcardformat_back, (void*)hWnd, 1);
  }
}

static void proc_MSG_SDNOTFIT(HWND hWnd) {
  int mesg = 0;

  printf("MSG_SDNOTFIT\n");
  if (parameter_get_video_lan() == 1)
    mesg =
        MessageBox(hWnd, "sd卡参数有误，\n是否格式化sd卡?", "警告!!!", MB_YESNO | MB_DEFBUTTON2);
  else if (parameter_get_video_lan() == 0)
    mesg = MessageBox(hWnd, "sd card data error,\nformat the sd card ?",
                      "Warning!!!", MB_YESNO);
  if (mesg == IDYES) {
    loadingWaitBmp(hWnd);
    fs_manage_format_sdcard(sdcardformat_notifyback, (void*)hWnd, 0);
  } else {
    PostMessage(hWnd, MSG_SDCHANGE, 0, 1);  
  }
}

static void proc_MSG_FS_INITFAIL(HWND hWnd, int param)
{
  int mesg = 0;
  printf("FS_INITFAIL\n");
  // no space
  if (param == -1) {
    if (parameter_get_video_lan() == 1)
      mesg = MessageBox(hWnd, "sd卡剩余空间不足\n是否格式化sd卡?", "警告!!!", MB_YESNO | MB_DEFBUTTON2);
    else if (parameter_get_video_lan() == 0)
      mesg = MessageBox(hWnd, "sd card no enough free space ,\nformat the sd card ?",
                      "Warning!!!", MB_YESNO);
  // init fail
  } else if (param == -2) {
    if (parameter_get_video_lan() == 1)
      mesg = MessageBox(hWnd, "sd卡初始化异常\n是否格式化sd卡?", "警告!!!", MB_YESNO | MB_DEFBUTTON2);
    else if (parameter_get_video_lan() == 0)
      mesg = MessageBox(hWnd, "sd card init fail ,\nformat the sd card ?",
                      "Warning!!!", MB_YESNO);
  // else error
  } else {
    if (parameter_get_video_lan() == 1)
      MessageBox(hWnd, "sd卡初始化异常", "警告!!!", MB_OK);
    else if (parameter_get_video_lan() == 0)
      MessageBox(hWnd, "sd card init fail", "Warning!!!", MB_OK);
    return;
  }

  if (mesg == IDYES) {
    // if  record now, need  stop the record
    stoprec(hWnd); 
    loadingWaitBmp(hWnd);
    fs_manage_format_sdcard(sdcardformat_notifyback, (void*)hWnd, 0);
  }

  return;
}

static void get_log_filename(char filename[], size_t size) {
  time_t now;
  struct tm* timenow;
  unsigned int year, mon, day, hour, min, sec;

  time(&now);
  timenow = localtime(&now);
  year = timenow->tm_year + 1900;
  mon = timenow->tm_mon + 1;
  day = timenow->tm_mday;
  hour = timenow->tm_hour;
  min = timenow->tm_min;
  sec = timenow->tm_sec;
  snprintf(filename, size, "%s/%04d%02d%02d_%02d%02d%02d.%s", "/mnt/sdcard",
           year, mon, day, hour, min, sec, "log");
}

static void ui_timer_debug_process(HWND hWnd) {
  if (parameter_get_debug_reboot() == 1) {
    reboot_time++;
    printf("--------REBOOT_TIME1-------\n");
    if (reboot_time == REBOOT_TIME) {
      reboot_time = 0;
      printf("--------REBOOT_TIME-------\n");
      runapp("busybox reboot");
    }
  }
  if (parameter_get_debug_recovery() == 1) {
    recovery_time++;
    if (recovery_time == RECOVERY_TIME) {
      recovery_time = 0;
      parameter_recover();
      parameter_save_debug_recovery(1);
    }
  }
  if (parameter_get_debug_modechange() == 1) {
    modechange_time++;
    if (SetMode == MODE_RECORDING) {
      if (video_rec_state == 0) {
        if (sdcard == 1) {
          startrec(hWnd);
        }
      } else if (video_rec_state != 0 && modechange_time == MODECHANGE_TIME) {
        stoprec(hWnd);
      }
      if (modechange_time == MODECHANGE_TIME) {
        modechange_time = 0;
        changemode(hWnd, MODE_PREVIEW);
      }
    } else if (SetMode == MODE_PREVIEW) {
      /*
      if(modechange_time ==1) {
        if(videoplay!=0) {
          //  printf("videoplay==-1\n");
          if(parameter_get_video_lan()==1) {
            modechange_time=MODECHANGE_TIME-1;
          } else if(parameter_get_video_lan()==0) {
            modechange_time=MODECHANGE_TIME-1;
          }
        } else {
          videopreview_play(hWnd);
        }
      }*/
      if (modechange_time == MODECHANGE_TIME /*&&modechange_pre_flage==1*/) {
        //modechange_pre_flage =0 ;
        modechange_time = 0;
        changemode(hWnd, MODE_RECORDING);
      }
    }
  }
  if (parameter_get_debug_video() == 1) {
    if (SetMode == MODE_PREVIEW) {
      if (video_time == 0) {
        if (videoplay != 0) {
          //  printf("videoplay==-1\n");
          if (parameter_get_video_lan() == 1) {
            video_time = 0;
            videopreview_next(hWnd);
          } else if (parameter_get_video_lan() == 0) {
            video_time = 0;
            videopreview_next(hWnd);
          }
        } else {
          video_time = 2;
          videopreview_play(hWnd);
        }
      }

      if (video_time == 1) {
        video_time = 0;
        videopreview_next(hWnd);
      }
    }
  }
  if (parameter_get_debug_beg_end_video() == 1) {
    if (SetMode == MODE_RECORDING) {
      beg_end_video_time++;
      if (beg_end_video_time == 1) {
        if (video_rec_state == 0) {
          if (sdcard == 1) {
            startrec(hWnd);
          }
        }
      }
      if (beg_end_video_time == BEG_END_VIDEO_TIME) {
        if (video_rec_state != 0) {
          stoprec(hWnd);
        }
        beg_end_video_time = 0;
      }
    }
  }
  if (parameter_get_debug_photo() == 1) {
    if (SetMode == MODE_RECORDING) {
      photo_time++;
      if (photo_time == PHOTO_TIME) {
        ui_takephoto(hWnd);
        photo_time = 0;
      }
    }
  }
}

static int CameraWinProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam) {
  char buff2[20];
  HDC hdc;
  HDC hdcforbater;
  RECT rc;
  char msg_text[30];
  int mesg = 0;

  // fprintf( stderr, "wParam =%d ,lParam =%d \n",(int)wParam,(int)lParam);
  switch (message) {
    case MSG_CREATE:
      SetTimer(hWnd, _ID_TIMER, 100);
      break;
    case MSG_HDMI: {
      //printf("%s wParam = %s, lParam = %d, hMainWnd = %d, hWnd = %d\n", __func__, wParam, lParam, hMainWnd, hWnd);
      if (lParam == 1) {
        rk_fb_set_out_device(OUT_DEVICE_HDMI);
      } else {
        rk_fb_set_out_device(OUT_DEVICE_LCD);
      }
      usleep(500000);
      if (SetMode == MODE_PREVIEW)
        videopreview_refresh(hWnd);
      else if (SetMode == MODE_USBDIALOG) {
        InvalidateRect(hWndUSB, NULL, TRUE);
        break;
      }
      InvalidateRect(hMainWnd, NULL, TRUE);
      break;
    }

    case MSG_REPAIR:
      if (parameter_get_video_lan() == 1)
        MessageBox(hWnd, "sd卡中有损坏视频，\n现已修复!", "警告!!!", MB_OK);
      else if (parameter_get_video_lan() == 0)
        MessageBox(hWnd, "Sd card is damaged in video, \nwe have repair!",
                   "Warning!!!", MB_OK);
      break;
    case MSG_FSINITFAIL:
      proc_MSG_FS_INITFAIL(hWnd, -1);
      break;
    case MSG_PHOTOEND:
      if (lParam == 1 && takephoto) {
      	unloadingWaitBmp(hWnd);
        takephoto = false;
      }
      break;
    case MSG_ADAS:
      if (SetMode == MODE_RECORDING) {
        memcpy((void *)&adas_out, (void *)wParam, sizeof(dsp_ldw_out));
        adasFlag = 1;
        InvalidateRect(hWnd, &adas_rc, FALSE);
      } else {
        memset(&adas_out, 0, sizeof(dsp_ldw_out));
        adasFlag = 0;
      }
      break;
    case MSG_SDMOUNTFAIL:
      proc_MSG_SDMOUNTFAIL(hWnd);
      break;
    case MSG_SDNOTFIT:
      proc_MSG_SDNOTFIT(hWnd);
      break;
    case MSG_SDCARDFORMAT:
      unloadingWaitBmp(hWnd);
      if (lParam == EVENT_SDCARDFORMAT_FINISH) {
        printf("sd card format finish\n");
      } else if (lParam == EVENT_SDCARDFORMAT_FAIL) {
        int formatfail = MessageBox(hWnd, "sd卡格式话失败，是否重新格式化！", "警告!!!", MB_YESNO);
        if(formatfail == IDYES)
          cmd_IDM_RE_FORMAT(hWnd);
        else
          if(sdcard == 1)
             cvr_sd_ctl(0);
      }
      break;
    case MSG_VIDEOPLAY:
      if (lParam == EVENT_VIDEOPLAY_EXIT) {
        exitplayvideo(hWnd);
        // InvalidateRect (hWnd, &msg_rcTime, TRUE);
        if (test_replay)
          startplayvideo(hWnd, testpath);
        if (parameter_get_debug_modechange() == 1) {
          //	modechange_pre_flage =1;
          modechange_time = MODECHANGE_TIME - 1;
        } else if (parameter_get_debug_modechange() == 0) {
          //		modechange_pre_flage=0;
          modechange_time = 0;
        }
        if (parameter_get_debug_video() == 1) {
          video_time = 1;
        } else if (parameter_get_debug_video() == 0) {
          video_time = 0;
        }
      } else if (lParam == EVENT_VIDEOPLAY_UPDATETIME) {
        video_play_state = 1;
        sec = (int)wParam;
        if (parameter_get_debug_modechange() == 1) {
          modechange_time = MODECHANGE_TIME - 1;
        } else if (parameter_get_debug_modechange() == 0) {
          modechange_time = 0;
        }
        if (parameter_get_debug_video() == 1) {
          video_time = 1;
        } else if (parameter_get_debug_video() == 0) {
          video_time = 0;
        }
        InvalidateRect(hWnd, &msg_rcTime, FALSE);
      }
      break;
    case MSG_VIDEOREC:
      if (lParam == EVENT_VIDEOREC_UPDATETIME) {
        sec = (int)wParam;
        InvalidateRect(hWnd, &msg_rcTime, FALSE);
        InvalidateRect(hWnd, &msg_rcRecimg, FALSE);
      }
      break;
    case MSG_CAMERA: {
      // printf("%s wParam = %s, lParam = %d\n", __func__, wParam, lParam);
      if (lParam == 1) {
        struct ui_frame front = {parameter_get_video_frontcamera_width(),
                                 parameter_get_video_frontcamera_height(),
                                 parameter_get_video_frontcamera_fps()};

        struct ui_frame back = {parameter_get_video_backcamera_width(),
                                parameter_get_video_backcamera_height(),
                                parameter_get_video_backcamera_fps()};

        usleep(200000);
        video_record_addvideo(wParam, &front, &back, video_rec_state);
      } else {
        video_record_deletevideo(wParam);
      }
      break;
    }
    case MSG_ATTACH_USER_MUXER: {
      video_record_attach_user_muxer((void *)wParam, (void *)lParam, 1);
      break;
    }
    case MSG_ATTACH_USER_MDPROCESSOR: {
      video_record_attach_user_mdprocessor((void *)wParam, (void *)lParam);
      break;
    }
    case MSG_RECORD_RATE_CHANGE: {
      printf("!!! lParam: %d, video_rec_state: %d\n", (bool)lParam, video_rec_state);
      if (!(bool)lParam && !video_rec_state && sdcard == 1) {
        startrec0(hWnd);
        video_record_rate_change((void *)wParam, (int)lParam);
        break;
      }
      else if (lParam && video_rec_state) {
        video_record_rate_change((void *)wParam, (int)lParam);
        stoprec0(hWnd);
        break;
      }
      video_record_rate_change((void *)wParam, (int)lParam);
      break;
    }

    case MSG_FILTER: {
      // filterForUI= (int)lParam;
      break;
    }

    case MSG_USBCHAGE:
      proc_MSG_USBCHAGE(hWnd, message, wParam, lParam);
      break;
    case MSG_SDCHANGE:
      sdcard = (int)lParam;
	  FlagForUI_ca.sdcard_ui = sdcard;
      printf("MSG_SDCHANGE sdcard = %d\n", sdcard);

      if (sdcard == 1) {
        int ret = fs_manage_init(sdcardmount_back, (void*)hWnd);
        if (ret < 0) {
          sdcard = 0;
          proc_MSG_FS_INITFAIL(hWnd, ret);
          break;
        }

        if (parameter_get_video_de() == 1) {
          PostMessage(hMainWnd, MSG_COMMAND, IDM_detectON, 1);
        }

        video_record_get_user_noise();
      } else {
        if (parameter_get_video_de() == 1) {
          PostMessage(hMainWnd, MSG_COMMAND, IDM_detectOFF, 1);
        }
      }

      if (SetMode == MODE_RECORDING) {
        if (sdcard == 1) {
          if (parameter_get_video_autorec())
            startrec(hWnd);
        } else {
          stoprec(hWnd);
        }
        InvalidateRect(hWnd, &msg_rcSD, FALSE);
        InvalidateRect(hWnd, &msg_rcSDCAP, FALSE);
      } else if (SetMode == MODE_EXPLORER) {
        if (explorer_path == 0)
          explorer_path = calloc(1, 256);
        sprintf(explorer_path, "%s", SDCARD_PATH);
        explorer_update(explorer_wnd);
        InvalidateRect(hWnd, &msg_rcSD, FALSE);
        InvalidateRect(hWnd, &msg_rcSDCAP, FALSE);
      } else if (SetMode == MODE_PLAY) {
        videoplay_exit();
      } else if (SetMode == MODE_PREVIEW) {
        if (sdcard == 1) {
          entervideopreview();
          InvalidateRect(hWnd, NULL, FALSE);
        } else {
          exitvideopreview();
          InvalidateRect(hWnd, NULL, FALSE);
        }
      } else if (SetMode == MODE_PHOTO) {
        InvalidateRect(hWnd, &msg_rcSD, FALSE);
        InvalidateRect(hWnd, &msg_rcSDCAP, FALSE);
      }

      if (sdcard == 0) {
        fs_manage_deinit();
      }
      break;

    case MSG_BATTERY:
		cap = lParam;
		if (charging_status_check())
			battery = 0;
		else if ((cap >= 0) && (cap < (25 - BATTERY_CUSHION)))
			battery = 1;
		else if ((cap >= (25 + BATTERY_CUSHION)) && (cap < (50 - BATTERY_CUSHION)))
			battery = 2;
		else if ((cap >= (50 + BATTERY_CUSHION)) && (cap < (75 - BATTERY_CUSHION)))
			battery = 3;
		else if (cap >= (75 + BATTERY_CUSHION))
			battery = 4;
		else {
			if ((battery != 0) && (last_battery != 0))
				battery = last_battery;
			if (battery == 0) {
				if ((cap >= (25 - BATTERY_CUSHION)) && (cap < (25 + BATTERY_CUSHION)))
					battery  = 1;
				if ((cap >= (50 - BATTERY_CUSHION)) && (cap < (50 + BATTERY_CUSHION)))
					battery  = 2;
				if ((cap >= (75 - BATTERY_CUSHION)) && (cap < (75 + BATTERY_CUSHION)))
					battery  = 3;
			}
		}
		last_battery = battery;

		//printf("battery level=%d\n", battery);
		InvalidateRect(hWnd, &msg_rcBtt, FALSE);
		break;

    case MSG_TIMER:
#ifdef USE_WATCHDOG
      if (fd_wtd != -1)
		ioctl(fd_wtd, WDIOC_KEEPALIVE, 0);
#endif
      if (SetMode != MODE_PLAY) {
        if (screenoff_time > 0) {
          screenoff_time--;
          if (screenoff_time == 0) {
            rkfb_screen_off();
          }
        }
      }
      if (SetMode < MODE_PLAY) {
        InvalidateRect(hWnd, &msg_rcWifi, FALSE);
        InvalidateRect(hWnd, &msg_rcSDCAP, FALSE);
        InvalidateRect (hWnd, &msg_rcWatermarkTime, FALSE);
        InvalidateRect (hWnd, &msg_rcWatermarkImg, FALSE);
      }
      if (SetMode == MODE_RECORDING) {
        video_record_fps_count();
      }

      ui_timer_debug_process(hWnd);

      break;
    case MSG_PAINT:
      hdc = BeginPaint(hWnd);

      if (SetMode == MODE_USBCONNECTION) {
        FillBoxWithBitmap(hdc, USB_IMG_X, USB_IMG_Y, USB_IMG_W, USB_IMG_H,
                          &usb_bmap);
      } else {
        if (SetMode < MODE_PLAY) {
          char tf_cap[20];
          long long tf_free;
          long long tf_total;
          FillBoxWithBitmap(hdc, TOPBK_IMG_X, TOPBK_IMG_Y, g_rcScr.right,
                            TOPBK_IMG_H, &topbk_bmap);
          FillBoxWithBitmap(hdc, BATT_IMG_X, BATT_IMG_Y, BATT_IMG_W, BATT_IMG_H,
                            &batt_bmap[battery]);
          FillBoxWithBitmap(hdc, TF_IMG_X, TF_IMG_Y, TF_IMG_W, TF_IMG_H,
                            &tf_bmap[sdcard]);
          FillBoxWithBitmap(hdc, MODE_IMG_X, MODE_IMG_Y, MODE_IMG_W, MODE_IMG_H,
                            &mode_bmap[SetMode]);
          SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
          SetTextColor(hdc, PIXEL_lightwhite);
          SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
          if (sdcard == 1) {
            fs_manage_getsdcardcapacity(&tf_free, &tf_total);
            sprintf(tf_cap, "%4.1fG/%4.1fG", (float)tf_free / 1024,
                    (float)tf_total / 1024);
            TextOut(hdc, TFCAP_STR_X, TFCAP_STR_Y, tf_cap);
          }
          {
            char systime[22];
            time_t ltime;
            struct tm* today;
            time(&ltime);
            today = localtime(&ltime);
            sprintf(systime, "%04d-%02d-%02d %02d:%02d:%02d\n",
                    1900 + today->tm_year, today->tm_mon + 1, today->tm_mday,
                    today->tm_hour, today->tm_min, today->tm_sec);

            SetBkColor (hdc, bkcolor);
            TextOut (hdc, WATERMARK_TIME_X, WATERMARK_TIME_Y, systime);

#ifdef USE_WATERMARK
            if (((SetMode == MODE_RECORDING) || (SetMode == MODE_PHOTO)) &&
                parameter_get_video_mark()) {
              watermark_minigui_setHDC(hdc);
              video_record_update_time(hdc);
              FillBoxWithBitmap(hdc, WATERMARK_IMG_X, WATERMARK_IMG_Y,
                                WATERMARK_IMG_W, WATERMARK_IMG_H,
                                &watermark_bmap[0]);
            }
#endif

          }
          if (FlagForUI_ca.adasflagtooff == 1) {
            printf("adasflagtooff");
            SetBrushColor(hdc, bkcolor);
            SetBkColor(hdc, bkcolor);
            FillBox(hdc, adas_X, adas_Y, adas_W, adas_H);
            FlagForUI_ca.adasflagtooff = 0;
          }
          if (SetMode == MODE_RECORDING &&
              parameter_get_video_ldw() == 1 && adasFlag == 1) {
            SetBrushColor(hdc, bkcolor);
            SetBkColor(hdc, bkcolor);
            FillBox(hdc, adas_X, adas_Y, adas_W, adas_H);
            if (adas_out.endPoints[0][0] < 0 ||
                adas_out.endPoints[0][1] < 0 ||
                adas_out.endPoints[1][0] < 0 ||
                adas_out.endPoints[1][1] < 0 ||
                adas_out.endPoints[2][0] < 0 ||
                adas_out.endPoints[2][1] < 0 ||
                adas_out.endPoints[3][0] < 0 ||
                adas_out.endPoints[3][1] < 0) {
              // printf("data<1\n");
            } else {
              SetPenColor(hdc, PIXEL_red);
              MoveTo(hdc, (adas_out.endPoints[0][0] * WIN_W) / 320,
                     (adas_out.endPoints[0][1] * (WIN_H)) / 240);
              LineTo(hdc, (adas_out.endPoints[1][0] * WIN_W) / 320,
                     (adas_out.endPoints[1][1] * (WIN_H)) / 240);
              MoveTo(hdc, (adas_out.endPoints[2][0] * WIN_W) / 320,
                     (adas_out.endPoints[2][1] * (WIN_H)) / 240);
              LineTo(hdc, (adas_out.endPoints[3][0] * WIN_W) / 320,
                     (adas_out.endPoints[3][1] * (WIN_H)) / 240);
            }
            adasFlag = 0;
          }
        }

        if (SetMode == MODE_RECORDING) {
          FillBoxWithBitmap(hdc, MOVE_IMG_X, MOVE_IMG_Y, MOVE_IMG_W, MOVE_IMG_H,
                            &move_bmap[parameter_get_video_de()]);
          FillBoxWithBitmap(hdc, MIC_IMG_X, MIC_IMG_Y, MIC_IMG_W, MIC_IMG_H,
                            &mic_bmap[parameter_get_video_audio()]);
		  if (parameter_get_wifi_en() == 1) {
		    FillBoxWithBitmap(hdc, WIFI_IMG_X, WIFI_IMG_Y, WIFI_IMG_W, WIFI_IMG_H,
                            &wifi_bmap[4]);
          } else {
            SetBrushColor(hdc, bkcolor);
            SetBkColor(hdc, bkcolor);
            FillBox(hdc, WIFI_IMG_X, WIFI_IMG_Y, WIFI_IMG_W, WIFI_IMG_H);
          }
		  #if 0
		  {
		    char ab[3][3] = {"A","B","AB"};
		    char resolution[3][6] = {"720P","1080P","1440P"};
		    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
            SetTextColor(hdc, PIXEL_lightwhite);
            SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
		    TextOut(hdc, A_IMG_X, A_IMG_Y, ab[parameter_get_abmode()]);
		    TextOut(hdc, RESOLUTION_IMG_X, RESOLUTION_IMG_Y, resolution[parameter_get_vcamresolution()]);
		  }
		  #else
		  FillBoxWithBitmap(hdc, A_IMG_X, A_IMG_Y, A_IMG_W, A_IMG_H,
                            &A_bmap[parameter_get_abmode()]);
          FillBoxWithBitmap(hdc, RESOLUTION_IMG_X, RESOLUTION_IMG_Y,
                            RESOLUTION_IMG_W, RESOLUTION_IMG_H,
                            &resolution_bmap[parameter_get_vcamresolution()]);
		  #endif
        } else if (SetMode == MODE_PLAY) {
          SetBrushColor(hdc, bkcolor);
          FillBox(hdc, g_rcScr.left, g_rcScr.top, g_rcScr.right - g_rcScr.left,
                  g_rcScr.bottom - g_rcScr.top);
        } else if (SetMode == MODE_PREVIEW) {
          int x = g_rcScr.left;
          int y = g_rcScr.top + TOPBK_IMG_H;
          int w = g_rcScr.right - g_rcScr.left;
          int h = g_rcScr.bottom - y;
          SetBrushColor(hdc, bkcolor);
          SetBkColor(hdc, bkcolor);
          FillBox(hdc, x, y, w, h);
          TextOut(hdc, FILENAME_STR_X, FILENAME_STR_Y, previewname);
          if (preview_isvideo)
            FillBoxWithBitmap(hdc, (w - PLAY_IMG_W) / 2,
                              (h - PLAY_IMG_H) / 2 + TOPBK_IMG_H, PLAY_IMG_W,
                              PLAY_IMG_H, &play_bmap);
        }

        if (video_rec_state || video_play_state) {
          char strtime[20];

          FillBoxWithBitmap(hdc, REC_IMG_X, REC_IMG_Y, REC_IMG_W, REC_IMG_H,
                            &recimg_bmap);
          sprintf(strtime, "%02ld:%02ld:%02ld", (long int)(sec / 3600),
                  (long int)((sec / 60) % 60), (long int)(sec % 60));
          SetBkColor(hdc, bkcolor);
          SetTextColor(hdc, PIXEL_lightwhite);
          SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
          TextOut(hdc, REC_TIME_X, REC_TIME_Y, strtime);
        }
      }
      EndPaint(hWnd, hdc);
      break;

    //处理MSG_COMMAND消息,处理各个菜单明令
    case MSG_COMMAND:
      commandEvent(hWnd, wParam, lParam);
      break;

    case MSG_ACTIVEMENU:
      break;

    case MSG_CLOSE:
      deinitrec(hWnd);
      KillTimer(hWnd, _ID_TIMER);
      DestroyAllControls(hWnd);
      DestroyMainWindow(hWnd);
      PostQuitMessage(hWnd);
      return 0;
    case MSG_KEYDOWN: {
    	if (key_lock)
    		break;
      printf("%s MSG_KEYDOWN SetMode = %d, key = %d\n", __func__, SetMode,
             wParam);
#ifdef USE_KEY_STOP_USB_DIS_SHUTDOWN
      //key stop usb disconnect shutdown
      if ((wParam != 116)&&(gshutdown != 0)) {
        stop_usb_disconnect_poweroff();
      }
#endif
      if (SetMode == MODE_PREVIEW) {
        hwndforpre = hWnd;
        switch (wParam) {
          case SCANCODE_TAB: {
            changemode(hWnd, MODE_RECORDING);
            break;
          }
          case SCANCODE_CURSORBLOCKUP:
          case SCANCODE_CURSORBLOCKRIGHT: {
            videopreview_next(hWnd);
            break;
          }
          case SCANCODE_CURSORBLOCKDOWN:
          case SCANCODE_CURSORBLOCKLEFT: {
            videopreview_pre(hWnd);
            break;
          }
          case SCANCODE_ENTER: {
            if (videoplay != 0) {
              //	printf("videoplay==-1\n");
              if (parameter_get_video_lan() == 1)
                MessageBox(hwndforpre, "视频错误!!!", "警告!!!", MB_OK);
              else if (parameter_get_video_lan() == 0)
                MessageBox(hwndforpre, "video error !!!", "Warning!!!", MB_OK);
            } else {
              videopreview_play(hWnd);
            }
            break;
          }
          case SCANCODE_MENU: {
            videopreview_delete(hWnd);
            break;
          }
        }
      } else if (SetMode == MODE_USBDIALOG) {
      } else if (SetMode == MODE_PLAY) {
        switch (wParam) {
          case SCANCODE_TAB:
          case SCANCODE_Q:
          case SCANCODE_ESCAPE:
            test_replay = 0;
            videoplay_exit();
            break;
          case SCANCODE_SPACE:
          case SCANCODE_ENTER:
            // do pause
            videoplay_pause();
            break;
          case SCANCODE_S:
            videoplay_step_play();
            break;
          case SCANCODE_CURSORBLOCKLEFT:
            videoplay_seek_time(-5.0);
            break;
          case SCANCODE_CURSORBLOCKRIGHT:
            videoplay_seek_time(5.0);
            break;
          case SCANCODE_CURSORBLOCKUP:
            videoplay_set_speed(1);
            break;
          case SCANCODE_CURSORBLOCKDOWN:
            videoplay_set_speed(-1);
            break;
          default:
            printf("scancode: %d\n", wParam);
        }
      } else if (SetMode == MODE_EXPLORER) {
        switch (wParam) {
          case SCANCODE_A:
          case SCANCODE_ENTER:
            explorer_enter(hWnd);
            break;
          case SCANCODE_TAB: {
            changemode(hWnd, MODE_RECORDING);
            break;
          }
          case SCANCODE_Q:
            explorer_back(hWnd);
            break;
          case SCANCODE_D:
            explorer_filedelete(hWnd);
            break;
        }
      } else if (SetMode == MODE_RECORDING) {
        switch (wParam) {
          case SCANCODE_MENU:
          case SCANCODE_LEFTALT:
          case SCANCODE_RIGHTALT:
            printf("add by lqw:SCANCODE_RIGHTALT\n");
            //   CreateMainWindowMenu(hWnd, creatmenu());
            if (firstrunmenu == 0) {
              CreateMainWindowMenu(hWnd, creatmenu());
              firstrunmenu = 1;
            } else if ((video_record_get_front_resolution(frontcamera, 4) ==
                        0) ||
                       (video_record_get_back_resolution(backcamera, 4) == 0)) {
              DestroyMainWindowMenu(hWnd);
              CreateMainWindowMenu(hWnd, creatmenu());
            }
            if (video_rec_state == 0)
              popmenu(hWnd);
            break;
          case SCANCODE_CURSORBLOCKDOWN:
            mic_onoff(hWnd, parameter_get_video_audio() == 0 ? 1 : 0);
            break;
          case SCANCODE_CURSORBLOCKUP:
            if (video_rec_state == 0) {
              if (sdcard == 1) {
                startrec(hWnd);
              }
            } else {
              stoprec(hWnd);
            }
            break;
          case SCANCODE_CURSORBLOCKRIGHT:
            proc_record_SCANCODE_CURSORBLOCKRIGHT(hWnd);
            break;
          case SCANCODE_TAB: {
            // changemode(hWnd, MODE_EXPLORER);
            changemode(hWnd, MODE_PHOTO);
            // changemode(hWnd, MODE_PREVIEW);
            break;
          }
          case SCANCODE_ENTER:
            ui_takephoto(hWnd);
            break;
          case SCANCODE_ESCAPE:
            video_record_display_switch();
            break;

          case SCANCODE_B:  // block video save
            video_record_blocknotify(BLOCK_PREV_NUM, BLOCK_LATER_NUM);
            video_record_savefile();
            printf("The envent SCANCODE_B\n");
            break;
        }
      } else if (SetMode == MODE_PHOTO) {
        switch (wParam) {
          case SCANCODE_MENU:
          case SCANCODE_RIGHTALT:
            if (video_rec_state == 0)
              popmenu(hWnd);
            break;
          case SCANCODE_TAB:
            changemode(hWnd, MODE_PREVIEW);
            break;
          case SCANCODE_ENTER:
            ui_takephoto(hWnd);
            break;
        }
      }
    } break;

	case MSG_KEYLONGPRESS:
		if (wParam == SCANCODE_SHUTDOWN) {
			shutdown_deinit(hWnd);
		}
      break;

	case MSG_USB_DISCONNECT:
		if (wParam == SCANCODE_SHUTDOWN) {
			shutdown_usb_disconnect(hWnd);
		}
      break;


    case MSG_KEYALWAYSPRESS:
      // printf("MSG_KEYALWAYSPRESS key = %d\n", wParam);
      break;

    case MSG_KEYUP:
      // printf("MSG_KEYUP key = %d\n", wParam);
      switch (wParam) {
        default:
          break;
      }
      break;
    case MSG_MAINWIN_KEYDOWN:
      // test code start
      if (SetMode != MODE_PLAY && SetMode != MODE_PREVIEW)
        audio_play(keypress_sound);
      // test code end

      if (screenoff_time == 0) {
        screenoff_time = parameter_get_screenoff_time();
        rkfb_screen_on();
      } else if (screenoff_time > 0) {
        screenoff_time = parameter_get_screenoff_time();
      }
      break;
    case MSG_MAINWIN_KEYUP:
      break;
    case MSG_MAINWIN_KEYLONGPRESS:
      break;
    case MSG_MAINWIN_KEYALWAYSPRESS:
      break;
  }

  return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int loadres(void) {
  int i;
  char img[128];
  char respath[] = "/usr/local/share/minigui/res/images/";

  char debug_img[2][20] = {"debug.bmp", "debug_sel.bmp"};
  char filetype_img[FILE_TYPE_MAX][20] = {"file_unknown.bmp", "file_jpeg.bmp",
                                          "file_video.bmp"};
  char play_img[] = "play.bmp";
  char back_img[] = "file_back.bmp";
  char usb_img[] = "usb.bmp";
  char folder_img[] = "folder.bmp";
  char mic_img[2][20] = {"mic_off.bmp", "mic_on.bmp"};
  char A_img[3][10] = {"a.bmp", "b.bmp", "ab.bmp"};
  char topbk_img[] = "top_bk.bmp";
  char rec_img[] = "rec.bmp";
  char watermark_img[7][30] = {"watermark.bmp", "watermark_240p.bmp", "watermark_360p.bmp", "watermark_480p.bmp",
                                "watermark_720p.bmp", "watermark_1080p.bmp", "watermark_1440p.bmp"};
  char batt_img[5][20] = {"battery_0.bmp", "battery_1.bmp", "battery_2.bmp",
                          "battery_3.bmp", "battery_4.bmp"};
  char tf_img[2][20] = {"tf_out.bmp", "tf_in.bmp"};
  char mode_img[4][20] = {"icon_video.bmp", "icon_camera.bmp", "icon_play.bmp",
                          "icon_play.bmp"};
  char resolution_img[2][30] = {"resolution_720p.bmp", "resolution_1080p.bmp"};
  char move_img[2][20] = {"move_0.bmp", "move_1.bmp"};
  char mp_img[2][20] = {"mp.bmp", "mp_sel.bmp"};
  char car_img[2][20] = {"car.bmp", "car_sel.bmp"};
  char time_img[2][20] = {"time.bmp", "time_sel.bmp"};
  char setting_img[2][20] = {"setting.bmp", "setting_sel.bmp"};
  char wifi_img[5][20] = {"wifi_0.bmp", "wifi_1.bmp", "wifi_2.bmp",
  	                      "wifi_3.bmp", "wifi_4.bmp"};

  /*load wifi bmp*/
  for (i = 0; i < (sizeof(wifi_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, wifi_img[i]);
    if (LoadBitmap(HDC_SCREEN, &wifi_bmap[i], img))
      return -1;
  }

  // load debug
  for (i = 0; i < (sizeof(png_menu_debug) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, debug_img[i]);
    if (LoadBitmap(HDC_SCREEN, &png_menu_debug[i], img))
      return -1;
  }
  /*load filetype bmp*/
  for (i = 0; i < (sizeof(filetype_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, filetype_img[i]);
    if (LoadBitmap(HDC_SCREEN, &filetype_bmap[i], img))
      return -1;
  }

  /* load play bmp */
  sprintf(img, "%s%s", respath, play_img);
  if (LoadBitmap(HDC_SCREEN, &play_bmap, img))
    return -1;

  /* load back bmp */
  sprintf(img, "%s%s", respath, back_img);
  if (LoadBitmap(HDC_SCREEN, &back_bmap, img))
    return -1;

  // load usb bmp
  sprintf(img, "%s%s", respath, usb_img);
  if (LoadBitmap(HDC_SCREEN, &usb_bmap, img))
    return -1;

  /* load folder bmp */
  sprintf(img, "%s%s", respath, folder_img);
  if (LoadBitmap(HDC_SCREEN, &folder_bmap, img))
    return -1;

  /*load mic bmp*/
  for (i = 0; i < (sizeof(mic_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, mic_img[i]);
    if (LoadBitmap(HDC_SCREEN, &mic_bmap[i], img))
      return -1;
  }

  /*load a bmp*/
  for (i = 0; i < (sizeof(A_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, A_img[i]);
    if (LoadBitmap(HDC_SCREEN, &A_bmap[i], img))
      return -1;
  }

  /* load topbk bmp */
  sprintf(img, "%s%s", respath, topbk_img);
  if (LoadBitmap(HDC_SCREEN, &topbk_bmap, img))
    return -1;

  /* load recimg bmp */
  sprintf(img, "%s%s", respath, rec_img);
  if (LoadBitmap(HDC_SCREEN, &recimg_bmap, img))
    return -1;

  /* load watermark bmp */
  for (i = 0; i < (sizeof(watermark_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, watermark_img[i]);
    if (LoadBitmap(HDC_SCREEN, &watermark_bmap[i], img)){
      printf("load watermark bmp error, i = %d\n", i);
      return -1;
    }
  }

  /* load batt bmp */
  for (i = 0; i < (sizeof(batt_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, batt_img[i]);
    if (LoadBitmap(HDC_SCREEN, &batt_bmap[i], img))
      return -1;
  }

  /* load tf card bmp */
  for (i = 0; i < (sizeof(tf_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, tf_img[i]);
    if (LoadBitmap(HDC_SCREEN, &tf_bmap[i], img))
      return -1;
  }

  /* load mode bmp */
  for (i = 0; i < (sizeof(mode_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, mode_img[i]);
    if (LoadBitmap(HDC_SCREEN, &mode_bmap[i], img))
      return -1;
  }

  /* load resolution bmp */
  for (i = 0; i < (sizeof(resolution_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, resolution_img[i]);
    if (LoadBitmap(HDC_SCREEN, &resolution_bmap[i], img))
      return -1;
  }

  /* load move bmp */
  for (i = 0; i < (sizeof(move_bmap) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, move_img[i]);
    if (LoadBitmap(HDC_SCREEN, &move_bmap[i], img))
      return -1;
  }

  /* load mp bmp */
  for (i = 0; i < (sizeof(bmp_menu_mp) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, mp_img[i]);
    if (LoadBitmap(HDC_SCREEN, &bmp_menu_mp[i], img))
      return -1;
  }

  /* load car bmp */
  for (i = 0; i < (sizeof(bmp_menu_car) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, car_img[i]);
    if (LoadBitmap(HDC_SCREEN, &bmp_menu_car[i], img))
      return -1;
  }

  /* load time bmp */
  for (i = 0; i < (sizeof(bmp_menu_time) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, time_img[i]);
    if (LoadBitmap(HDC_SCREEN, &bmp_menu_time[i], img))
      return -1;
  }
  /* load setting bmp */
  for (i = 0; i < (sizeof(bmp_menu_setting) / sizeof(BITMAP)); i++) {
    sprintf(img, "%s%s", respath, setting_img[i]);
    if (LoadBitmap(HDC_SCREEN, &bmp_menu_setting[i], img))
      return -1;
  }

  return 0;
}

void unloadres(void) {
  int i;

  /* unload wifi bmp */
  for (i = 0; i < (sizeof(wifi_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&wifi_bmap[i]);
  }
 
  /* unload filetype bmp */
  for (i = 0; i < (sizeof(filetype_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&filetype_bmap[i]);
  }

  /* unload back bmp */
  UnloadBitmap(&play_bmap);

  /* unload back bmp */
  UnloadBitmap(&back_bmap);

  /* unload folder bmp */
  UnloadBitmap(&folder_bmap);

  /* unload topbk bmp */
  UnloadBitmap(&topbk_bmap);

  /* unload recimg bmp */
  UnloadBitmap(&recimg_bmap);

  /* unload watermark bmp */
  for (i = 0; i < (sizeof(watermark_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&watermark_bmap[i]);
  }

  // unload usb bmp
  UnloadBitmap(&usb_bmap);
  /* unload mic bmp */
  for (i = 0; i < (sizeof(mic_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&mic_bmap[i]);
  }

  /* unload a bmp */
  for (i = 0; i < (sizeof(A_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&A_bmap[i]);
  }

  /* unload batt bmp */
  for (i = 0; i < (sizeof(batt_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&batt_bmap[i]);
  }

  /* unload tf card bmp */
  for (i = 0; i < (sizeof(tf_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&tf_bmap[i]);
  }

  /* unload mode bmp */
  for (i = 0; i < (sizeof(mode_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&mode_bmap[i]);
  }

  /* unload resolution bmp */
  for (i = 0; i < (sizeof(resolution_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&resolution_bmap[i]);
  }

  /* unload move bmp */
  for (i = 0; i < (sizeof(move_bmap) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&move_bmap[i]);
  }

  /* unload mp bmp */
  for (i = 0; i < (sizeof(bmp_menu_mp) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&bmp_menu_mp[i]);
  }

  /*un load car bmp */
  for (i = 0; i < (sizeof(bmp_menu_car) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&bmp_menu_car[i]);
  }

  /* unload time bmp */
  for (i = 0; i < (sizeof(bmp_menu_time) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&bmp_menu_time[i]);
  }

  /* unload setting bmp */
  for (i = 0; i < (sizeof(bmp_menu_setting) / sizeof(BITMAP)); i++) {
    UnloadBitmap(&bmp_menu_setting[i]);
  }
}

int video_SendMessage(HWND hWnd, int iMsg, WPARAM wParam, LPARAM lParam) {
  return PostMessage(hWnd, iMsg, wParam, lParam);
}

void video_rkfb_get_resolution(int* width, int* height) {
  rkfb_get_resolution(width, height);
}

#if DEBUG
extern void init_dump(void);
#endif

#if HAVE_GUI
int MiniGUIMain(int argc, const char* argv[])
{
	MSG Msg;
	MAINWINCREATE CreateInfo;
	struct color_key key;
#ifdef USE_WATCHDOG
	char pathname[] = "/dev/watchdog";

	printf ("use watchdog\n");
	fd_wtd = open(pathname, O_WRONLY);
	if (fd_wtd == -1)
		printf ("/dev/watchdog open error\n");
	else {
		ioctl(fd_wtd, WDIOC_KEEPALIVE, 0);
	}
#endif
	android_usb_init();

	printf("camera run\n");
#if DEBUG
	init_dump();
#endif
	parameter_init();
    //if the sdcard have't be umount, will run fsck, maybe spend some time
//	fs_manage_sdcard_fsck();
	
	screenoff_time = parameter_get_screenoff_time();
	if (0 != audio_dev_init()) {
		printf("audio_dev_init failed\n");
	}
	// test code start
	// file path should be read from config_file, TODO
	// these files are copied from $ANDROID_PROJECT/frameworks/base/data/sounds
	if (audio_play_init(&keypress_sound,
                      "/usr/local/share/sounds/KeypressStandard.wav", 1)) {
		printf("keypress sound init failed\n");
	}
	if (audio_play_init(&capture_sound,
                      "/usr/local/share/sounds/camera_click.wav", 1)) {
		printf("capture sound init failed\n");
	}
	// test code end

	rk_fb_set_lcd_backlight(parameter_get_video_backlt());
	if (loadres()) {
		printf("loadres fail\n");
		// return -1;
	}

	pthread_mutex_init(&set_mutex, NULL);

	memset(&CreateInfo, 0, sizeof(MAINWINCREATE));
	CreateInfo.dwStyle = WS_VISIBLE | WS_HIDEMENUBAR | WS_WITHOUTCLOSEMENU;
	CreateInfo.dwExStyle = WS_EX_NONE | WS_EX_AUTOSECONDARYDC;
	CreateInfo.spCaption = "camera";
	//	CreateInfo.hMenu=creatmenu();
	CreateInfo.hCursor = GetSystemCursor(0);
	CreateInfo.hIcon = 0;
	CreateInfo.MainWindowProc = CameraWinProc;
	CreateInfo.lx = 0;
	CreateInfo.ty = 0;
	CreateInfo.rx = g_rcScr.right;
	CreateInfo.by = g_rcScr.bottom;
	// CreateInfo.iBkColor=COLOR_red;
	CreateInfo.dwAddData = 0;
	CreateInfo.hHosting = HWND_DESKTOP;
	CreateInfo.language = parameter_get_video_lan();
	// fprintf( stderr, "%d---%d\n",g_rcScr.right,g_rcScr.bottom);
	hMainWnd = CreateMainWindow(&CreateInfo);

	if (hMainWnd == HWND_INVALID)
		return -1;
	RegisterMainWindow(hMainWnd);
	bkcolor = GetWindowElementPixel(hMainWnd, WE_BGC_DESKTOP);
	SetWindowBkColor(hMainWnd, bkcolor);

	key.blue = (bkcolor & 0x1f) << 3;
	key.green = ((bkcolor >> 5) & 0x3f) << 2;
	key.red = ((bkcolor >> 11) & 0x1f) << 3;
	key.enable = 1;
	if (rkfb_set_color_key(&key) == -1) {
		printf("rkfb_set_color_key err\n");
	}
	if (rkfb_set_color_key(&key) == -1) {
		printf("rkfb_set_color_key err\n");
	}
	ShowWindow(hMainWnd, SW_SHOWNORMAL);
	gsensor_init();
	temp_control_init();
	//
	sd_reg_event_callback(sd_event_callback);
	usb_reg_event_callback(usb_event_callback);
	batt_reg_event_callback(batt_event_callback);
	uevent_monitor_run();

	//
	REC_RegEventCallback(rec_event_callback);
	USER_RegEventCallback(user_event_callback);
	VIDEOPLAY_RegEventCallback(videoplay_event_callback);
	GSENSOR_RegEventCallback(gsensor_event_callback);
	hdmi_reg_event_callback(hdmi_event_callback);
	camera_reg_event_callback(camere_event_callback);
	
#ifdef USE_WATERMARK
  watermark_minigui_setHWND(hMainWnd);
#endif
	video_record_setaudio(parameter_get_video_audio());
	initrec(hMainWnd);
    if (parameter_get_wifi_en() == 1)
      wifi_management_start();

	GetMainWindowHwnd(hMainWnd);
	FlagForUI_ca.adasflagtooff = 0;
	FlagForUI_ca.formatflag = 0;
	FlagForUI_ca.setmode_ui = 0;
	FlagForUI_ca.sdcard_ui = sdcard;
	FlagForUI_ca.video_rec_state_ui = video_rec_state;
#ifdef MSG_FWK
	fwk_glue_init();
	fwk_controller_init();
#endif

#ifdef PROTOCOL_GB28181
	rk_gb28181_init();
#endif
	while (GetMessage(&Msg, hMainWnd)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

#ifdef USE_WATCHDOG
	//printf ("close watchdog\n");
	//sleep(2);
	//if (fd_wtd != -1) {
		//write(fd_wtd, "V", 1);
		//close(fd_wtd);
	//}
#endif

#ifdef PROTOCOL_GB28181
	rk_gb28181_destroy();
#endif

#ifdef MSG_FWK
	fwk_controller_destroy();
	fwk_glue_destroy();
#endif

    wifi_management_stop();
    video_record_deinit();
    videoplay_deinit();
	fs_manage_deinit();
	gsensor_release();
	UnregisterMainWindow(hMainWnd);
	MainWindowThreadCleanup(hMainWnd);
	unloadres();

	// test code start
	audio_play_deinit(keypress_sound);
	audio_play_deinit(capture_sound);
	// test code end
	audio_dev_deinit();

	printf("camera exit\n");
	return 0;
}
#endif

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif
