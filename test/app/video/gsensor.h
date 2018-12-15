#ifndef __GSENSOR_H__
#define __GSENSOR_H__

#define CMD_COLLISION				0

#define GSENSOR_IOCTL_MAGIC			'a'
#define GBUFF_SIZE				12	/* Rx buffer size */

/* IOCTLs for gsensor */
#define GSENSOR_IOCTL_INIT				_IO(GSENSOR_IOCTL_MAGIC, 0x01)
#define GSENSOR_IOCTL_RESET      	    _IO(GSENSOR_IOCTL_MAGIC, 0x04)
#define GSENSOR_IOCTL_CLOSE		        _IO(GSENSOR_IOCTL_MAGIC, 0x02)
#define GSENSOR_IOCTL_START		        _IO(GSENSOR_IOCTL_MAGIC, 0x03)
//#define GSENSOR_IOCTL_START_DATA_INT    _IO(GSENSOR_IOCTL_MAGIC, 0x05)
//#define GSENSOR_IOCTL_CLOSE_DATA_INT    _IO(GSENSOR_IOCTL_MAGIC, 0x06)
//#define GSENSOR_IOCTL_START_INT2     	_IO(GSENSOR_IOCTL_MAGIC, 0x07)
#define GSENSOR_IOCTL_GETDATA           _IOR(GSENSOR_IOCTL_MAGIC, 0x08, char[GBUFF_SIZE+1])
//#define GSENSOR_IOCTL_CLOSE_INT2    	_IO(GSENSOR_IOCTL_MAGIC, 0x09)

#define GSENSOR_IOCTL_COL_SET			_IO(GSENSOR_IOCTL_MAGIC, 0x10)
#define GSENSOR_IOCTL_USE_INT1			_IOW(GSENSOR_IOCTL_MAGIC, 0x05, char)
#define GSENSOR_IOCTL_USE_INT2			_IOW(GSENSOR_IOCTL_MAGIC, 0x06, char)


#define abs(x) ({ long __x = (x); (__x < 0) ? -__x : __x; })

struct sensor_axis {
	float x;
	float y;
	float z;
};

#endif
