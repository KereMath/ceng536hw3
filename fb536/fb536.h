#ifndef _FB536_H_
#define _FB536_H_

#include <linux/ioctl.h>

struct fb_viewport {
    unsigned short x, y;
    unsigned short width, height;
};

#define FB536_SET   0
#define FB536_ADD   1
#define FB536_SUB   2
#define FB536_AND   3
#define FB536_OR    4
#define FB536_XOR   5

#define FB536_IOC_MAGIC  'F'

#define FB536_IOCRESET       _IO(FB536_IOC_MAGIC, 0)
#define FB536_IOCTSETSIZE    _IO(FB536_IOC_MAGIC, 1)
#define FB536_IOCQGETSIZE    _IO(FB536_IOC_MAGIC, 2)
#define FB536_IOCSETVIEWPORT _IOW(FB536_IOC_MAGIC, 3, struct fb_viewport)
#define FB536_IOCGETVIEWPORT _IOR(FB536_IOC_MAGIC, 4, struct fb_viewport)
#define FB536_IOCTSETOP      _IO(FB536_IOC_MAGIC, 5)
#define FB536_IOCQGETOP      _IO(FB536_IOC_MAGIC, 6)
#define FB536_IOCWAIT        _IO(FB536_IOC_MAGIC, 7)

#define FB536_IOC_MAXNR 7

#endif
