#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/poll.h>
#include <dirent.h>
#include <math.h>

#include "gsensor.h"

//the same as camera_ui.c
#define BLOCK_PREV_NUM 1 //file
#define BLOCK_LATER_NUM 1 //file

#define MODE_RECORDING				0
#define MODE_PHOTO					1
#define MODE_EXPLORER				2
#define MODE_PREVIEW				3
#define MODE_PLAY					4
#define MODE_USBDIALOG				5
#define MODE_USBCONNECTION			6

#define GSENSOR_INT_START           1
#define GSENSOR_INT_STOP            0
#define GSENSOR_INT1                1
#define GSENSOR_INT2                2

#define GSDEV "/dev/mma8452_daemon"
#define ACCELERATION_RATIO_ANDROID_TO_HW (9.80665f / 1000000)

static int gsfd;
static int gsinput;
static int gstate;
static struct pollfd fds[1];
static int collision_cnt = 0;
pthread_mutex_t collision_mutex;
void (*gsensor_event_call)(int cmd, void *msg0, void *msg1);

int stop_car_record_ctrl(const int enable)
{
	int rec = 0;
	printf("%s :%d\n", __func__,enable);
	//TODO: 1. add menu config reading & signal for car stop
	//		2. add uevent & react for gsensor interrupt2
	if (enable) {
		rec = 1;
		/*if(ioctl(gsfd, GSENSOR_IOCTL_START_INT2, &rec) < 0) {
			printf("open gsensor int2 failed\n");
			return -1;
		}*/
	} else {
		rec = 0;
		/*if(ioctl(gsfd, GSENSOR_IOCTL_CLOSE_INT2, &rec) < 0) {
			printf("close gsensor int2 failed\n");
			return -1;
		}*/
	}

	return 0;
}

int collision_ctrl(const int level)
{
	if (level < 0) {
		return -1;
	}

	if (level == 0) {
		if (gstate == 1){
			if(ioctl(gsfd, GSENSOR_IOCTL_CLOSE, &level) < 0) {
				printf("start gsensor failed\n");
				return -1;
			}
			gstate = 0;
		}
	} else {
		if (gstate == 0){
			if(ioctl(gsfd, GSENSOR_IOCTL_START, &level) < 0) {
				printf("set gsensor collision failed\n");
				return -1;
			}
			gstate = 1;
		}
	}

	return 0;
}

void collision_react(void)
{
  if (gsensor_event_call)
    (*gsensor_event_call)(CMD_COLLISION, 0, 0);
}

static int axis_calc(struct sensor_axis data[10], int threshold)
{
	int ret = 0;
	int i=0, x, y, z;
	//int maxx, maxy, maxz;

	for (i = 0; i < 7 && abs(data[i+1].x - data[i].x) < 2; i++)
		;
	x = abs(data[i+1].x);

	for (i = 0; i < 7 && abs(data[i+1].y - data[i].y) < 2; i++)
		;
	y = abs(data[i+1].y);

	for (i = 0; i < 7 && abs(data[i+1].z - data[i].z) < 2; i++)
		;
	z = abs(data[i+1].z);

	if (x > threshold) {
		printf("x axis collision\n");
		ret = 1;
	} else if (y > threshold){
		printf("y axis collision\n");
		ret = 2;
	} else if (z > threshold) {
		printf("z axis collision\n");
		ret = 3;
	}

	return ret;
}

static int axis_calc_4max_count(struct sensor_axis data[10], int threshold)
{
	int ret = 0;
	int i;
	int count=0;
	int threshold_count = 3;

	for(i=0;i<10;i++)
	{
		if(abs(data[i].x) > threshold | abs(data[i].y)  > threshold |abs(data[i].z)  > threshold)
		{
			count++;
			printf("[i=%d,count=%d] x = %f, y = %f, z = %f\n",
				i,count,data[i].x, data[i].y, data[i].z);
		}
	}

	if(count >= threshold_count)
	{
		ret = 1;
	}
	return ret;
}
/*
 *
 */
