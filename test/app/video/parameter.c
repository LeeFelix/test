#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include "common.h"

#define PARA_VERSION "1.0.4"

#define FIRMWARE_VERSION "SDK-V0.1.0 2016-9-7"
struct _SAVE {
  char pararater_version[12];
  char firmware_version[30];
  char WIFI_SSID[33];
  char WIFI_PASS[65];
  char STA_WIFI_SSID[33];
  char STA_WIFI_PASS[65];
  char wifi_mode;
  char wifi_en;
  char vcam_resolution;
  char ccam_resolution;
  short recodetime;
  char abmode;
  char wb;
  char ex;
  char movedete;
  char timemark;
  char voicerec;
  // new add by lqw
  char video_de;
  char video_mark;
  char video_audioenable;
  char video_autorec;
  char video_lan;
  char video_fre;
  char video_3dnr;
  char video_ldw;
  char video_backlt;
  char video_usb;
  char video_fontcamera;
  char video_backcamera;
  char vcam_resolution_photo;
  char Debug_reboot;
  char Debug_recovery;
  char Debug_awake;
  char Debug_standby;
  char Debug_mode_change;
  char Debug_video;
  char Debug_beg_end_video;
  char Debug_photo;
  char Debug_temp;
  short back_width;
  short back_height;
  char back_fps;
  short front_width;
  short front_height;
  char front_fps;
  short cif_inputid;
  char colli_level;
  char video_leavecarrec;
  short screenoff_time;
  unsigned int bit_rate_per_pixel;
  char video_idc;
};
struct _SAVE parameter;

// vendor parameter
#define VENDOR_REQ_TAG 0x56524551
#define VENDOR_READ_IO _IOW('v', 0x01, unsigned int)
#define VENDOR_WRITE_IO _IOW('v', 0x02, unsigned int)

#define VENDOR_SN_ID 1
#define VENDOR_WIFI_MAC_ID 2
#define VENDOR_LAN_MAC_ID 3
#define VENDOR_BLUETOOTH_ID 4

#define VENDOR_DATA_SIZE (3 * 1024)  // 3k
#define VERDOR_DEVICE "/dev/vendor_storage"

typedef struct _RK_VERDOR_REQ {
  uint32_t tag;
  uint16_t id;
  uint16_t len;
  uint8_t data[VENDOR_DATA_SIZE];
} RK_VERDOR_REQ;

void rknand_print_hex_data(uint8_t* s, uint32_t* buf, uint32_t len) {
  uint32_t i, j, count;
  RK_VERDOR_REQ* req = (RK_VERDOR_REQ*)buf;

  printf("%s\n", s);
  printf("tag:%x\n", req->tag);
  printf("id:%d\n", req->id);
  printf("len:%d\n", req->len);
  printf("data:");
  for (i = 0; i < req->len; i++)
    printf("%2x ", req->data[i]);

  printf("\n");
}

int parameter_vendor_read(int buffer_size, uint8_t* buffer) {
  int ret;
  RK_VERDOR_REQ req;

  int sys_fd = open(VERDOR_DEVICE, O_RDWR, 0);
  if (sys_fd < 0) {
    printf("vendor_storage open fail\n");
    return -2;
  }

  req.tag = VENDOR_REQ_TAG;
  req.id = VENDOR_SN_ID;
  req.len = buffer_size > VENDOR_DATA_SIZE
                ? VENDOR_DATA_SIZE
                : buffer_size; /* max read length to read*/

  ret = ioctl(sys_fd, VENDOR_READ_IO, &req);
  /* return req->len is the real data length stored in the NV-storage */
  if (ret) {
    //	printf("vendor read error ret = %d\n", ret);
    close(sys_fd);
    return -1;
  }
  memcpy(buffer, req.data, req.len);
  // rknand_print_hex_data("vendor read:", (uint32_t *)(& req), req.len + 8);
  close(sys_fd);

  return 0;
}

int parameter_vendor_write(int buffer_size, uint8_t* buffer) {
  int ret;
  RK_VERDOR_REQ req;

  int sys_fd = open(VERDOR_DEVICE, O_RDWR, 0);
  if (sys_fd < 0) {
    printf("vendor_storage open fail\n");
    return -1;
  }

  req.tag = VENDOR_REQ_TAG;
  req.id = VENDOR_SN_ID;
  req.len = buffer_size > VENDOR_DATA_SIZE ? VENDOR_DATA_SIZE : buffer_size;
  memcpy(req.data, buffer, req.len);
  ret = ioctl(sys_fd, VENDOR_WRITE_IO, &req);
  if (ret) {
    printf("vendor write error\n");
    close(sys_fd);
    return -1;
  }
  // rknand_print_hex_data("vendor write:", (uint32_t *)(& req), req.len + 8);
  close(sys_fd);

  return 0;
}

