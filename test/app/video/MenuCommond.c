#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <errno.h>

#if HAVE_GUI
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
#include "MenuCommond.h"
#include "common.h"
#include "videoplay.h"
#include <dpp/dpp_frame.h>

#include "audioplay.h"
#include <assert.h>

//#include <minigui/fbvideo.h>
#include <pthread.h>
extern RECT msg_rcAB;

extern RECT adas_rc;

extern RECT msg_rcMove;

extern struct FlagForUI FlagForUI_ca;
HWND HwndForMenuCommand;
static struct ui_frame frontcamera_menu[2] = {0},backcamera_menu[2] = {0};
int getfontcameranum(void)
{
	return sizeof(frontcamera_menu)/sizeof(frontcamera_menu[0]);
}
int getbackcameranum(void)
{
	return sizeof(backcamera_menu)/sizeof(backcamera_menu[0]);
}
struct ui_frame *getfontcamera(int n)
{
    video_record_get_front_resolution(frontcamera_menu,2);
	return &frontcamera_menu[n];
}
struct ui_frame *getbackcamera(int n)
{
	video_record_get_back_resolution(backcamera_menu,2);
	return &backcamera_menu[n];
}

void GetMainWindowHwnd(HWND hWnd)
{
	HwndForMenuCommand = hWnd;
}

int setting_func_recovery_ui()
{
	if(FlagForUI_ca.video_rec_state_ui ==0)
		{
//			stoprec(HwndForMenuCommand);
		    printf("setting_func_recovery\n");
		    parameter_recover();
				runapp("busybox reboot");
//			startrec(HwndForMenuCommand);
			return 1;
		}
	else
		{
			return 0;
		}
}

int setting_func_USBMODE_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==0)
		{
		    if(strcmp(str, "msc")==0){
		        printf("SET usb mode msc\n");
		        parameter_save_video_usb(0);
				android_usb_config_ums();
		    }else if(strcmp(str, "adb")==0){
		        printf("SET usb mode adb\n");
		        parameter_save_video_usb(1);
				android_usb_config_adb();
		    }
			return 1;
		}
	else
		{
			return 0;
		}
}
int setting_func_phtomp_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);
		    if(strcmp(str, "1m")==0){
		        parameter_save_vcamresolution_photo(0);
					ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
		    }else if(strcmp(str, "2m")==0){
		        parameter_save_vcamresolution_photo(1);
					ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
		    }
			else if(strcmp(str, "3m")==0){
		        parameter_save_vcamresolution_photo(2);
					ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
		    }
			startrec(HwndForMenuCommand);
		}
	else if(FlagForUI_ca.video_rec_state_ui ==0)
		{
			if(strcmp(str, "1m")==0){
			        parameter_save_vcamresolution_photo(0);
						ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
			    }else if(strcmp(str, "2m")==0){
			        parameter_save_vcamresolution_photo(1);
						ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
			    }
				else if(strcmp(str, "3m")==0){
			        parameter_save_vcamresolution_photo(2);
						ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
			    }
		}
}

int setting_func_video_length_ui(char * str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
			{
				stoprec(HwndForMenuCommand);
			    if(strcmp(str, "1min")==0){
			        parameter_save_recodetime(60);
					InvalidateRect (HwndForMenuCommand, &msg_rcAB, FALSE);
			    }else if(strcmp(str, "3min")==0){
			        parameter_save_recodetime(180);
					InvalidateRect (HwndForMenuCommand, &msg_rcAB, FALSE);
			    }else if(strcmp(str, "5min")==0){
			        parameter_save_recodetime(300);
					InvalidateRect (HwndForMenuCommand, &msg_rcAB, FALSE);
			    }
				startrec(HwndForMenuCommand);
		}
	else if(FlagForUI_ca.video_rec_state_ui ==0)
		{
		  		if(strcmp(str, "1min")==0){
			        parameter_save_recodetime(60);
					InvalidateRect (HwndForMenuCommand, &msg_rcAB, FALSE);
			    }else if(strcmp(str, "3min")==0){
			        parameter_save_recodetime(180);
					InvalidateRect (HwndForMenuCommand, &msg_rcAB, FALSE);
			    }else if(strcmp(str, "5min")==0){
			        parameter_save_recodetime(300);
					InvalidateRect (HwndForMenuCommand, &msg_rcAB, FALSE);
			    }
		}
}
int setting_func_3dnr_ui(char * str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);
		    if(strcmp(str, "ON")==0){
		        parameter_save_video_3dnr(1);
				ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
		    }else if(strcmp(str, "OFF")==0){
		        parameter_save_video_3dnr(0);
				ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
		    }
			startrec(HwndForMenuCommand);
		}
	else if (FlagForUI_ca.video_rec_state_ui ==0)
		{
				if(strcmp(str, "ON")==0){
		        parameter_save_video_3dnr(1);
				ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
		    }else if(strcmp(str, "OFF")==0){
		        parameter_save_video_3dnr(0);
				ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
		    }
		}
}
//
int GetformatStatus(void)
{
    return FlagForUI_ca.formatflag;
}
int GetMode_ui(void)
{
		return FlagForUI_ca.setmode_ui;
}
int GetSrcState(void)
{
	return FlagForUI_ca.video_rec_state_ui;
}
extern void (*wififormat_callback)(void);
//


