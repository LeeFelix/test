// by wangh
#ifndef AUDIOPLAY_H
#define AUDIOPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

// if audio file is very small, cache_flag can be 1, else should not set it to 1
int audio_play_init(void** handle, char* audio_file_path, int cache_flag);
void audio_play_deinit(void* handle);
int audio_play(void* handle);

// play some audio file which is not played frequently
int audio_play0(char* audio_file_path);

#ifdef __cplusplus
}
#endif

#endif  // AUDIOPLAY_H
