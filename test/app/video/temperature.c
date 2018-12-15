#include <poll.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include "temperature.h"

TempState state;
int temp_base = 0;
char temp_min[] = "/sys/class/hwmon/hwmon0/device/temp0_min";
char temp_min_alarm[] = "/sys/class/hwmon/hwmon0/device/temp0_min_alarm";
char temp_max[] = "/sys/class/hwmon/hwmon0/device/temp0_max";
char temp_max_hyst[] = "/sys/class/hwmon/hwmon0/device/temp0_max_hyst";
char temp_max_alarm[] = "/sys/class/hwmon/hwmon0/device/temp0_max_alarm";
char temp_input[] = "/sys/class/hwmon/hwmon0/device/temp0_input";

void temp_get_status(TempState* status) {
  status->temp = state.temp;
  status->level = state.level;
}

int temp_set(char* patch, int val) {
  int fd = -1;
  char buf[10] = {0};

  fd = open(patch, O_WRONLY);

  if (fd > 0) {
    sprintf(buf, "%d", val);
    write(fd, buf, sizeof(buf));
    close(fd);
  } else {
    printf("open %s fail\n", patch);
    return -1;
  }

  return 0;
}

int temp_read(char* patch) {
  int fd = -1;
  char temp[10] = {0};
  int i = 0;
  int ret = 0;

  fd = open(patch, O_RDONLY);
  if (fd > 0) {
    read(fd, temp, sizeof(temp));

    while (i < sizeof(temp) && temp[i]) {
      if (temp[i] >= '0' && temp[i] <= '9')
        ret = ret * 10 + temp[i] - '0';
      i++;
    }

    close(fd);
  } else
    printf("can't open %s\n", temp);

  return ret;
}

void* temp_pthread(void* arg) {
  int fd1 = -1;
  int fd2 = -1;
  int ret = 0;
  int temp;
  struct pollfd pfd[2];
  fd1 = open(temp_max_alarm, O_RDONLY);
  fd2 = open(temp_min_alarm, O_RDONLY);
  if (fd1 < 0 || fd2 < 0) {
    printf("get temperature alarm fail\n");
    pthread_exit(0);
  }

  pfd[0].fd = fd1;
  pfd[0].events = POLLPRI;
  pfd[1].fd = fd2;
  pfd[1].events = POLLPRI;
  while (1) {
    pfd[0].revents = 0;
    pfd[1].revents = 0;
    read(pfd[0].fd, &ret, sizeof(ret));
    read(pfd[1].fd, &ret, sizeof(ret));
    ret = poll(pfd, 2, -1);
    if (ret > 0) {
      temp = temp_read(temp_input);
      state.temp = temp;
      if (pfd[0].revents != 0) {  // max_alarm
        FILE* fp = fopen(temp_max_alarm, "r");
        fscanf(fp, "%d", &ret);
        fclose(fp);
        if (ret) {  // temp > max
          if (state.level == temp_level0) {
            state.level = temp_level1;
            temp_set(temp_max, (temp_base + TEMP_LEVEL2_OFFSET));
            temp_set(temp_max_hyst, 5);
          } else if (state.level == temp_level1) {
            state.level = temp_level2;
          }
        } else {  // temp < max - hyst
          if (state.level == temp_level2)
            state.level = temp_level1;
        }
      }
      if (pfd[1].revents != 0) {  // min_alarm
        FILE* fp = fopen(temp_min_alarm, "r");
        fscanf(fp, "%d", &ret);
        fclose(fp);
        if (ret) {  // temp < min
          state.level = temp_level0;
          temp_set(temp_max, (temp_base - TEMP_LEVEL1_OFFSET));
          temp_set(temp_max_hyst, (TEMP_LEVEL0_OFFSET - TEMP_LEVEL1_OFFSET));
        }
      }
      printf("temp:%d level:%d\n", state.temp, state.level);
    }
  }
  close(fd1);
  close(fd2);
  pthread_exit(0);
}

int temp_control_init(void) {
  int ret;
  pthread_t pid;
  char buf[10] = {0};
  FILE* fp = fopen("/sys/dvfs/cpu_temp_target", "r");
  if (fp == NULL) {
    printf("can't get cpu_temp_target, temp control inti fail\n");
    goto err;
  }
  fscanf(fp, "%s", buf);
  fclose(fp);
  temp_base = atoi((buf + 4));
  state.level = temp_level0;
  state.temp = temp_read(temp_input);
  if (temp_set(temp_min, (temp_base - TEMP_LEVEL0_OFFSET)) < 0)
    goto err;
  if (temp_set(temp_max, (temp_base - TEMP_LEVEL1_OFFSET)) < 0)
    goto err;
  if (temp_set(temp_max_hyst, (TEMP_LEVEL0_OFFSET - TEMP_LEVEL1_OFFSET)) < 0)
    goto err;

  if (pthread_create(&pid, NULL, temp_pthread, NULL))
    goto err;

  printf("temperature pthread create ok\n");

  return 0;

err:
  printf("temperature control init fail!\n");
  return -1;
}