int setting_func_format_ui(void (*p)(void))
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);
		    wififormat_callback = p;
			if(FlagForUI_ca.formatflag==0)
			{
				fs_manage_format_sdcard(sdcardformat_back, (void *)HwndForMenuCommand,0);
		    }
		}
		else
			{
			   	wififormat_callback = p;
				if(FlagForUI_ca.formatflag==0)
				{
					fs_manage_format_sdcard(sdcardformat_back, (void *)HwndForMenuCommand,0);
			    }
			}
}
int setting_func_ldw_ui(char * str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
	{
		stoprec(HwndForMenuCommand);

	    if(strcmp(str, "ON")==0){
	        parameter_save_video_ldw(1);
			ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
	    }else if(strcmp(str, "OFF")==0){
	        parameter_save_video_ldw(0);
		    ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
		    FlagForUI_ca.adasflagtooff = 1;
			InvalidateRect (HwndForMenuCommand, &adas_rc, FALSE);
	    }
		startrec(HwndForMenuCommand);
		}
	else 
		{
		 if(strcmp(str, "ON")==0){
	        parameter_save_video_ldw(1);
			ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
	    }else if(strcmp(str, "OFF")==0){
	        parameter_save_video_ldw(0);
		    ui_deinit_init_camera(HwndForMenuCommand,-1,-1);
		    FlagForUI_ca.adasflagtooff = 1;
			InvalidateRect (HwndForMenuCommand, &adas_rc, FALSE);
	    }
		}
}

int setting_func_white_balance_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);

		    if(strcmp(str, "auto")==0){
			   ui_set_white_balance(0);
		    }else if(strcmp(str, "Daylight")==0){
		       ui_set_white_balance(1);
		    }else if(strcmp(str, "fluocrescence")==0){
		        ui_set_white_balance(2);
		    }else if(strcmp(str, "cloudysky")==0){
		        ui_set_white_balance(3);
		    }else if(strcmp(str, "tungsten")==0){
		      ui_set_white_balance(4);
		    }
			startrec(HwndForMenuCommand);
		}
	else
		{
			if(strcmp(str, "auto")==0){
			   ui_set_white_balance(0);
		    }else if(strcmp(str, "Daylight")==0){
		       ui_set_white_balance(1);
		    }else if(strcmp(str, "fluocrescence")==0){
		        ui_set_white_balance(2);
		    }else if(strcmp(str, "cloudysky")==0){
		        ui_set_white_balance(3);
		    }else if(strcmp(str, "tungsten")==0){
		      ui_set_white_balance(4);
		    }
		}
}
int setting_func_exposure_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);

		    if(strcmp(str, "-3")==0){
		       ui_set_exposure_compensation(0);
		    }else if(strcmp(str, "-2")==0){
		        ui_set_exposure_compensation(1);
		    }else if(strcmp(str, "-1")==0){
		       ui_set_exposure_compensation(2);
		    }else if(strcmp(str, "0")==0){
		        ui_set_exposure_compensation(3);
		    }else if(strcmp(str, "1")==0){
		        ui_set_exposure_compensation(4);
		    }
			startrec(HwndForMenuCommand);
		}
	else 
		{
		   if(strcmp(str, "-3")==0){
		       ui_set_exposure_compensation(0);
		    }else if(strcmp(str, "-2")==0){
		        ui_set_exposure_compensation(1);
		    }else if(strcmp(str, "-1")==0){
		       ui_set_exposure_compensation(2);
		    }else if(strcmp(str, "0")==0){
		        ui_set_exposure_compensation(3);
		    }else if(strcmp(str, "1")==0){
		        ui_set_exposure_compensation(4);
		    }
		}
}

