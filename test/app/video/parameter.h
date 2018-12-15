#ifndef __PARAMETER_H__
#define __PARAMETER_H__

int parameter_init(void);
int parameter_sav_staewifiinfo(char* ssid, char* password);
int parameter_getwifistainfo(char* ssid, char* password);
int parameter_savewifipass(char* password);
int parameter_getwifiinfo(char* ssid, char* password);
int parameter_sav_wifi_mode(char mod);
char parameter_get_wifi_mode(void);
int parameter_recover(void);
int parameter_save_wb(char wb);
char parameter_get_wb(void);
int parameter_save_ex(char ex);
char parameter_get_ex(void);
int parameter_save_fcamresolution(char resolution);
char parameter_get_fcamresolution(void);
int parameter_save_bcamresolution(char resolution);
char parameter_get_bcamresolution(void);
int parameter_save_recodetime(short time);
short parameter_get_recodetime(void);
int parameter_save_abmode(char mode);
char parameter_get_abmode(void);
int parameter_save_movedete(char en);
char parameter_get_movedete(void);
int parameter_save_timemark(char en);
char parameter_get_timemark(void);
int parameter_save_voicerec(char en);
char parameter_get_voicerec();
int parameter_save();
char parameter_get_video_mark(void);
int parameter_save_video_mark(char resolution);
char parameter_get_video_audio(void);
int parameter_save_video_3dnr(char resolution);
char parameter_get_video_3dnr(void);
int parameter_save_video_ldw(char resolution);
char parameter_get_video_ldw(void);
int parameter_save_video_de(char resolution);
char parameter_get_video_de(void);
int parameter_save_video_backlt(char resolution);
char parameter_get_video_backlt(void);
int parameter_save_video_usb(char resolution);
char parameter_get_video_usb(void);
char* parameter_get_firmware_version();
char parameter_get_video_fre(void);

int parameter_save_video_fontcamera(char resolution);
char parameter_get_video_fontcamera(void);
int parameter_save_video_backcamera(char resolution);
char parameter_get_video_backcamera(void);

int parameter_save_vcamresolution_photo(char resolution);
char parameter_get_vcamresolution_photo(void);
int parameter_save_debug_reboot(char resolution);
char parameter_get_debug_reboot(void);
int parameter_save_debug_recovery(char resolution);
char parameter_get_debug_recovery(void);
int parameter_save_debug_awake(char resolution);
char parameter_get_debug_awake(void);
int parameter_save_debug_standby(char resolution);
char parameter_get_debug_standby(void);
int parameter_save_debug_modechange(char resolution);
char parameter_get_debug_modechange(void);
int parameter_save_debug_video(char resolution);
char parameter_get_debug_video(void);
int parameter_save_debug_beg_end_video(char resolution);
char parameter_get_debug_beg_end_video(void);
int parameter_save_debug_photo(char resolution);
char parameter_get_debug_photo(void);
char parameter_get_debug_temp(void);
int parameter_save_debug_temp(char resolution);
int parameter_save_video_backcamera_width(short width);
short parameter_get_video_backcamera_width(void);
int parameter_save_video_backcamera_height(short height);
short parameter_get_video_backcamera_height(void);
int parameter_save_video_backcamera_fps(char fps);
char parameter_get_video_backcamera_fps(void);
int parameter_save_video_frontcamera_width(short width);
short parameter_get_video_frontcamera_width(void);
int parameter_save_video_frontcamera_height(short height);
short parameter_get_video_frontcamera_height(void);
int parameter_save_video_frontcamera_fps(char fps);
char parameter_get_video_frontcamera_fps(void);
int parameter_save_video_frontcamera_all(char resolution,
                                         short width,
                                         short height,
                                         char fps);
int parameter_save_video_backcamera_all(char resolution,
                                        short width,
                                        short height,
                                        char fps);
int parameter_save_cif_inputid(short cif_inputid);
short parameter_get_cif_inputid(void);
int parameter_save_video_autorec(char resolution);
char parameter_get_video_autorec(void);
int parameter_save_screenoff_time(short screenoff_time);
short parameter_get_screenoff_time(void);
int parameter_save_bit_rate_per_pixel(unsigned int val);
unsigned int parameter_get_bit_rate_per_pixel(void);
int parameter_save_wifi_en(char val);
char parameter_get_wifi_en(void);
int parameter_save_video_idc(char idc);
char parameter_get_video_idc(void);
#endif
