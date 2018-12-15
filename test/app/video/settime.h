#ifndef __SETTIME_H__
#define __SETTIME_H__
int rtcSetTime(const struct tm* tm_time);
int setDateTime(struct tm* ptm);
#endif