int setting_func_motion_detection_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);

		    if(strcmp(str, "ON")==0){
		        parameter_save_video_de(1);
				InvalidateRect (HwndForMenuCommand, &msg_rcMove, FALSE);
		    }else if(strcmp(str, "OFF")==0){
		        parameter_save_video_de(0);
				InvalidateRect (HwndForMenuCommand, &msg_rcMove, FALSE);
		    }
			startrec(HwndForMenuCommand);
		}
	else
		{
		  if(strcmp(str, "ON")==0){
		        parameter_save_video_de(1);
				InvalidateRect (HwndForMenuCommand, &msg_rcMove, FALSE);
		    }else if(strcmp(str, "OFF")==0){
		        parameter_save_video_de(0);
				InvalidateRect (HwndForMenuCommand, &msg_rcMove, FALSE);
		    }
		}
}
int setting_func_data_stamp_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
	{
		stoprec(HwndForMenuCommand);

	    if(strcmp(str, "ON")==0){
	        parameter_save_video_mark(1);
	    }else if(strcmp(str, "OFF")==0){
	        parameter_save_video_mark(0);
	    }
		startrec(HwndForMenuCommand);
	}
	else
	{
		
	    if(strcmp(str, "ON")==0){
	        parameter_save_video_mark(1);
	    }else if(strcmp(str, "OFF")==0){
	        parameter_save_video_mark(0);
	    }
	}
}
int setting_func_record_audio_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
	{
		stoprec(HwndForMenuCommand);
	    if(strcmp(str, "ON")==0){
	       mic_onoff(HwndForMenuCommand, 1);
	    }else if(strcmp(str, "OFF")==0){
	       mic_onoff(HwndForMenuCommand, 0);
	    }
		startrec(HwndForMenuCommand);
	}
	else 
		{
			if(strcmp(str, "ON")==0){
		       mic_onoff(HwndForMenuCommand, 1);
		    }else if(strcmp(str, "OFF")==0){
		       mic_onoff(HwndForMenuCommand, 0);
		    }
		}
}
int setting_func_language_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);

		    if(strcmp(str, "English")==0){
		        printf("SET language English\n");
		        parameter_save_video_lan(0);
				SetSystemLanguage(HwndForMenuCommand, 0);
		    }else if(strcmp(str, "Chinese")==0){
		        printf("SET language Chinese\n");
		        parameter_save_video_lan(1);
				SetSystemLanguage(HwndForMenuCommand, 1);
		    }
			startrec(HwndForMenuCommand);
		}
	else
		{
			 if(strcmp(str, "English")==0){
		        printf("SET language English\n");
		        parameter_save_video_lan(0);
				SetSystemLanguage(HwndForMenuCommand, 0);
		    }else if(strcmp(str, "Chinese")==0){
		        printf("SET language Chinese\n");
		        parameter_save_video_lan(1);
				SetSystemLanguage(HwndForMenuCommand, 1);
		    }
		}
}

