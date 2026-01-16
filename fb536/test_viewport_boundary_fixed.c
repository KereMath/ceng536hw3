#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "fb536.h"

int main() {
    int fd1, fd2;
    unsigned char buf[10];
    ssize_t ret;
    int size;

    printf("=== Viewport Boundary Check Test (Fixed) ===\n\n");

    fd1 = open("/dev/fb536_0", O_RDWR);
    if (fd1 < 0) {
        perror("Failed to open device");
        return 1;
    }

    size = ioctl(fd1, FB536_IOCQGETSIZE);
    int initial_w = size >> 16;
    int initial_h = size & 0xFFFF;
    printf("Initial framebuffer: %dx%d\n", initial_w, initial_h);

    ioctl(fd1, FB536_IOCTSETSIZE, (500 << 16) | 500);
    printf("Set framebuffer to 500x500\n");
    close(fd1);

    fd1 = open("/dev/fb536_0", O_RDWR);
    printf("FD1: Opened with 500x500 framebuffer\n");

    fd2 = open("/dev/fb536_0", O_RDWR);
    ioctl(fd2, FB536_IOCTSETSIZE, (300 << 16) | 300);
    printf("FD2: Resized framebuffer to 300x300\n");
    close(fd2);

    printf("\nFD1: Attempting read (viewport 500x500 > framebuffer 300x300)...\n");
    ret = read(fd1, buf, 10);
    printf("Read returned: %zd (expected: 0 for EOF)\n", ret);
    printf("%s\n\n", ret == 0 ? "PASS - Returns EOF" : "FAIL - Should return EOF!");

    printf("FD1: Attempting write...\n");
    ret = write(fd1, buf, 10);
    printf("Write returned: %zd (expected: 0 for EOF)\n", ret);
    printf("%s\n", ret == 0 ? "PASS - Returns EOF" : "FAIL - Should return EOF!");

    close(fd1);

    fd1 = open("/dev/fb536_0", O_RDWR);
    ioctl(fd1, FB536_IOCTSETSIZE, (initial_w << 16) | initial_h);
    printf("\nRestored framebuffer to %dx%d\n", initial_w, initial_h);
    close(fd1);

    return 0;
}
