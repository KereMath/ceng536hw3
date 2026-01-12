/* -*- C -*-
 * fb536.h -- definitions for the fb536 framebuffer char module
 *
 * Based on scullc from "Linux Device Drivers" by Alessandro Rubini
 * and Jonathan Corbet, published by O'Reilly & Associates.
 *
 * The source code can be freely used, adapted, and redistributed
 * in source or binary form. An acknowledgment that the code comes
 * from the book "Linux Device Drivers" is appreciated.
 *
 * Modified for CEng 536 - Fall 2025 - Homework 3
 */

#ifndef _FB536_H_
#define _FB536_H_

#include <linux/ioctl.h>

struct fb_viewport {
    unsigned short x, y;
    unsigned short width, height;
};

/* Operations for Write */
#define FB536_SET   0
#define FB536_ADD   1
#define FB536_SUB   2
#define FB536_AND   3
#define FB536_OR    4
#define FB536_XOR   5

/* IOC magic number */
#define FB536_IOC_MAGIC  'F'

/* IOCTL definitions */
#define FB536_IOCRESET       _IO(FB536_IOC_MAGIC, 0)
#define FB536_IOCTSETSIZE    _IO(FB536_IOC_MAGIC, 1)
#define FB536_IOCQGETSIZE    _IO(FB536_IOC_MAGIC, 2)
#define FB536_IOCSETVIEWPORT _IOW(FB536_IOC_MAGIC, 3, struct fb_viewport)
#define FB536_IOCGETVIEWPORT _IOR(FB536_IOC_MAGIC, 4, struct fb_viewport)
#define FB536_IOCTSETOP      _IO(FB536_IOC_MAGIC, 5)
#define FB536_IOCQGETOP      _IO(FB536_IOC_MAGIC, 6)
#define FB536_IOCWAIT        _IO(FB536_IOC_MAGIC, 7)

#define FB536_IOC_MAXNR 7

#endif /* _FB536_H_ */