int setting_func_backlight_lcd_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);

		    if(strcmp(str, "high")==0){
		       parameter_save_video_backlt(LCD_BACKLT_H);
					rk_fb_set_lcd_backlight(LCD_BACKLT_H);
		    }else if(strcmp(str, "mid")==0){
		       parameter_save_video_backlt(LCD_BACKLT_M);
					rk_fb_set_lcd_backlight(LCD_BACKLT_M);
		    }
			else if(strcmp(str, "low")==0){
		        parameter_save_video_backlt(LCD_BACKLT_L);
				rk_fb_set_lcd_backlight(LCD_BACKLT_L);
		    }	
			startrec(HwndForMenuCommand);
		}
	else
		{
		 	if(strcmp(str, "high")==0){
		       parameter_save_video_backlt(LCD_BACKLT_H);
					rk_fb_set_lcd_backlight(LCD_BACKLT_H);
		    }else if(strcmp(str, "mid")==0){
		       parameter_save_video_backlt(LCD_BACKLT_M);
					rk_fb_set_lcd_backlight(LCD_BACKLT_M);
		    }
			else if(strcmp(str, "low")==0){
		        parameter_save_video_backlt(LCD_BACKLT_L);
				rk_fb_set_lcd_backlight(LCD_BACKLT_L);
		    }	
		}
}
int setting_func_cammp_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
			{
				stoprec(HwndForMenuCommand);

			    if(strcmp(str, "font1")==0){
			       ui_deinit_init_camera(HwndForMenuCommand,0,-1);
			    }else if(strcmp(str, "font2")==0){
			       	ui_deinit_init_camera(HwndForMenuCommand,1,-1);
			    }
				else if(strcmp(str, "back1")==0){
			        ui_deinit_init_camera(HwndForMenuCommand,-1,0);
			    }else if(strcmp(str, "back2")==0){
			        ui_deinit_init_camera(HwndForMenuCommand,-1,1);
			    }
				startrec(HwndForMenuCommand);
		}
	else
		{
			if(strcmp(str, "font1")==0){
			       ui_deinit_init_camera(HwndForMenuCommand,0,-1);
			    }else if(strcmp(str, "font2")==0){
			       	ui_deinit_init_camera(HwndForMenuCommand,1,-1);
			    }
				else if(strcmp(str, "back1")==0){
			        ui_deinit_init_camera(HwndForMenuCommand,-1,0);
			    }else if(strcmp(str, "back2")==0){
			        ui_deinit_init_camera(HwndForMenuCommand,-1,1);
			    }
		}
}
int setting_func_setdata_ui(char *str)
{
	struct tm tm;
	int year;
	int mon;
	int mday;
	int hour;
	int min;
	int sec;
	sscanf(str,"%u-%u-%u %u:%u:%u",&year,&mon,&mday,&hour,&min,&sec);
	if(FlagForUI_ca.video_rec_state_ui ==1)
			{
				stoprec(HwndForMenuCommand);
				tm.tm_year = year-1900;
				tm.tm_mon = mon-1;
				tm.tm_mday = mday;
				tm.tm_hour = hour;
				tm.tm_min = min;
				tm.tm_sec = sec;
				setDateTime(&tm);
				startrec(HwndForMenuCommand);
		}
	else
		{
				tm.tm_year = year-1900;
				tm.tm_mon = mon-1;
				tm.tm_mday = mday;
				tm.tm_hour = hour;
				tm.tm_min = min;
				tm.tm_sec = sec;
				setDateTime(&tm);
		}
}
//
int setting_func_frequency_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);
			if(strcmp(str, "50HZ")==0){
			   cmd_IDM_frequency(CAMARE_FREQ_50HZ);
			}else if(strcmp(str, "60HZ")==0){
				cmd_IDM_frequency(CAMARE_FREQ_50HZ);
			}
			startrec(HwndForMenuCommand);
		}
	else
		{
			if(strcmp(str, "50HZ")==0){
			   cmd_IDM_frequency(CAMARE_FREQ_50HZ);
			}else if(strcmp(str, "60HZ")==0){
				cmd_IDM_frequency(CAMARE_FREQ_50HZ);
			}
		}
}
int setting_func_autorecord_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);
			if(strcmp(str, "ON")==0){
			   parameter_save_video_autorec(1);
			}else if(strcmp(str, "OFF")==0){
				parameter_save_video_autorec(0);
			}
			startrec(HwndForMenuCommand);
		}
	else
		{
			if(strcmp(str, "ON")==0){
			   parameter_save_video_autorec(1);
			}else if(strcmp(str, "OFF")==0){
				parameter_save_video_autorec(0);
			}
		}
}
int setting_func_car_ui(char *str)
{
	if(FlagForUI_ca.video_rec_state_ui ==1)
		{
			stoprec(HwndForMenuCommand);

			if(strcmp(str, "Front video")==0){
			   cmd_IDM_CAR(HwndForMenuCommand, 0);
			}else if(strcmp(str, "Rear video")==0){
				cmd_IDM_CAR(HwndForMenuCommand, 1);
			}
			else if(strcmp(str, "Double video")==0){
				cmd_IDM_CAR(HwndForMenuCommand, 2);
			}
			startrec(HwndForMenuCommand);
		}
	else
		{
			if(strcmp(str, "Front video")==0){
			    cmd_IDM_CAR(HwndForMenuCommand, 0);
			}else if(strcmp(str, "Rear video")==0){
				cmd_IDM_CAR(HwndForMenuCommand, 1);
			}
			else if(strcmp(str, "Double video")==0){
				cmd_IDM_CAR(HwndForMenuCommand, 2);
			}
		}
}

