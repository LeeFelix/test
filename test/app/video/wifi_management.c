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
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include "parameter.h"
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include "common.h"

#include "wlan_service_client.h"

#if 0
#include "fs_fileopera.h"
#else
#define BLOCK_FILE_TAG "DCIM"
#endif

#define RTP_TS_TRANS_ENABLE
#define MAX_CLIENT_LIMIT_NUM   15

static int broadcast_fd= -1;
static int tcp_server_fd= -1;
static int tcp_server_tab[MAX_CLIENT_LIMIT_NUM];

#define UDPPORT 18889
#define TCPPORT 8888

char SSID[33];
char PASSWD[65];

#define CMD_GETWIFI 1
#define STR_GETWIFI "CMD_GETWIFI"
#define CMD_SETWIFI 2
#define STR_SETWIFI "CMD_SETWIFI"
#define CMD_GETFCAMFILE 3
#define STR_GETFCAMFILE "CMD_GETFCAMFILE"
#define CMD_DELFCAMFILE 4
#define STR_DELFCAMFILE "CMD_DELFCAMFILE"
#define CMD_DOWNLOADFILE 5
#define STR_DOWNLOADFILE "CMD_DOWNLOADFILE"
#define CMD_REQUESTTRANS 6
#define STR_REQUESTTRANS "CMD_REQUESTTRANS"
#define CMD_REQUESTRETRANS 7
#define STR_REQUESTRETRANS "CMD_REQUESTRETRANS"
#define CMD_APPEARSSID 8
#define STR_APPEARSSID "CMD_APPEARSSID"
#define CMD_ARGSETTING 9
#define STR_ARGSETTING "CMD_ARGSETTING"
#define CMD_GET_ARGSETTING 10
#define STR_GET_ARGSETTING "CMD_GET_ARGSETTING"
#define CMD_GET_FONT_CAMERARESPLUTION  11
#define STR_GET_FONT_CAMERARESPLUTION  "CMD_GET_FONT_CAMERARESPLUTION"
#define CMD_GET_BACK_CAMERARESPLUTION  12
#define STR_GET_BACK_CAMERARESPLUTION  "CMD_GET_BACK_CAMERARESPLUTION"
#define CMD_GET_FORMAT_STATUS  13
#define STR_GET_FORMAT_STATUS  "CMD_GET_FORMAT_STATUS"
#define CMD_Control_Recording  14
#define STR_Control_Recording  "CMD_Control_Recording"
#define CMD_GET_Control_Recording  15
#define STR_GET_Control_Recording  "CMD_GET_Control_Recording"
#define CMD_RTP_TS_TRANS_START     16
#define STR_RTP_TS_TRANS_START     "CMD_RTP_TS_TRANS_START"
#define CMD_RTP_TS_TRANS_STOP      17
#define STR_RTP_TS_TRANS_STOP      "CMD_RTP_TS_TRANS_STOP"

typedef int (*_setting_func_tp)(int nfd, char* str);
#define STR_SETTING_RESUME_ALL "RESUME_ALL"
#define STR_SETTING_Front_camera "Font_camera"
#define STR_SETTING_Back_camera "Back_camera"
#define STR_SETTING_Video_length "Videolength"
#define STR_SETTING_Record_Mode "RecordMode"
#define STR_SETTING_3DNR "3DNR"
#define STR_SETTING_ADAS_LDW "ADAS_LDW"
#define STR_SETTING_WhiteBalance "WhiteBalance"
#define STR_SETTING_Exposure "Exposure"
#define STR_SETTING_MotionDetection "MotionDetection"
#define STR_SETTING_DataStamp "DataStamp"
#define STR_SETTING_RecordAudio "RecordAudio"
#define STR_SETTING_BootRecord "BootRecord"
#define STR_SETTING_Language "Language"
#define STR_SETTING_Frequency "Frequency"
#define STR_SETTING_Bright "Bright"
#define STR_SETTING_USBMODE "USBMODE"
#define STR_SETTING_Format "Format"
#define STR_SETTING_DateSet "DateSet"
#define STR_SETTING_Recovery "Recovery"
#define STR_SETTING_Version "Version"

typedef struct _type_cmd_setting {
  _setting_func_tp func;
  char* cmd;
} type_cmd_setting;

int setting_func_front_camera(int nfd, char* str);
int setting_func_back_camera(int nfd, char* str);
int setting_func_video_length(int nfd, char* str);
int setting_func_record_mode(int nfd, char* str);
int setting_func_3dnr(int nfd, char* str);
int setting_func_adas_ldw(int nfd, char* str);
int setting_func_white_balance(int nfd, char* str);
int setting_func_exposure(int nfd, char* str);
int setting_func_motion_detection(int nfd, char* str);
int setting_func_data_stamp(int nfd, char* str);
int setting_func_record_audio(int nfd, char* str);
int setting_func_boot_record(int nfd, char* str);
int setting_func_language(int nfd, char* str);
int setting_func_frequency(int nfd, char* str);
int setting_func_bright(int nfd, char* str);
int setting_func_USBMODE(int nfd, char* str);
int setting_func_format(int nfd, char* str);
int setting_func_dateset(int nfd, char* str);
int setting_func_recovery(int nfd, char* str);
int setting_func_version(int nfd, char* str);

