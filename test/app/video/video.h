#ifndef __VIDEO_H__
#define __VIDEO_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_UPDATETIME			0
#define CMD_ADAS						1
#define CMD_PHOTOEND				2
void video_record_getfilename(char *str, unsigned short str_len,
                              const char *path, int ch, const char *filetype);

int video_record_addvideo(int id, struct ui_frame *front,
                          struct ui_frame *back, char rec_immediately);
int video_record_deletevideo(int deviceid);
void video_record_init(struct ui_frame *front,
                       struct ui_frame *back);
void video_record_deinit(void);
int video_record_startrec(void);
void video_record_stoprec(void);
void video_record_savefile(void);
void video_record_blocknotify(int prev_num, int later_num);

void video_record_setaudio(int flag);
int video_record_set_power_line_frequency(int i);
void video_record_takephoto(void);

void video_record_start_cache(int sec);
void video_record_stop_cache();

void video_record_reset_bitrate();

/* default attach to the main video node, TODO... */
void video_record_start_ts_transfer(char *url);
void video_record_stop_ts_transfer(char sync);
void video_record_attach_user_muxer(void *muxer, char *uri, int need_av);
void video_record_detach_user_muxer(void *muxer);
void video_record_attach_user_mdprocessor(void *processor, void *md_attr);
void video_record_detach_user_mdprocessor(void *processor);
void video_record_rate_change(void *processor, int low_frame_rate);
int REC_RegEventCallback(void (*call)(int cmd, void *msg0, void *msg1));
int video_record_get_list_num(void);
int video_record_get_front_resolution(struct ui_frame* frame, int count);
int video_record_get_back_resolution(struct ui_frame* frame, int count);
void video_record_get_user_noise(void);
#ifdef __cplusplus
}
#endif

#endif