int setting_func_modechage_ui(char *str)
	{
		
		if(strcmp(str, "photo")==0){
			if(GetMode_ui()==0)
		  		changemode(HwndForMenuCommand,1);
		}
		else if(strcmp(str, "recording")==0){
			if(GetMode_ui()==1)
			 changemode(HwndForMenuCommand,0);
		}
}

int setting_func_rec_ui(char *str)
{
		
		if(strcmp(str, "on")==0){
			if((FlagForUI_ca.video_rec_state_ui ==1)||(FlagForUI_ca.sdcard_ui = 0))
				return 0;
			else
				{
					startrec(HwndForMenuCommand);
					return 1;
				}
		}
		else if(strcmp(str, "off")==0){
			if((FlagForUI_ca.video_rec_state_ui ==0)||(FlagForUI_ca.sdcard_ui = 0))
				return 0;
			else
				{
					stoprec(HwndForMenuCommand);
					return 1;
				}
		}
}
int setting_func_collision_ui(char *str)
{
		
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "close")==0){
						parameter_save_collision_level(COLLI_CLOSE);
					}
					else if(strcmp(str, "low")==0){
						parameter_save_collision_level(COLLI_LEVEL_L);
					}
					else if(strcmp(str, "mid")==0){
						parameter_save_collision_level(COLLI_LEVEL_M);
					}
					else if(strcmp(str, "high")==0){
						parameter_save_collision_level(COLLI_LEVEL_H);
					}
					startrec(HwndForMenuCommand);
			}
		else
			{
					if(strcmp(str, "close")==0){
						parameter_save_collision_level(COLLI_CLOSE);
					}
					else if(strcmp(str, "low")==0){
						parameter_save_collision_level(COLLI_LEVEL_L);
					}
					else if(strcmp(str, "mid")==0){
						parameter_save_collision_level(COLLI_LEVEL_M);
					}
					else if(strcmp(str, "high")==0){
						parameter_save_collision_level(COLLI_LEVEL_H);
					}
			
			}
}
int setting_func_leavecarrec_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_leavecarrec(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_leavecarrec(1);
					}
					startrec(HwndForMenuCommand);
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_leavecarrec(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_leavecarrec(1);
					}
			}
}
//debug
//reboot
int setting_func_reboot_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_debug_reboot(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_reboot(1);
					}
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_debug_reboot(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_reboot(1);
					}
			}
}
int GetRebootParameter(void)
{
	return parameter_get_debug_reboot();
}
void SaveRebootParameter(int param)
{
	parameter_save_debug_reboot(param);
}
//end reboot
//recover
int setting_func_recover_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_debug_recovery(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_recovery(1);
					}
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_debug_recovery(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_recovery(1);
					}
			}
}
int GetRecoverParameter(void)
{
	return parameter_get_debug_recovery();
}
void SaveRecoverParameter(int param)
{
	parameter_save_debug_recovery(param);
}
//end recover
//awake 1level
int setting_func_awake_1_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_debug_awake(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_awake(1);
					}
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_debug_awake(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_awake(1);
					}
			}
}
int GetAwake1Parameter(void)
{
	return parameter_get_debug_awake();
}
void SaveAwake1Parameter(int param)
{
	parameter_save_debug_awake(param);
}
//end awake 1level
//standby
int setting_func_standby_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_debug_standby(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_standby(1);
					}
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_debug_standby(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_standby(1);
					}
			}
}
int GetStandbyParameter(void)
{
	return parameter_get_debug_standby();
}
void SaveStandbyParameter(int param)
{
	parameter_save_debug_standby(param);
}
//end standby
//mode change
int setting_func_modechange_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_debug_modechange(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_modechange(1);
					}
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_debug_modechange(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_modechange(1);
					}
			}
}
int GetModechangeParameter(void)
{
	return parameter_get_debug_modechange();
}
void SaveModechangeParameter(int param)
{
	parameter_save_debug_modechange(param);
}

//end modechange