int parameter_read_test(void) {
  int ret;
  struct _SAVE myparameter;

  ret = parameter_vendor_read(sizeof(struct _SAVE), (uint8_t*)&myparameter);

  printf("parameter_read_test\n");
  printf("pararater_version = %s\n", parameter.pararater_version);
  printf("WIFI_SSID = %s\n", parameter.WIFI_SSID);
  printf("WIFI_PASS = %s\n", parameter.WIFI_PASS);
  printf("parameter.video_fre = %d\n", parameter.video_fre);
  printf("parameter.recodetime = %d\n", parameter.recodetime);
  printf("parameter.video_audioenable = %d\n", parameter.video_audioenable);

  return 0;
}

int parameter_save() {
  int ret;

  ret = parameter_vendor_write(sizeof(struct _SAVE), (uint8_t*)&parameter);
  if (ret < 0) {
    printf("vendor_storage_read_test fail\n");
    return -1;
  }

  return 0;
}

int parameter_sav_wifi_mode(char mod) {
  parameter.wifi_mode = mod;
  return parameter_save();
}
char parameter_get_wifi_mode(void) {
  return parameter.wifi_mode;
}

int parameter_savewifipass(char* password) {
  memset(parameter.WIFI_PASS, 0, sizeof(parameter.WIFI_PASS));
  memcpy(parameter.WIFI_PASS, password, strlen(password));

  return parameter_save();
}

int parameter_sav_staewifiinfo(char* ssid, char* password) {
  memset(parameter.STA_WIFI_SSID, 0, sizeof(parameter.STA_WIFI_SSID));
  memcpy(parameter.STA_WIFI_SSID, ssid, strlen(ssid));

  memset(parameter.STA_WIFI_PASS, 0, sizeof(parameter.STA_WIFI_PASS));
  memcpy(parameter.STA_WIFI_PASS, password, strlen(password));

  return parameter_save();
}

int parameter_getwifiinfo(char* ssid, char* password) {
  memcpy(ssid, parameter.WIFI_SSID, sizeof(parameter.WIFI_SSID));
  memcpy(password, parameter.WIFI_PASS, sizeof(parameter.WIFI_PASS));

  return 0;
}

int parameter_getwifistainfo(char* ssid, char* password) {
  memcpy(ssid, parameter.STA_WIFI_SSID, sizeof(parameter.STA_WIFI_SSID));
  memcpy(password, parameter.STA_WIFI_PASS, sizeof(parameter.STA_WIFI_PASS));

  return 0;
}

int parameter_save_wb(char wb) {
  parameter.wb = wb;
  return parameter_save();
}

char parameter_get_wb(void) {
  return parameter.wb;
}

int parameter_save_ex(char ex) {
  parameter.ex = ex;
  return parameter_save();
}

char parameter_get_ex(void) {
  return parameter.ex;
}

int parameter_save_vcamresolution(char resolution) {
  parameter.vcam_resolution = resolution;
  return parameter_save();
}

char parameter_get_vcamresolution(void) {
  return parameter.vcam_resolution;
}
int parameter_save_vcamresolution_photo(char resolution) {
  parameter.vcam_resolution_photo = resolution;
  return parameter_save();
}

char parameter_get_vcamresolution_photo(void) {
  return parameter.vcam_resolution_photo;
}

int parameter_save_video_idc(char idc) {
  parameter.video_idc = idc;
  return parameter_save();
}

char parameter_get_video_idc(void) {
  return parameter.video_idc;
}

//--------------------------------------------------
int parameter_save_video_3dnr(char resolution) {
  parameter.video_3dnr = resolution;
  return parameter_save();
}

char parameter_get_video_3dnr(void) {
  return parameter.video_3dnr;
}

int parameter_save_video_frontcamera_all(char resolution,
                                         short width,
                                         short height,
                                         char fps) {
  parameter.video_fontcamera = resolution;
  parameter.front_width = width;
  parameter.front_height = height;
  parameter.front_fps = fps;
  return parameter_save();
}

int parameter_save_video_fontcamera(char resolution) {
  parameter.video_fontcamera = resolution;
  return parameter_save();
}

char parameter_get_video_fontcamera(void) {
  return parameter.video_fontcamera;
}

int parameter_save_video_frontcamera_width(short width) {
  parameter.front_width = width;
  return parameter_save();
}

short parameter_get_video_frontcamera_width(void) {
  return parameter.front_width;
}

int parameter_save_video_frontcamera_height(short height) {
  parameter.front_height = height;
  return parameter_save();
}

short parameter_get_video_frontcamera_height(void) {
  return parameter.front_height;
}

