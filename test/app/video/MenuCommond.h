#ifndef __MENUCOMMOND_H__
#define __MENUCOMMOND_H__

void GetMainWindowHwnd(HWND hWnd);
struct ui_frame *getfontcamera(int n);
struct ui_frame *getbackcamera(int n);
void GetMainWindowHwnd(HWND hWnd);
int setting_func_recovery_ui();
int setting_func_USBMODE_ui(char *str);
int setting_func_phtomp_ui(char *str);
int setting_func_video_length_ui(char * str);
int setting_func_3dnr_ui(char * str);
int setting_func_format_ui(void (*p)(void));
int setting_func_ldw_ui(char * str);
int setting_func_white_balance_ui(char *str);
int setting_func_exposure_ui(char *str);
int setting_func_motion_detection_ui(char *str);
int setting_func_data_stamp_ui(char *str);
int setting_func_record_audio_ui(char *str);
int setting_func_language_ui(char *str);
int setting_func_backlight_lcd_ui(char *str);
int setting_func_cammp_ui(char *str);
int setting_func_setdata_ui(char *str);
int setting_func_frequency_ui(char *str);
int setting_func_autorecord_ui(char *str);
int setting_func_car_ui(char *str);
int setting_func_modechage_ui(char *str);
int setting_func_rec_ui(char *str);
int setting_func_collision_ui(char *str);
int setting_func_leavecarrec_ui(char *str);
//usb
int setting_func_reboot_ui(char *str);
int GetRebootParameter(void);
void SaveRebootParameter(int param);
//end reboot
//recover
int setting_func_recover_ui(char *str);
int GetRecoverParameter(void);
void SaveRecoverParameter(int param);
//end recover
//awake 1level
int setting_func_awake_1_ui(char *str);
int GetAwake1Parameter(void);

void SaveAwake1Parameter(int param);
//end awake 1level
//standby
int setting_func_standby_ui(char *str);
int GetStandbyParameter(void);
void SaveStandbyParameter(int param);
//end standby
//mode change
int setting_func_modechange_ui(char *str);
int GetModechangeParameter(void);
void SaveModechangeParameter(int param);
//end modechange
//video
int setting_func_video_ui(char *str);
int GetVideoParameter(void);
void SaveVideoParameter(int param);
//end video
//begin or end video
int setting_func_begin_end_video_ui(char *str);
int GetBegEndVideoParameter(void);
void SaveBegEndVideoParameter(int param);
//end begin or end video
//photo
int setting_func_photo_ui(char *str);
int GetPhotoParameter(void);
void SavePhotoParameter(int param);
//end photo
//tempture
int setting_func_temp_ui(char *str);
int GetTempParameter(void);
void SaveTempParameter(int param);
//end tempeture
//video bit rate
int setting_func_videobitrate_ui(char *str);
int GetVideobitrateParameter(void);
void SaveVideobitrateParameter(int param);
//end video bit rate

#endif