type_cmd_setting const cmd_setting_tab[] =
{
    {
        setting_func_front_camera, STR_SETTING_Front_camera,
    },
    {
        setting_func_back_camera, STR_SETTING_Back_camera,
    },
    {
        setting_func_video_length, STR_SETTING_Video_length,
    },
    {
        setting_func_record_mode, STR_SETTING_Record_Mode,
    },
    {
        setting_func_3dnr, STR_SETTING_3DNR,
    },
    {
        setting_func_adas_ldw, STR_SETTING_ADAS_LDW,
    },
    {
        setting_func_white_balance, STR_SETTING_WhiteBalance,
    },
    {
        setting_func_exposure, STR_SETTING_Exposure,
    },
    {
        setting_func_motion_detection, STR_SETTING_MotionDetection,
    },
    {
        setting_func_data_stamp, STR_SETTING_DataStamp,
    },
    {
        setting_func_record_audio, STR_SETTING_RecordAudio,
    },
    {
        setting_func_boot_record, STR_SETTING_BootRecord,
    },
    {
        setting_func_language, STR_SETTING_Language,
    },
    {
        setting_func_frequency, STR_SETTING_Frequency,
    },
    {
        setting_func_bright, STR_SETTING_Bright,
    },
    {
        setting_func_USBMODE, STR_SETTING_USBMODE,
    },
    {
        setting_func_format, STR_SETTING_Format,
    },
    {
        setting_func_dateset, STR_SETTING_DateSet,
    },
    {
        setting_func_recovery, STR_SETTING_Recovery,
    }
};

static const char* save_file = "/catch/save.bin";
static const char* vedio_folder = "/mnt/sdcard/DCIM";
static const char* power_config = "/sys/class/rkwifi/driver";

static pthread_t broadcast_tid;
static pthread_t tcp_server_tid;
static int format_finish = 0;
static int g_wifi_is_running = 0;
static int live_client_num = 0;

#if 0
unsigned short int CRC16(unsigned char* Data_Array,
                         unsigned long int DataLength)  //»ñÈ¡CRC
{
  unsigned long int i, j;
  unsigned short int CRCTemp;

  CRCTemp = 0xffff;
  for (i = 0; i < DataLength; i++) {
    CRCTemp = CRCTemp ^ Data_Array[i];
    for (j = 0; j <= 7; j++) {
      if ((CRCTemp & 0x0001) == 0x0001) {
        CRCTemp = CRCTemp >> 1;
        CRCTemp ^= 0xa001;
      } else
        CRCTemp = CRCTemp >> 1;
    }
  }

  return CRCTemp;
}
#endif

int ResolveDeleteFile(int nfp, char* buffer) {
  char *p, *p1;
  char path[128];

  printf("delete file cmd : %s\n", buffer);

  while ((p = strstr(buffer, "NAME:")) != NULL) {
    if ((p1 = strstr(p, "END")) != NULL) {
      p1[0] = 0;
      buffer = &p1[3];
    } else {
      break;
    }

    strcat(path, vedio_folder);
    strcat(path, "/");
    strcat(path, (char*)&p[5]);
    printf("path:%s\n", path);

    if (remove(path) < 0) {
      printf("delete %s fault\n", path);
      if (0 > write(nfp, "CMD_DELFAULT", sizeof("CMD_DELFAULT"))) {
        printf("write fail!\r\n");
      }
    } else {
      printf("delete %s success\n", path);
      if (0 > write(nfp, "CMD_DELSUCCESS", sizeof("CMD_DELSUCCESS"))) {
        printf("write fail!\r\n");
      }
    }
  }

  return 0;
}

void ResolveGetFileList(int nfp, char* buffer) {
  int i;
  char count[32];
  char wifi_info[1024];
  char ackbuf[256];
  struct tm* time;

  struct dirent* ent = NULL;
  DIR* pDir;
  struct stat fileinfo;
  int startN = -1, countN = -1;

  sscanf(buffer, "CMD_GETFCAMFILESTART:%dCOUNT:%dEND", &startN, &countN);
  if (startN < 0 || countN < 0) {
    printf("ResolveGetFileList cmd error %s\n", buffer);
    return;
  }
  printf("startN:%d %d\n", startN, countN);

  if (access(vedio_folder, 0) < 0) {
    if (mkdir(vedio_folder, 0777)) {
      perror(vedio_folder);
      return;
    }
  }

  pDir = opendir(vedio_folder);
  if (pDir == NULL) {
    perror("opendir\n");
    return;
  }

  for (i = 0; i < startN + 2; i++) {
    readdir(pDir);
  }

  i = startN;
  while (NULL != (ent = readdir(pDir)) && countN > 0) {
    if (ent->d_type == 8) {
      memset(count, 0, sizeof(count));
      memset(wifi_info, 0, sizeof(wifi_info));
      memset(ackbuf, 0, sizeof(ackbuf));
      sprintf(count, "%d", i);

      {
        char filepath[128];
        sprintf(filepath, "%s/%s", vedio_folder, ent->d_name);
        stat(filepath, &fileinfo);
      }

      strcat(wifi_info, "CMD_GETCAMFILE");
      strcat(wifi_info, "NAME:");
      strcat(wifi_info, ent->d_name);

      sprintf(&wifi_info[strlen(wifi_info)], "SIZE:%ld", fileinfo.st_size);
      time = localtime(&fileinfo.st_mtime);
      sprintf(&wifi_info[strlen(wifi_info)], "DAY:%04d-%02d-%02d",
              time->tm_year + 1900, time->tm_mon + 1, time->tm_mday);
      sprintf(&wifi_info[strlen(wifi_info)], "TIME:%02d:%02d:%02d",
              time->tm_hour, time->tm_min, time->tm_sec);

      strcat(wifi_info, "COUNT:");
      strcat(wifi_info, count);
      strcat(wifi_info, "END");
      printf("wifi_info = %s\n", wifi_info);

      if (0 > write(nfp, wifi_info, strlen(wifi_info))) {
        printf("write fail!\r\n");
      }

      if (0 > read(nfp, ackbuf, sizeof(ackbuf))) {
        printf("write fail!\r\n");
      }
      if (strcmp(ackbuf, "CMD_NEXTFILE") < 0) {
        printf("CMD_NEXTFILE fault : %s\n", ackbuf);
        break;
      }

      i++;
      countN--;
    }
  }
}

