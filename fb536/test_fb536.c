#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include "fb536.h"

#define DEVICE "/dev/fb5360"

void test_basic_rw() {
    printf("Testing Basic Read/Write and Viewport... ");
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { perror("open"); exit(1); }

    // Reset
    ioctl(fd, FB536_IOCRESET);

    // Set Viewport (10,10, 5, 2)
    struct fb_viewport vp = {10, 10, 5, 2};
    if (ioctl(fd, FB536_IOCSETVIEWPORT, &vp) < 0) { perror("ioctl setviewport"); exit(1); }

    // Write pattern
    char data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    if (write(fd, data, 10) != 10) { perror("write"); exit(1); }

    // Read back
    char buf[10];
    lseek(fd, 0, SEEK_SET);
    if (read(fd, buf, 10) != 10) { perror("read"); exit(1); }

    for (int i=0; i<10; i++) {
        if (buf[i] != data[i]) {
            printf("FAIL: Index %d expected %d got %d\n", i, data[i], buf[i]);
            exit(1);
        }
    }
    printf("PASS\n");
    close(fd);
}

void test_operations() {
    printf("Testing Operations (ADD)... ");
    int fd = open(DEVICE, O_RDWR);
    
    struct fb_viewport vp = {20, 20, 1, 1};
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    char val = 10;
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    write(fd, &val, 1);

    val = 5;
    ioctl(fd, FB536_IOCTSETOP, FB536_ADD);
    write(fd, &val, 1); // Should be 15

    lseek(fd, 0, SEEK_SET);
    char res;
    read(fd, &res, 1);
    
    if (res != 15) {
        printf("FAIL: Expected 15 got %d\n", res);
        exit(1);
    }
    printf("PASS\n");
    close(fd);
}

void *waiter_thread(void *arg) {
    int fd = *(int*)arg;
    printf("[Thread] Waiting for update...\n");
    int ret = ioctl(fd, FB536_IOCWAIT);
    printf("[Thread] Woke up! ret=%d\n", ret);
    return NULL;
}

void test_wait_blocking() {
    printf("Testing Blocking (IOCWAIT)... \n");
    int fd_waiter = open(DEVICE, O_RDWR);
    int fd_writer = open(DEVICE, O_RDWR);
    
    // Viewport overlap
    struct fb_viewport vp = {50, 50, 10, 10};
    ioctl(fd_waiter, FB536_IOCSETVIEWPORT, &vp);
    ioctl(fd_writer, FB536_IOCSETVIEWPORT, &vp);

    pthread_t th;
    pthread_create(&th, NULL, waiter_thread, &fd_waiter);

    sleep(2);
    printf("Writing now...\n");
    char c = 1;
    write(fd_writer, &c, 1);

    pthread_join(th, NULL);
    printf("PASS\n");
    close(fd_waiter);
    close(fd_writer);
}

int main() {
    test_basic_rw();
    test_operations();
    test_wait_blocking();
    printf("All tests completed successfully.\n");
    return 0;
}
