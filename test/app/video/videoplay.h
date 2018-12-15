#ifndef __VIDEOPLAY_H__
#define __VIDEOPLAY_H__

#define CMD_VIDEOPLAY_EXIT				0
#define CMD_VIDEOPLAY_UPDATETIME		1

int videoplay_init(char* filepath);
int videoplay_exit(void);
int videoplay_deinit(void);

void videoplay_pause();
void videoplay_step_play();
void videoplay_seek_time(double time_step);
int videoplay_set_speed(int direction);
int videoplay_decode_one_frame(unsigned char* filepath);
void videoplay_decode_jpeg(unsigned char* filepath);
void videoplay_set_fb_black();
int VIDEOPLAY_RegEventCallback(void (*call)(int cmd, void *msg0, void *msg1));
#endif