int parameter_save_video_frontcamera_fps(char fps) {
  parameter.front_fps = fps;
  return parameter_save();
}

char parameter_get_video_frontcamera_fps(void) {
  return parameter.front_fps;
}

int parameter_save_video_backcamera_all(char resolution,
                                        short width,
                                        short height,
                                        char fps) {
  parameter.video_backcamera = resolution;
  parameter.back_width = width;
  parameter.back_height = height;
  parameter.back_fps = fps;
  return parameter_save();
}

int parameter_save_video_backcamera(char resolution) {
  parameter.video_backcamera = resolution;
  return parameter_save();
}

char parameter_get_video_backcamera(void) {
  return parameter.video_backcamera;
}

int parameter_save_video_backcamera_width(short width) {
  parameter.back_width = width;
  return parameter_save();
}

short parameter_get_video_backcamera_width(void) {
  return parameter.back_width;
}

int parameter_save_video_backcamera_height(short height) {
  parameter.back_height = height;
  return parameter_save();
}

short parameter_get_video_backcamera_height(void) {
  return parameter.back_height;
}

int parameter_save_video_backcamera_fps(char fps) {
  parameter.back_fps = fps;
  return parameter_save();
}

char parameter_get_video_backcamera_fps(void) {
  return parameter.back_fps;
}

int parameter_save_video_ldw(char resolution) {
  parameter.video_ldw = resolution;
  return parameter_save();
}

char parameter_get_video_ldw(void) {
  return parameter.video_ldw;
}

int parameter_save_video_de(char resolution) {
  parameter.video_de = resolution;
  return parameter_save();
}

char parameter_get_video_de(void) {
  return parameter.video_de;
}
int parameter_save_video_mark(char resolution) {
  parameter.video_mark = resolution;
  return parameter_save();
}

char parameter_get_video_mark(void) {
  return parameter.video_mark;
}
int parameter_save_video_audio(char resolution) {
  parameter.video_audioenable = resolution;
  return parameter_save();
}

char parameter_get_video_audio(void) {
  return parameter.video_audioenable;
}
int parameter_save_video_autorec(char resolution) {
  parameter.video_autorec = resolution;
  return parameter_save();
}

char parameter_get_video_autorec(void) {
  return parameter.video_autorec;
}
int parameter_save_video_lan(char resolution) {
  parameter.video_lan = resolution;
  return parameter_save();
}

char parameter_get_video_lan(void) {
  return parameter.video_lan;
}
int parameter_save_video_usb(char resolution) {
  parameter.video_usb = resolution;
  return parameter_save();
}

char parameter_get_video_usb(void) {
  return parameter.video_usb;
}

int parameter_save_video_fre(char resolution) {
  parameter.video_fre = resolution;
  return parameter_save();
}

char parameter_get_video_fre(void) {
  return parameter.video_fre;
}

int parameter_save_video_backlt(char resolution) {
  parameter.video_backlt = resolution;
  return parameter_save();
}

char parameter_get_video_backlt(void) {
  return parameter.video_backlt;
}

int parameter_save_collision_level(char resolution) {
  parameter.colli_level = resolution;
  return parameter_save();
}

char parameter_get_collision_level(void) {
  return parameter.colli_level;
}

int parameter_save_leavecarrec(char resolution) {
  parameter.video_leavecarrec = resolution;
  return parameter_save();
}

char parameter_get_leavecarrec(void) {
  return parameter.video_leavecarrec;
}

//------------------------------------------------------
int parameter_save_ccamresolution(char resolution) {
  parameter.ccam_resolution = resolution;
  return parameter_save();
}

char parameter_get_ccamresolution(void) {
  return parameter.ccam_resolution;
}

int parameter_save_recodetime(short time) {
  parameter.recodetime = time;
  return parameter_save();
}

short parameter_get_recodetime(void) {
  return parameter.recodetime;
}

int parameter_save_cif_inputid(short cif_inputid) {
  parameter.cif_inputid = cif_inputid;
  return parameter_save();
}

short parameter_get_cif_inputid(void) {
  return parameter.cif_inputid;
}

int parameter_save_abmode(char mode) {
  parameter.abmode = mode;
  return parameter_save();
}

char parameter_get_abmode(void) {
  return parameter.abmode;
}

int parameter_save_movedete(char en) {
  parameter.movedete = en;
  return parameter_save();
}

char parameter_get_movedete(void) {
  return parameter.movedete;
}

int parameter_save_timemark(char en) {
  parameter.timemark = en;
  return parameter_save();
}

char parameter_get_timemark(void) {
  return parameter.timemark;
}

int parameter_save_voicerec(char en) {
  parameter.voicerec = en;
  return parameter_save();
}

char parameter_get_voicerec() {
  return parameter.voicerec;
}