//video
int setting_func_video_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_debug_video(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_video(1);
					}
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_debug_video(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_video(1);
					}
			}
}
int GetVideoParameter(void)
{
	return parameter_get_debug_video();
}
void SaveVideoParameter(int param)
{
	parameter_save_debug_video(param);
}
//end video
//begin or end video
int setting_func_begin_end_video_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_debug_beg_end_video(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_beg_end_video(1);
					}
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_debug_beg_end_video(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_beg_end_video(1);
					}
			}
}
int GetBegEndVideoParameter(void)
{
	return parameter_get_debug_beg_end_video();
}
void SaveBegEndVideoParameter(int param)
{
	parameter_save_debug_beg_end_video(param);
}
//end begin or end video
//photo
int setting_func_photo_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_debug_photo(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_photo(1);
					}
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_debug_photo(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_photo(1);
					}
			}
}
int GetPhotoParameter(void)
{
	return parameter_get_debug_photo();
}
void SavePhotoParameter(int param)
{
	parameter_save_debug_photo(param);
}
//end photo
//tempture
int setting_func_temp_ui(char *str)
{
		if(FlagForUI_ca.video_rec_state_ui ==1)
				{
					stoprec(HwndForMenuCommand);
					if(strcmp(str, "off")==0){
						 parameter_save_debug_temp(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_temp(1);
					}
			}
		else
			{
					if(strcmp(str, "off")==0){
						 parameter_save_debug_temp(0);
					}
					else if(strcmp(str, "on")==0){
						 parameter_save_debug_temp(1);
					}
			}
}
int GetTempParameter(void)
{
	return parameter_get_debug_temp();
}
void SaveTempParameter(int param)
{
	parameter_save_debug_temp(param);
}
//end tempeture
//video bit rate
int setting_func_videobitrate_ui(char *str)
{
	if(strcmp(str, "1")==0){
	     cmd_IDM_DEBUG_VIDEO_BIT_RATE(HwndForMenuCommand,1);
	}
	else if(strcmp(str, "2")==0){
		 cmd_IDM_DEBUG_VIDEO_BIT_RATE(HwndForMenuCommand,2);
	}
	else if(strcmp(str, "4")==0){
		 cmd_IDM_DEBUG_VIDEO_BIT_RATE(HwndForMenuCommand,3);
	}
	else if(strcmp(str, "6")==0){
		 cmd_IDM_DEBUG_VIDEO_BIT_RATE(HwndForMenuCommand,4);
	}
	else if(strcmp(str, "8")==0){
		 cmd_IDM_DEBUG_VIDEO_BIT_RATE(HwndForMenuCommand,5);
	}
	else if(strcmp(str, "10")==0){
		cmd_IDM_DEBUG_VIDEO_BIT_RATE(HwndForMenuCommand,6);
	}
	else if(strcmp(str, "12")==0){
		 cmd_IDM_DEBUG_VIDEO_BIT_RATE(HwndForMenuCommand,7);
	}
}
int GetVideobitrateParameter(void)
{
	return parameter_get_bit_rate_per_pixel();
}
void SaveVideobitrateParameter(int param)//param = 1-7
{
	
	parameter_save_bit_rate_per_pixel(param);
}
//end video bit rate

#else
int setting_func_cammp_ui(char *str)
{
	
}

int setting_func_video_length_ui(char * str)
{
	
}

int setting_func_3dnr_ui(char * str)
{
	
}
int setting_func_ldw_ui(char * str)
{
	
}
int setting_func_white_balance_ui(char *str)
{
}
int setting_func_exposure_ui(char *str)
{
}
int setting_func_motion_detection_ui(char *str)
{
}
int setting_func_data_stamp_ui(char *str)
{
}
int setting_func_record_audio_ui(char *str)
{
}
int setting_func_language_ui(char *str)
{
}
int setting_func_backlight_lcd_ui(char *str)
{
}
int setting_func_USBMODE_ui(char *str)
{
}
int setting_func_format_ui(void (*p)(void))
{
}
int setting_func_setdata_ui(char *str)
{
}
int setting_func_recovery_ui()
{
}
int setting_func_rec_ui(char *str)
{
}
int getfontcameranum(void)
{
}
struct ui_frame *getfontcamera(int n)
{
}
int getbackcameranum(void)
{
}
struct ui_frame *getbackcamera(int n)
{
}
int GetformatStatus(void)
{
}
int GetMode_ui(void)
{
}
int GetSrcState(void)
{
}
#endif