int getwlan0ip(char* sip) {
  int inet_sock;
  struct ifreq ifr;

  inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
  strcpy(ifr.ifr_name, "wlan0");
  if (ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0) {
    perror("ioctl");
    return -1;
  }
  strcpy(sip, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
  return 0;
}

int getip() {
  int inet_sock;
  struct ifreq ifr;
  FILE* net_devs;
  char buf[1024];
  char interface[32+1];
  int index;

  net_devs = fopen("/proc/net/dev", "r");
  if (net_devs == NULL) {
    perror("Can't open /proc/net/dev for reading");
    exit(1);
  }

  inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

  while (1) {
    if (fscanf(net_devs, "%32s %1024[^\n]", interface, buf) <= 0) {
      break;
    }
    index = 0;

    while (index < 32 && interface[index] != ':') {
      index++;
    }

    if (index >= 32)
      continue;

    interface[index] = '\0';
    strcpy(ifr.ifr_name, interface);

    if (ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0)
      perror("ioctl");
    printf("%s:\t%s\n", interface,
           inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
  }
  fclose (net_devs);
  close(inet_sock);

  return 0;
}

int cmd_resolve(char* cmdstr, char* retdata) {
  char* str;
  int len;
  if (strstr(cmdstr, STR_GETWIFI) != 0)
    return CMD_GETWIFI;
  else if (strstr(cmdstr, STR_SETWIFI) != 0) {
    return CMD_SETWIFI;
  } else if (strstr(cmdstr, STR_GETFCAMFILE) != 0) {
    return CMD_GETFCAMFILE;
  } else if (strstr(cmdstr, STR_DELFCAMFILE) != 0) {
    return CMD_DELFCAMFILE;
  } else if (strstr(cmdstr, STR_DOWNLOADFILE) != 0) {
    return CMD_DOWNLOADFILE;
  } else if (strstr(cmdstr, STR_REQUESTTRANS) != 0) {
    return CMD_REQUESTTRANS;
  } else if (strstr(cmdstr, STR_REQUESTRETRANS) != 0) {
    return CMD_REQUESTRETRANS;
  } else if (strstr(cmdstr, STR_APPEARSSID) != 0) {
    return CMD_APPEARSSID;
  } else if (strstr(cmdstr, STR_ARGSETTING) != 0) {
    return CMD_ARGSETTING;
  } else if (strstr(cmdstr, STR_GET_ARGSETTING) != 0) {
    return CMD_GET_ARGSETTING;
  } else if (strstr(cmdstr, STR_GET_FONT_CAMERARESPLUTION) != 0) {
    return CMD_GET_FONT_CAMERARESPLUTION;
  }else if (strstr(cmdstr, STR_GET_BACK_CAMERARESPLUTION) != 0) {
    return CMD_GET_BACK_CAMERARESPLUTION;
  }else if (strstr(cmdstr, STR_GET_FORMAT_STATUS) != 0) {
    return CMD_GET_FORMAT_STATUS;
  }else if (strstr(cmdstr, STR_Control_Recording) != 0) {
    return CMD_Control_Recording;
  }else if (strstr(cmdstr, STR_GET_Control_Recording) != 0) {
    return CMD_GET_Control_Recording;
  }else if (strstr(cmdstr, STR_RTP_TS_TRANS_START) != 0) {
    return CMD_RTP_TS_TRANS_START;
  }else if (strstr(cmdstr, STR_RTP_TS_TRANS_STOP) != 0) {
    return CMD_RTP_TS_TRANS_STOP;
  }
  return -1;
}

int setting_cmd_resolve(int nfd, char* cmdstr) {
  int i = 0;
  printf("setting_cmd_resolve:%s\n",cmdstr);
  for (i = 0; i < sizeof(cmd_setting_tab) / sizeof(cmd_setting_tab[0]); i++) {
    if (strstr(cmdstr, cmd_setting_tab[i].cmd) != 0) {
      if (cmd_setting_tab[i].func != NULL) {
        return cmd_setting_tab[i].func(nfd,&cmdstr[sizeof(STR_ARGSETTING) - 1]);
      }
    }
  }
  return -1;
}

int setting_get_cmd_resolve(int nfp) {
  char buff[1024];
  int arg;
  time_t t;
  struct tm* tm;
  char tbuf[128];

  memset(buff, 0, sizeof(buff));

  strcat(buff, "CMD_ACK_GET_ARGSETTING");

  strcat(buff, "Videolength:");
  arg = parameter_get_recodetime();
  if (arg == 60) {
    strcat(buff, "1min");
  } else if (arg == 180) {
    strcat(buff, "3min");
  } else if (arg == 300) {
    strcat(buff, "5min");
  }

  strcat(buff, "RecordMode:");
  arg = parameter_get_abmode();
  if (arg == 0) {
    strcat(buff, "Front");
  } else if (arg == 1) {
    strcat(buff, "Rear");
  } else if (arg == 2) {
    strcat(buff, "Double");
  }

  strcat(buff, "3DNR:");
  if (parameter_get_video_3dnr()) {
    strcat(buff, "ON");
  } else {
    strcat(buff, "OFF");
  }

  strcat(buff, "ADAS_LDW:");

  arg = parameter_get_video_ldw();
  if (arg==1) {
    strcat(buff, "ON");
  } else {
    strcat(buff, "OFF");
  }

  strcat(buff, "WhiteBalance:");
  arg = parameter_get_wb();
  if (arg == 0) {
    strcat(buff, "auto");
  } else if (arg == 1) {
    strcat(buff, "Daylight");
  } else if (arg == 2) {
    strcat(buff, "fluocrescence");
  } else if (arg == 3) {
    strcat(buff, "cloudysky");
  } else if (arg == 4) {
    strcat(buff, "tungsten");
  }

  strcat(buff, "Exposure:");
  arg = parameter_get_ex();
  if (arg == 0) {
    strcat(buff, "-3");
  } else if (arg == 1) {
    strcat(buff, "-2");
  } else if (arg == 2) {
    strcat(buff, "-1");
  } else if (arg == 3) {
    strcat(buff, "0");
  } else if (arg == 4) {
    strcat(buff, "1");
  }

  strcat(buff, "MotionDetection:");
  arg = parameter_get_video_de();
  if (arg == 1) {
    strcat(buff, "ON");
  } else if (arg == 0) {
    strcat(buff, "OFF");
  }

  strcat(buff, "DataStamp:");
  arg = parameter_get_video_mark();
  if (arg == 1) {
    strcat(buff, "ON");
  } else if (arg == 0) {
    strcat(buff, "OFF");
  }

  strcat(buff, "RecordAudio:");
  arg = parameter_get_video_audio();
  if (arg == 1) {
    strcat(buff, "ON");
  } else if (arg == 0) {
    strcat(buff, "OFF");
  }

  strcat(buff, "BootRecord:");
  arg = parameter_get_video_autorec();
  if (arg == 1) {
    strcat(buff, "ON");
  } else if (arg == 0) {
    strcat(buff, "OFF");
  }

  strcat(buff, "Language:");
  arg = parameter_get_video_lan();
  if (arg == 0) {
    strcat(buff, "English");
  } else if (arg == 1) {
    strcat(buff, "Chinese");
  }

  strcat(buff, "Frequency:");
  arg = parameter_get_video_fre();
  if (arg == CAMARE_FREQ_50HZ) {
    strcat(buff, "50HZ");
  } else if (arg == CAMARE_FREQ_60HZ) {
    strcat(buff, "60HZ");
  }

  strcat(buff, "Bright:");
  arg = parameter_get_video_backlt();
  if (arg == LCD_BACKLT_L) {
    strcat(buff, "low");
  } else if (arg == LCD_BACKLT_M) {
    strcat(buff, "middle");
  } else if (arg == LCD_BACKLT_H) {
    strcat(buff, "hight");
  }

  strcat(buff, "USBMODE:");
  arg = parameter_get_video_usb();
  if (arg == 0) {
    strcat(buff, "msc");
  } else if (arg == 1) {
    strcat(buff, "adb");
  }

  time(&t);
  tm = localtime(&t);
  sprintf(tbuf,"Time:%04d-%02d-%02d %02d:%02d:%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
  strcat(buff, tbuf);

  strcat(buff, "Version:");
  strcat(buff, parameter_get_firmware_version());

  printf("cmd = %s\n", buff);
  if (0 > write(nfp, buff, strlen(buff))) {
    printf("write fail!\r\n");
  }
}


int setting_func_front_camera(int nfp, char* str)
{
  char* p = &str[sizeof(STR_SETTING_Front_camera)];
  printf("setting_func_front_camera : %s\n", p);
  setting_func_cammp_ui(p);
  return 0;
}

int setting_func_video_length(int nfp, char* str)
{
  char* p = &str[sizeof(STR_SETTING_Video_length)];
  setting_func_video_length_ui(p);
  return 0;
}

int setting_func_record_mode(int nfp, char* str)
{
  char* p = &str[sizeof(STR_SETTING_Record_Mode)];

  printf("setting_func_record_mode : %s\n", str);

  if (strcmp(p, "Front") == 0) {
    printf("SET record_mode Front\n");
    parameter_save_abmode(0);
  } else if (strcmp(p, "Rear") == 0) {
    printf("SET record_mode Rear\n");
    parameter_save_abmode(1);
  } else if (strcmp(p, "Double") == 0) {
    printf("SET record_mode Double\n");
    parameter_save_abmode(2);
  }
  return 0;
}

int setting_func_back_camera(int nfp, char* str)
{
  char* p = &str[sizeof(STR_SETTING_Back_camera)];
  printf("setting_func_back_camera : %s\n", p);
  setting_func_cammp_ui(p);
  return 0;
}

int setting_func_3dnr(int nfp, char* str)
{
  char* p = &str[sizeof(STR_SETTING_3DNR)];
  setting_func_3dnr_ui(p);
  return 0;
}

int setting_func_adas_ldw(int nfp, char* str)
{
  char* p = &str[sizeof(STR_SETTING_ADAS_LDW)];
  setting_func_ldw_ui(p);
  return 0;
}

int setting_func_white_balance(int nfd, char* str)
{
  char* p = &str[sizeof(STR_SETTING_WhiteBalance)];
  setting_func_white_balance_ui(p);
}

int setting_func_exposure(int nfd, char* str)
{
  char* p = &str[sizeof(STR_SETTING_Exposure)];
  setting_func_exposure_ui(p);
}

int setting_func_motion_detection(int nfd, char* str)
{
  char* p = &str[sizeof(STR_SETTING_MotionDetection)];
  setting_func_motion_detection_ui(p);
}

int setting_func_data_stamp(int nfd, char* str)
{
  char* p = &str[sizeof(STR_SETTING_DataStamp)];
  setting_func_data_stamp_ui(p);
}

int setting_func_record_audio(int nfd, char* str)
{
  char* p = &str[sizeof(STR_SETTING_RecordAudio)];
  setting_func_record_audio_ui(p);
}

int setting_func_boot_record(int nfd, char* str) {
  char* p = &str[sizeof(STR_SETTING_BootRecord)];

  printf("setting_func_boot_record : %s\n", str);

  if (strcmp(p, "ON") == 0) {
    printf("SET boot_record ON\n");
    parameter_save_video_autorec(1);
  } else if (strcmp(p, "OFF") == 0) {
    printf("SET boot_record OFF\n");
    parameter_save_video_autorec(0);
  }
}
int setting_func_language(int nfd, char* str)
{
  char* p = &str[sizeof(STR_SETTING_Language)];
  setting_func_language_ui(p);
}
int setting_func_frequency(int nfd, char* str) {
  char* p = &str[sizeof(STR_SETTING_Frequency)];

  printf("setting frequency : %s\n", str);

  if (strcmp(p, "50HZ") == 0) {
    parameter_save_video_fre(CAMARE_FREQ_50HZ);
    video_record_set_power_line_frequency(CAMARE_FREQ_50HZ);
  } else if (strcmp(p, "60HZ") == 0) {
    parameter_save_video_fre(CAMARE_FREQ_60HZ);
    video_record_set_power_line_frequency(CAMARE_FREQ_60HZ);
  }
}
int setting_func_bright(int nfd, char* str)
{
  char* p = &str[sizeof(STR_SETTING_Bright)];
  setting_func_backlight_lcd_ui(p);
}
int setting_func_USBMODE(int nfd, char* str)
{
  char* p = &str[sizeof(STR_SETTING_USBMODE)];
  setting_func_USBMODE_ui(p);
}

void setting_func_format_finish(void)
{
    printf("setting_func_format_finish\n");
    format_finish = 1;
}

int setting_func_format(int nfd, char* str)
{
  printf("setting_func_format\n",str);
  format_finish = 0;
  setting_func_format_ui(&setting_func_format_finish);

  while(1){
    if(format_finish!=0)break;
    sleep(1);
  }
  if (0 > write(nfd, "CMD_GET_ACK_FORMAT_STATUS_FINISH", sizeof("CMD_GET_ACK_FORMAT_STATUS_FINISH")))
  {
    printf("write fail!\r\n");
  }
}

int setting_func_dateset(int nfd, char* str)
{
  char* p = &str[sizeof(STR_SETTING_DateSet)];
  printf("setting_func_dateset:%s\n",p);
  setting_func_setdata_ui(p);
}

int setting_func_recovery(int nfd, char* str)
{
  printf("setting_func_recovery\n");
  setting_func_recovery_ui();
}

void* tcp_receive(void* arg) {
  int sfp, nfp= (int)arg;
  int recbytes;
  char buffer[512] = {0};
  char data[512] = {0};
  int cmd;
  int fd = -1;
  char path[256];
  unsigned short int port = 0;

  pthread_detach(pthread_self());  // unjoinable
  printf("tcp_receive : tcp_receive nfp = 0x%x\n", nfp);
  while (g_wifi_is_running) {
    memset(buffer, 0, sizeof(buffer));
    memset(data, 0, sizeof(data));

    printf("tcp_receive : TCP Recerive:Wait TCP CMD.....\n");
    recbytes = read(nfp, buffer, sizeof(buffer));
    if (0 == recbytes) {
      printf("tcp_receive : disconnect!\r\n");
      goto stop_tcp_receive;
    }
    // printf("read ok\r\nREC:\r\n");
    buffer[recbytes] = '\0';
    cmd = cmd_resolve(buffer, data);
    printf("tcp_receive : TCP Recerive:cmd = %d\n", cmd);
    switch (cmd) {
      case CMD_GETWIFI: {
        char wifi_info[100] = "CMD_WIFI_INFO";
        char ssid[33], passwd[65];
        parameter_getwifiinfo(ssid, passwd);

        strcat(wifi_info, "WIFINAME:");
        strcat(wifi_info, ssid);
        strcat(wifi_info, "WIFIPASSWORD:");
        strcat(wifi_info, passwd);
        strcat(wifi_info, "MODE:");
        if (parameter_get_wifi_mode() == 0) {
          strcat(wifi_info, "AP");
        } else {
          strcat(wifi_info, "STA");
        }
        strcat(wifi_info, "END");
        printf("CMD_GETWIFI name is \"%s\", password is \"%s\"\n", ssid,
               passwd);
        if (0 > write(nfp, wifi_info, strlen(wifi_info))) {
          printf("write fail!\r\n");
        }
        break;
      }
      case CMD_SETWIFI: {
        char *p = NULL, *p1 = NULL;
        char ssid[33], passwd[65];

        memset(ssid, 0, sizeof(ssid));
        memset(passwd, 0, sizeof(passwd));

        printf("CMD_SETWIFI Get Data:%s\n", buffer);

        p1 = buffer;
        if ((p = strstr(buffer, "WIFINAME:")) != NULL) {
          p = &p[sizeof("WIFINAME:") - 1];

          while (strstr(p1, "PASSWD:") != NULL) {
            p1 = strstr(p1, "PASSWD:");
            p1 = &p1[sizeof("PASSWD:") - 1];
          }

          if (p1 != buffer) {
            memcpy(ssid, p, (int)p1 - (int)p - sizeof("PASSWD:") + 1);
            memcpy(passwd, p1, strlen(p1));
          } else {
            printf("without PASSWD:\n");
            break;
          }
        } else {
          printf("without SSID:\n");
          break;
        }
        printf("CMD_SETWIFI SSID = %s PASSWD = %s\n", ssid, passwd);

        parameter_getwifiinfo(SSID, PASSWD);
        if (strcmp(ssid, SSID) != 0) {
          printf("%s != %s\n", ssid, SSID);
          break;
        }

        parameter_savewifipass(passwd);
        // memcpy(PASSWD,passwd,sizeof(PASSWD));

        // write_accesspoint_config_hostapd(ssid, passwd);

        parameter_sav_wifi_mode(0);
        runapp("busybox reboot");

        break;
      }
      case CMD_GETFCAMFILE: {
        ResolveGetFileList(nfp, buffer);
        break;
      }
      case CMD_DELFCAMFILE: {
        ResolveDeleteFile(nfp, buffer);
        break;
      }

      case CMD_APPEARSSID: {
        char *p = NULL, *p1 = NULL;
        char ssid[33], passwd[65];

        memset(ssid, 0, sizeof(ssid));
        memset(passwd, 0, sizeof(passwd));

        printf("CMD_APPEARSSID Get Data:%s\n", buffer);

        p1 = buffer;
        if ((p = strstr(buffer, "SSID:")) != NULL) {
          p = &p[sizeof("SSID:") - 1];

          while (strstr(p1, "PASSWD:") != NULL) {
            p1 = strstr(p1, "PASSWD:");
            p1 = &p1[sizeof("PASSWD:") - 1];
          }

          if (p1 != buffer) {
            memcpy(ssid, p, (int)p1 - (int)p - sizeof("PASSWD:") + 1);
            memcpy(passwd, p1, strlen(p1));
          } else {
            printf("without PASSWD:\n");
            break;
          }
        } else {
          printf("without SSID:\n");
          break;
        }

        printf("CMD_APPEARSSID:\nssid = %s\n passwd = %s\n", ssid, passwd);
        parameter_sav_wifi_mode(1);
        parameter_sav_staewifiinfo(ssid, passwd);

        printf("reboot....\n");
        runapp("busybox reboot");

        break;
      }

      case CMD_ARGSETTING:
        printf("CMD_ARGSETTING\n");
        setting_cmd_resolve(nfp, buffer);
        break;

      case CMD_GET_ARGSETTING:
        printf("CMD_GET_ARGSETTING\n");
        setting_get_cmd_resolve(nfp);
        break;

      case CMD_GET_FONT_CAMERARESPLUTION:
        {
            char msg[128];
            char str[128];
            struct ui_frame * pfrontcamera_menu;
            memset(msg, 0, sizeof(msg));

            int i = getfontcameranum(), j;
            strcat(msg, "CMD_ACK_FONT_CAMERARESPLUTION:");

            for(j= 0;j< i; j++){
                memset(str, 0, sizeof(str));
                pfrontcamera_menu= (struct ui_frame *)getfontcamera(j);
                if(pfrontcamera_menu->width!=0 && pfrontcamera_menu->height!=0){
                    sprintf(str,"%d*%d:",pfrontcamera_menu->width,pfrontcamera_menu->height);
                    strcat(msg,str);
                }
            }

            if (0 > write(nfp, msg, strlen(msg))) {
                printf("write fail!\r\n");
            }
        }
        break;

      case CMD_GET_BACK_CAMERARESPLUTION:
        {
            char msg[128];
            char str[128];
            struct ui_frame * pfrontcamera_menu;
            memset(msg, 0, sizeof(msg));

            int i = getbackcameranum(), j;
            strcat(msg, "CMD_ACK_BACK_CAMERARESPLUTION:");

            for(j= 0;j< i; j++){
                memset(str, 0, sizeof(str));
                pfrontcamera_menu= (struct ui_frame *)getbackcamera(j);
                if(pfrontcamera_menu->width!=0 && pfrontcamera_menu->height!=0){
                    sprintf(str,"%d*%d:",pfrontcamera_menu->width,pfrontcamera_menu->height);
                    strcat(msg,str);
                }
            }

            if (0 > write(nfp, msg, strlen(msg))) {
                printf("write fail!\r\n");
            }
        }
        break;

      case CMD_GET_FORMAT_STATUS:
        {
            if(GetformatStatus()==0){
                if (0 > write(nfp, "CMD_GET_ACK_FORMAT_STATUS_IDLE", sizeof("CMD_GET_ACK_FORMAT_STATUS_IDLE"))) {
                    printf("write fail!\r\n");
                }
            }else{
                if(GetformatStatus()==0){
                    if (0 > write(nfp, "CMD_GET_ACK_FORMAT_STATUS_BUSY", sizeof("CMD_GET_ACK_FORMAT_STATUS_BUSY"))) {
                        printf("write fail!\r\n");
                    }
                }
            }
        }
        break;

	    case CMD_Control_Recording:
	    {
		    char * p = &buffer[sizeof(STR_Control_Recording)];
		    printf("%s %s\n",buffer, p);
		    setting_func_rec_ui(p);
	    }
	    break;

	    case CMD_GET_Control_Recording:
	    printf("CMD_GET_Control_Recording\n");
	
	    if(GetSrcState()==0){
            if (0 > write(nfp, "CMD_ACK_GET_Control_Recording_IDLE", sizeof("CMD_ACK_GET_Control_Recording_IDLE"))) {
                printf("write fail!\r\n");
            }
        }else{
            if (0 > write(nfp, "CMD_ACK_GET_Control_Recording_BUSY", sizeof("CMD_ACK_GET_Control_Recording_BUSY"))) {
                printf("write fail!\r\n");
            }
        }
	    break;

        case CMD_RTP_TS_TRANS_START:
            live_client_num++;
            printf("CMD_RTP_TS_TRANS_START %d\n",live_client_num);
#ifdef RTP_TS_TRANS_ENABLE
            if(live_client_num> 0){
                //ts_test_code("rtp://@239.1.1.1:1234");
                video_record_start_ts_transfer("rtp://@239.1.1.1:1234");
            }
#endif
            break;

        case CMD_RTP_TS_TRANS_STOP:
            live_client_num--;
            printf("CMD_RTP_TS_TRANS_STOP %d\n",live_client_num);
            if(live_client_num<= 0){
                live_client_num = 0;
#ifdef RTP_TS_TRANS_ENABLE
                printf("wifi_management_stop: stop ts transfer...!\n");
                //stop_ts_transfer(1);
                video_record_stop_ts_transfer(1);
#endif
            }
            break;

      default:
        printf("no this cmd read data is \"%s\"\n", buffer);
        break;
    }
  }

stop_tcp_receive:
  printf("tcp_receive : stop tcp_receive\n");

  {
    int i;
    for(i= 0;i< MAX_CLIENT_LIMIT_NUM; i++){
        if(tcp_server_tab[i]== nfp){
            shutdown(nfp, 2);
            tcp_server_tab[i]= -1;
        }
    }
  }

  pthread_exit(NULL);
}

void* tcp_server_thread(void* arg) {
  int nfp;
  struct sockaddr_in s_add, c_add;
  int sin_size;
  pthread_t tcpreceive_tid;
  int i;
  int opt= -1;
  
  printf("Hello,welcome to my server !\r\n");

  pthread_detach(pthread_self());  // unjoinable

  for(i= 0; i< MAX_CLIENT_LIMIT_NUM;i++){
    tcp_server_tab[i]= -1;
  }

  tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (tcp_server_fd < 0) {
    printf("tcp_server_thread : socket fail ! \r\n");
    pthread_exit(NULL);
  }
  if (setsockopt(tcp_server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) <
      0) {
    perror("tcp_server_thread setsockopt SO_REUSEADDR");
    return NULL;
  }
  printf("socket ok !\r\n");

  bzero(&s_add, sizeof(struct sockaddr_in));
  s_add.sin_family = AF_INET;
  s_add.sin_addr.s_addr = INADDR_ANY;
  s_add.sin_port = htons(TCPPORT);

  if (0 > bind(tcp_server_fd, (struct sockaddr*)(&s_add), sizeof(struct sockaddr))) {
    perror("tcp_server_thread : bind fail !\r\n");
    goto stop_tcp_server_thread;
  }
  printf("tcp_server_thread : bind ok !\r\n");

  if (0 > listen(tcp_server_fd, MAX_CLIENT_LIMIT_NUM)) {
    perror("tcp_server_thread : listen fail !\r\n");
    goto stop_tcp_server_thread;
  }
  printf("listen ok\r\n");
  while (g_wifi_is_running) {
    sin_size = sizeof(struct sockaddr_in);

    nfp = accept(tcp_server_fd, (struct sockaddr*)(&c_add), &sin_size);
    if (nfp < 0) {
      perror("tcp_server_thread : accept fail!\r\n");
      goto stop_tcp_server_thread;
    }
    printf("accept ok!\r\nServer start get connect from %#x : %#x\r\n",
           ntohl(c_add.sin_addr.s_addr), ntohs(c_add.sin_port));

    for(i= 0; i< MAX_CLIENT_LIMIT_NUM; i++){
        if(tcp_server_tab[i]< 0){
            tcp_server_tab[i] = nfp;
            break;
        }
    }

    if (pthread_create(&tcpreceive_tid, NULL, tcp_receive, (void *)nfp) != 0) {
      perror("tcp_server_thread : Create thread error!\n");
//      goto stop_tcp_server_thread;
    }

    printf("tcpreceive TID in pthread_create function: %u.\n",
           (int)tcpreceive_tid);
  }

stop_tcp_server_thread:
  printf("tcp_server_thread : stop tcp_server_thread\n");
  if(tcp_server_fd > 0)
    shutdown(tcp_server_fd, 2);
  pthread_exit(NULL);
}

int runapp(char* cmd) {
  char buffer[BUFSIZ];
  FILE* read_fp;
  int chars_read;
  int ret;

  memset(buffer, 0, BUFSIZ);
  read_fp = popen(cmd, "r");
  if (read_fp != NULL) {
    chars_read = fread(buffer, sizeof(char), BUFSIZ - 1, read_fp);
    if (chars_read > 0) {
      ret = 1;
    } else {
      ret = -1;
    }
    pclose(read_fp);
  } else {
    ret = -1;
  }

  return ret;
}

int loadingwififw(void) {
  FILE* fp;
  char path[] = "//sys//module//bcmdhd//parameters//firmware_path";
  char text[1024] = {0};

  fp = fopen(path, "wt+");
  if (fp != 0) {
    fputs("/system/etc/firmware/fw_bcm4339a0_ag_apsta.bin", fp);
    fclose(fp);
    return 0;
  }
  return -1;
}

int loadingstationfw(void) {
  FILE* fp;
  char path[] = "//sys//module//bcmdhd//parameters//firmware_path";
  char text[1024] = {0};
  fp = fopen(path, "wt+");

  if (fp != 0) {
    fputs("/system/etc/firmware/fw_bcm4339a0_ag.bin", fp);
    fclose(fp);
  }

  fp = fopen("//sys//class//rkwifi//driver", "wt+");
  if (fp != 0) {
    fputs("1", fp);
    fclose(fp);
  }

  return 0;
}

void wifi_management_stop(void) {
  int i;
  
  if (!g_wifi_is_running) {
    printf("Wifi is not runnning, cannot stop it!\n");
    return;
  }

  printf("wifi_management_stop: stop...!\n");
  g_wifi_is_running = 0;

#ifdef RTP_TS_TRANS_ENABLE
  printf("wifi_management_stop: stop ts transfer...!\n");
  //stop_ts_transfer(1);
  video_record_stop_ts_transfer(1);
#endif

  printf("wifi_management_stop: stop lighttpd...!\n");
  runapp("kill -15 $(pidof lighttpd)");

  for(i= 0; i< MAX_CLIENT_LIMIT_NUM; i++){
      if(tcp_server_tab[i]>= 0){
          printf("wifi_management_stop: close socket %d...!\n",tcp_server_tab[i]);
          shutdown(tcp_server_tab[i],2);
          tcp_server_tab[i]= -1;
      }
  }
  
  if(broadcast_fd> 0){
    printf("wifi_management_stop: close broadcast_fd...!\n");
    shutdown(broadcast_fd,2);
    broadcast_fd= -1;
  }
  if(tcp_server_fd> 0){
    printf("wifi_management_stop: close tcp_server_fd...!\n");
    shutdown(tcp_server_fd,2);
    tcp_server_fd= -1;
  }
  
   WlanServiceSetPower("off");
//  pthread_kill(broadcast_tid, 0);
//  pthread_kill(tcp_server_tid, 0);
}

int getifaddr(char* network, char* addressBuffer) {
  struct ifaddrs* ifAddrStruct = NULL;
  void* tmpAddrPtr = NULL;

  getifaddrs(&ifAddrStruct);

  while (ifAddrStruct != NULL) {
    if (ifAddrStruct->ifa_addr->sa_family == AF_INET) {  // check it is IP4
      // is a valid IP4 Address
      tmpAddrPtr = &((struct sockaddr_in*)ifAddrStruct->ifa_addr)->sin_addr;
      inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

      printf("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
      if (strcmp(network, ifAddrStruct->ifa_name) == 0) {
        return 0;
      }
    }
#if 0
    else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)
    {   // check it is IP6
        // is a valid IP6 Address
        tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
        char addressBuffer[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
        printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
    }
#endif
    ifAddrStruct = ifAddrStruct->ifa_next;
  }
  return -1;
}

void* broadcast_thread(void* arg) {
  socklen_t addr_len = 0;
  char buf[64];
  struct sockaddr_in server_addr;
  char wifi_info[100];

  pthread_detach(pthread_self());  // unjoinable

  if ((broadcast_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("broadcast_thread : socket");
    pthread_exit(NULL);
  }
  printf("broadcast_thread socketfd = %d\n", broadcast_fd);

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(UDPPORT);
  addr_len = sizeof(server_addr);

  int opt = -1;
  if (setsockopt(broadcast_fd, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt)) <
      0) {
    perror("broadcast_thread setsockopt SO_BROADCAST");
    goto stop_broadcast_thread;
  }
  if (setsockopt(broadcast_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) <
      0) {
    perror("broadcast_thread setsockopt SO_REUSEADDR");
    goto stop_broadcast_thread;
  }
  
  if (bind(broadcast_fd, (struct sockaddr*)&server_addr, addr_len) < 0) {
    perror("broadcast_thread bind");
    goto stop_broadcast_thread;
  }
  char name[64];
  size_t len = sizeof(name);
  gethostname(name, len);
  while (g_wifi_is_running) {
    printf("Wait Discover....\n");
    addr_len = sizeof(server_addr);
    memset(&server_addr, 0, sizeof(server_addr));
    memset(buf, 0, sizeof(buf));
    if (recvfrom(broadcast_fd, buf, 64, 0, (struct sockaddr*)&server_addr,
                 &addr_len) < 0) {
      perror("broadcast_thread recvfrom");
      goto stop_broadcast_thread;
    }
    //       printf("host name > %s\n",name);
    printf("broadcast_thread : from: %s port: %d > %s\n", inet_ntoa(server_addr.sin_addr),
           ntohs(server_addr.sin_port), buf);

    if (strcmp("CMD_DISCOVER", buf) == 0) {
      memset(wifi_info, 0, sizeof(wifi_info));

      strcat(wifi_info, "CMD_DISCOVER_CVR:WIFINAME:");
      strcat(wifi_info, SSID);
      strcat(wifi_info, "END");

      addr_len = sizeof(server_addr);
      printf("sendto: %s port: %d > %s\n", inet_ntoa(server_addr.sin_addr),
             ntohs(server_addr.sin_port), wifi_info);
      if (sendto(broadcast_fd, wifi_info, strlen(wifi_info), 0,
                 (struct sockaddr*)&server_addr, addr_len) < 0) {
        perror("broadcast_thread recvfrom");
        goto stop_broadcast_thread;
      }
    }
  }

stop_broadcast_thread:
  printf("stop broadcast_thread\n");
  if(broadcast_fd> 0)
    shutdown(broadcast_fd, 2);
  pthread_exit(NULL);
}


void wifi_management_start(void) {

  if (g_wifi_is_running) {
    printf("Wifi is now runnning, don't start again!\n");
    return;
  }

  // printf("Main process: PID: %d,TID: %u.\n",getpid(),pthread_self());
  g_wifi_is_running = 1;

  printf("This is wifi_management_start\n");

  WlanServiceSetPower("on");

  if (parameter_get_wifi_mode()) {
    printf("start station\n");
    parameter_getwifistainfo(SSID, PASSWD);
    WlanServiceSetMode("station", SSID, PASSWD);
  } else {
    printf("start access point\n");
    parameter_getwifiinfo(SSID, PASSWD);
    WlanServiceSetMode("ap", SSID, PASSWD);
  }

  if (pthread_create(&broadcast_tid, NULL, broadcast_thread, NULL) != 0)
    printf("Create thread error!\n");

  if (pthread_create(&tcp_server_tid, NULL, tcp_server_thread, NULL) != 0)
    printf("Createthread error!\n");

  sleep(5);
  runapp("/usr/local/sbin/lighttpd -f /usr/local/etc/lighttpd.conf");
  runapp("route add -net 224.0.0.0 netmask 240.0.0.0 dev wlan0");
}