char* parameter_get_firmware_version() {
  return parameter.firmware_version;
}
// debug---------------------------
int parameter_save_debug_reboot(char resolution) {
  parameter.Debug_reboot = resolution;
  return parameter_save();
}

char parameter_get_debug_reboot(void) {
  return parameter.Debug_reboot;
}
int parameter_save_debug_recovery(char resolution) {
  parameter.Debug_recovery = resolution;
  return parameter_save();
}

char parameter_get_debug_recovery(void) {
  return parameter.Debug_recovery;
}
int parameter_save_debug_awake(char resolution) {
  parameter.Debug_awake = resolution;
  return parameter_save();
}

char parameter_get_debug_awake(void) {
  return parameter.Debug_awake;
}

int parameter_save_debug_standby(char resolution) {
  parameter.Debug_standby = resolution;
  return parameter_save();
}

char parameter_get_debug_standby(void) {
  return parameter.Debug_standby;
}
int parameter_save_debug_modechange(char resolution) {
  parameter.Debug_mode_change = resolution;
  return parameter_save();
}

char parameter_get_debug_modechange(void) {
  return parameter.Debug_mode_change;
}
int parameter_save_debug_video(char resolution) {
  parameter.Debug_video = resolution;
  return parameter_save();
}

char parameter_get_debug_video(void) {
  return parameter.Debug_video;
}
int parameter_save_debug_beg_end_video(char resolution) {
  parameter.Debug_beg_end_video = resolution;
  return parameter_save();
}

char parameter_get_debug_beg_end_video(void) {
  return parameter.Debug_beg_end_video;
}
int parameter_save_debug_photo(char resolution) {
  parameter.Debug_photo = resolution;
  return parameter_save();
}

char parameter_get_debug_photo(void) {
  return parameter.Debug_photo;
}

int parameter_save_debug_temp(char resolution) {
  parameter.Debug_temp = resolution;
  return parameter_save();
}

char parameter_get_debug_temp(void) {
  return parameter.Debug_temp;
}

int parameter_save_screenoff_time(short screenoff_time) {
  parameter.screenoff_time = screenoff_time;
  return parameter_save();
}

short parameter_get_screenoff_time(void) {
  return parameter.screenoff_time;
}

int parameter_save_bit_rate_per_pixel(unsigned int val) {
  parameter.bit_rate_per_pixel = val;
  return parameter_save();
}

unsigned int parameter_get_bit_rate_per_pixel(void) {
  return parameter.bit_rate_per_pixel;
}

int parameter_save_wifi_en(char val) {
  parameter.wifi_en = val;
  return parameter_save();
}

char parameter_get_wifi_en(void) {
  return parameter.wifi_en;
}


//--------------------------------------
int parameter_recover(void) {
  printf("%s\n", __func__);

  int randnum;
  struct timeval tpstart;
  gettimeofday(&tpstart, NULL);
  srand(tpstart.tv_usec);
  randnum = (rand() + 0x1000) % 0x10000;
  memset((void*)&parameter, 0, sizeof(struct _SAVE));

  sprintf(parameter.pararater_version, "%s", PARA_VERSION);
  sprintf(parameter.WIFI_SSID, "RK_CVR_%X", randnum);
  sprintf(parameter.WIFI_PASS, "%s", "123456789");
  parameter.video_fre = CAMARE_FREQ_50HZ;
  parameter.recodetime = 60;  // 60sec
  parameter.video_audioenable = 0;
  parameter.wifi_mode = 0;
  parameter.video_3dnr = 1;
  parameter.video_ldw = 0;
  parameter.video_backlt = LCD_BACKLT_M;
  parameter.vcam_resolution = 1;
  parameter.back_width = 640;
  parameter.back_height = 480;
  parameter.back_fps = 30;
  parameter.front_width = 1920;
  parameter.front_height = 1080;
  parameter.front_fps = 30;
  parameter.abmode = 2;
  parameter.vcam_resolution_photo = 1;
  parameter.screenoff_time = -1;
  parameter.wifi_en = 0;
  parameter.ex = 3;
  parameter.video_autorec = 1;
  parameter.video_mark = 1;
  parameter.video_idc = 1;
  sprintf(parameter.firmware_version, "%s", FIRMWARE_VERSION);
#define VIDEO_RATE_PER_PIXEL 8
  parameter.bit_rate_per_pixel = VIDEO_RATE_PER_PIXEL;
  return parameter_save();
}

int parameter_init(void) {
  int ret;

  ret = parameter_vendor_read(sizeof(struct _SAVE), (uint8_t*)&parameter);
  if (ret == -1)
    return parameter_recover();
  if (ret == -2)
    return -1;

  return 0;
}