static void *collision_calc(void *arg)
{
	int i = 0, j = 0, ret;
	int inputfd;
	char oldlevel,newlevel;
	struct sensor_axis accData[10];
	struct input_event event;
	int k =0;

	pthread_mutex_lock(&collision_mutex);

	oldlevel = parameter_get_collision_level();

	collision_ctrl(oldlevel);
	while(1) {
		newlevel = parameter_get_collision_level();
		if (oldlevel != newlevel) {
			collision_ctrl(newlevel);
			oldlevel = newlevel;
			parameter_save_collision_level(newlevel);
		}
		if (newlevel == 0) {
			usleep(80000);
			continue;
		}

		k = 0;
		memset(accData, 0, sizeof(accData));
		for(i = 0; i < 10; i++) {
#if 0
			if(ioctl(gsensorfd, GSENSOR_IOCTL_GETDATA, &accData[i]) < 0) {
				printf("failed to get data\n");
				continue;
			}
#else
			while(1) {
				ret = poll(fds, 1, 60);
				if (ret <= 0) {
					if(j++ > 8){
						j = 0;
						printf("wait gsensor err or time out\n");
					}
					break;
				} else {
					ret = read(gsinput, &event, sizeof(event));
				}
				if (ret > 0 && event.type == EV_ABS) {
					//printf("%04x %08x, %f\n", event.code, event.value, event.value);
					if (event.code == ABS_X)
						accData[i].x = event.value * ACCELERATION_RATIO_ANDROID_TO_HW;
					else if (event.code == ABS_Y)
						accData[i].y = event.value * ACCELERATION_RATIO_ANDROID_TO_HW;
					else if (event.code == ABS_Z)
						accData[i].z = event.value * ACCELERATION_RATIO_ANDROID_TO_HW;
				} else if (event.type == EV_SYN) {
					break;
				}
			}
#endif
		}

		//ret = axis_calc(accData, newlevel);
		ret = axis_calc_4max_count(accData, newlevel);
		if (ret) {
			printf("start recording. ret=%d\n",ret);
			collision_react();
			sleep(3);
		}
	}
	pthread_mutex_unlock(&collision_mutex);

	pthread_exit(0);
}

static int open_input(const char* inputname)
{
    int fd = -1;
    const char *dirname = "/dev/input";
    char devname[512];
    char *filename;
    DIR *dir;
    struct dirent *de;

    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
                (de->d_name[1] == '\0' ||
                        (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        fd = open(devname, O_RDONLY);
        if (fd>=0) {
            char name[80];
            if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                name[0] = '\0';
            }
            if (!strcmp(name, inputname)) {
                break;
            } else {
                close(fd);
                fd = -1;
            }
        }
    }
    closedir(dir);

	return fd;
}

int gsensor_use_interrupt(int intr_num, int intr_st)
{
	int ret = 0;
	int int_status = intr_st;

	if (gsfd < 0) {
		printf ("gsensor open fail,not use interrupt%d",intr_num);
		ret = -1;
	} else {
		switch (intr_num) {
			case GSENSOR_INT1:
				if (intr_st == GSENSOR_INT_START) {
					ret = ioctl(gsfd, GSENSOR_IOCTL_USE_INT1, &int_status);
					if (ret < 0)
						printf ("set gsensor int%d status=%d failure\n",intr_num,int_status);
				} else {
					int_status = 0;
					ret = ioctl(gsfd, GSENSOR_IOCTL_USE_INT1, &int_status);
					if (ret < 0)
						printf ("set gsensor int%d status=%d failure\n",intr_num,int_status);
				}
				break;
			case GSENSOR_INT2:
				if (intr_st == GSENSOR_INT_START) {
					ret = ioctl(gsfd, GSENSOR_IOCTL_USE_INT2, &int_status);
					if (ret < 0)
						printf ("set gsensor int%d status=%d failure\n",intr_num,int_status);
				} else {
					int_status = 0;
					ret = ioctl(gsfd, GSENSOR_IOCTL_USE_INT2, &int_status);
					if (ret < 0)
						printf ("set gsensor int%d status=%d failure\n",intr_num,int_status);
				}
				break;
			default:
				printf ("use gsensor int%d error\n",intr_num);
				ret = -1;
				break;
		}
	}
	return ret;
}

void gsensor_release(void)
{
	int tmp;
	if (gsfd >= 0) {
		if (parameter_get_collision_level()) {
			printf("not close gsensor\n");
			collision_ctrl(0);
		}
		close(gsfd);
	}
	if (gsinput >= 0) {
		printf("close gsensor input dev\n");
		close(gsinput);
	}
}

int gsensor_init(void)
{
	pthread_t tid;

	printf("%s\n", __func__);
	gsfd = open(GSDEV, O_RDWR);
	if (gsfd < 0) {
		printf("failed to open gsensor dev\n");
		return -1;
	}
#if 1
	gsinput = open_input("gsensor");
	if (gsinput < 0) {
		printf("failed to open gsensor input dev\n");
		return -1;
	}
	fds[0].fd	  = gsinput;
	fds[0].events = POLLIN;
#endif
	//gsensor driver state 0:off 1:on
	gstate = 0;
	//stop_car_record_ctrl(0);

	pthread_mutex_init(&collision_mutex, NULL);
	if(pthread_create(&tid, NULL, collision_calc, NULL)) {
		printf("create collision_calc pthread failed\n");
		return -1;
	}

	return 0;
}

int GSENSOR_RegEventCallback(void (*call)(int cmd, void *msg0, void *msg1))
{
  gsensor_event_call = call;
  return 0;
}
