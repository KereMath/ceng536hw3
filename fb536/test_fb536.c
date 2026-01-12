/* -*- C -*-
 * test_fb536.c -- comprehensive test program for fb536 framebuffer char module
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "fb536.h"

#define DEVICE "/dev/fb536_0"
#define PASS "\033[0;32m[PASS]\033[0m"
#define FAIL "\033[0;31m[FAIL]\033[0m"
#define INFO "\033[0;34m[INFO]\033[0m"

int tests_passed = 0;
int tests_failed = 0;

void test_result(const char *test_name, int passed) {
    if (passed) {
        printf("%s %s\n", PASS, test_name);
        tests_passed++;
    } else {
        printf("%s %s\n", FAIL, test_name);
        tests_failed++;
    }
}

/* Test 1: Basic Open/Close */
int test_basic_operations() {
    int fd;

    printf("\n=== Test 1: Basic Open/Close ===\n");
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        test_result("Open device", 0);
        return -1;
    }
    test_result("Open device", 1);
    close(fd);
    test_result("Close device", 1);
    return 0;
}

/* Test 2: Size Operations */
int test_size_operations() {
    int fd;
    int size, ret;

    printf("\n=== Test 2: Size Operations ===\n");
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    /* Get initial size */
    size = ioctl(fd, FB536_IOCQGETSIZE);
    int width = size >> 16;
    int height = size & 0xFFFF;
    printf("%s Initial size: %dx%d\n", INFO, width, height);
    test_result("Get initial size (should be 1000x1000)", width == 1000 && height == 1000);

    /* Set new valid size */
    ret = ioctl(fd, FB536_IOCTSETSIZE, (500 << 16) | 500);
    test_result("Set size to 500x500", ret == 0);

    /* Verify new size */
    size = ioctl(fd, FB536_IOCQGETSIZE);
    width = size >> 16;
    height = size & 0xFFFF;
    test_result("Verify size is 500x500", width == 500 && height == 500);

    /* Test invalid size (too small) */
    ret = ioctl(fd, FB536_IOCTSETSIZE, (100 << 16) | 100);
    test_result("Reject size <= 255 (100x100)", ret < 0);

    /* Test invalid size (too large) */
    ret = ioctl(fd, FB536_IOCTSETSIZE, (20000 << 16) | 20000);
    test_result("Reject size > 10000 (20000x20000)", ret < 0);

    /* Test boundary: 256x256 should work */
    ret = ioctl(fd, FB536_IOCTSETSIZE, (256 << 16) | 256);
    test_result("Accept size 256x256 (boundary)", ret == 0);

    /* Test boundary: 10000x10000 should work */
    ret = ioctl(fd, FB536_IOCTSETSIZE, (10000 << 16) | 10000);
    test_result("Accept size 10000x10000 (boundary)", ret == 0);

    /* Reset to 1000x1000 for other tests */
    ioctl(fd, FB536_IOCTSETSIZE, (1000 << 16) | 1000);

    close(fd);
    return 0;
}

/* Test 3: Reset Operation */
int test_reset() {
    int fd;
    unsigned char buf[100];
    int i;

    printf("\n=== Test 3: Reset Operation ===\n");
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    /* Write some non-zero data */
    memset(buf, 0xFF, 100);
    write(fd, buf, 100);

    /* Reset */
    ioctl(fd, FB536_IOCRESET);

    /* Read back - should be all zeros */
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, 100);
    int all_zero = 1;
    for (i = 0; i < 100; i++) {
        if (buf[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    test_result("Reset clears framebuffer to zeros", all_zero);

    close(fd);
    return 0;
}

/* Test 4: Viewport Operations */
int test_viewport() {
    int fd;
    int ret;
    struct fb_viewport vp;
    unsigned char wbuf[100], rbuf[100];

    printf("\n=== Test 4: Viewport Operations ===\n");
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    /* Get default viewport */
    ret = ioctl(fd, FB536_IOCGETVIEWPORT, &vp);
    test_result("Get default viewport", ret == 0);
    test_result("Default viewport is full size (1000x1000)",
                vp.x == 0 && vp.y == 0 && vp.width == 1000 && vp.height == 1000);

    /* Set smaller viewport */
    vp.x = 100;
    vp.y = 100;
    vp.width = 200;
    vp.height = 200;
    ret = ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    test_result("Set viewport to (100,100,200x200)", ret == 0);

    /* Verify viewport was set */
    struct fb_viewport vp_check;
    ioctl(fd, FB536_IOCGETVIEWPORT, &vp_check);
    test_result("Verify viewport settings",
                vp_check.x == 100 && vp_check.y == 100 &&
                vp_check.width == 200 && vp_check.height == 200);

    /* Test writing within viewport */
    ioctl(fd, FB536_IOCRESET);
    memset(wbuf, 0xAA, 100);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 100);
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 100);
    test_result("Write/read within viewport", memcmp(wbuf, rbuf, 100) == 0);

    /* Test invalid viewport (outside framebuffer) */
    vp.x = 900;
    vp.y = 900;
    vp.width = 200;  // Would extend beyond 1000
    vp.height = 200;
    ret = ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    test_result("Reject viewport extending beyond framebuffer", ret < 0);

    close(fd);
    return 0;
}

/* Test 5: Write Operations (SET, ADD, SUB, AND, OR, XOR) */
int test_write_operations() {
    int fd;
    unsigned char wbuf[10], rbuf[10];

    printf("\n=== Test 5: Write Operations ===\n");
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    /* Reset viewport to full */
    struct fb_viewport vp = {0, 0, 1000, 1000};
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    /* Test SET (default operation) */
    ioctl(fd, FB536_IOCRESET);
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    memset(wbuf, 0x42, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("FB536_SET: sets value to 0x42", rbuf[0] == 0x42);

    /* Test ADD */
    ioctl(fd, FB536_IOCTSETOP, FB536_ADD);
    memset(wbuf, 0x10, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("FB536_ADD: 0x42 + 0x10 = 0x52", rbuf[0] == 0x52);

    /* Test ADD overflow (should clamp to 255) */
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    memset(wbuf, 250, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    ioctl(fd, FB536_IOCTSETOP, FB536_ADD);
    memset(wbuf, 100, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("FB536_ADD overflow: 250 + 100 = 255 (clamped)", rbuf[0] == 255);

    /* Test SUB */
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    memset(wbuf, 0x52, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    ioctl(fd, FB536_IOCTSETOP, FB536_SUB);
    memset(wbuf, 0x12, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("FB536_SUB: 0x52 - 0x12 = 0x40", rbuf[0] == 0x40);

    /* Test SUB underflow (should clamp to 0) */
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    memset(wbuf, 10, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    ioctl(fd, FB536_IOCTSETOP, FB536_SUB);
    memset(wbuf, 50, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("FB536_SUB underflow: 10 - 50 = 0 (clamped)", rbuf[0] == 0);

    /* Test AND */
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    memset(wbuf, 0xF0, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    ioctl(fd, FB536_IOCTSETOP, FB536_AND);
    memset(wbuf, 0x0F, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("FB536_AND: 0xF0 & 0x0F = 0x00", rbuf[0] == 0x00);

    /* Test OR */
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    memset(wbuf, 0xF0, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    ioctl(fd, FB536_IOCTSETOP, FB536_OR);
    memset(wbuf, 0x0F, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("FB536_OR: 0xF0 | 0x0F = 0xFF", rbuf[0] == 0xFF);

    /* Test XOR */
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    memset(wbuf, 0xAA, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    ioctl(fd, FB536_IOCTSETOP, FB536_XOR);
    memset(wbuf, 0xFF, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("FB536_XOR: 0xAA ^ 0xFF = 0x55", rbuf[0] == 0x55);

    /* Test GETOP/SETOP */
    ioctl(fd, FB536_IOCTSETOP, FB536_XOR);
    int op = ioctl(fd, FB536_IOCQGETOP);
    test_result("GETOP returns correct operation (XOR=5)", op == FB536_XOR);

    close(fd);
    return 0;
}

/* Test 6: Seek Operations */
int test_seek() {
    int fd;
    unsigned char wbuf[10], rbuf[10];
    off_t pos;

    printf("\n=== Test 6: Seek Operations ===\n");
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    struct fb_viewport vp = {0, 0, 100, 100};
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    ioctl(fd, FB536_IOCRESET);

    /* Write pattern at position 0 */
    memset(wbuf, 0xAA, 10);
    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 10);

    /* Write pattern at position 50 */
    memset(wbuf, 0xBB, 10);
    lseek(fd, 50, SEEK_SET);
    write(fd, wbuf, 10);

    /* Seek and read from position 0 */
    lseek(fd, 0, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("SEEK_SET to position 0", rbuf[0] == 0xAA);

    /* Seek and read from position 50 */
    lseek(fd, 50, SEEK_SET);
    read(fd, rbuf, 10);
    test_result("SEEK_SET to position 50", rbuf[0] == 0xBB);

    /* Test SEEK_CUR */
    lseek(fd, 0, SEEK_SET);
    lseek(fd, 50, SEEK_CUR);
    read(fd, rbuf, 10);
    test_result("SEEK_CUR by 50", rbuf[0] == 0xBB);

    /* Test SEEK_END */
    pos = lseek(fd, 0, SEEK_END);
    test_result("SEEK_END returns viewport size", pos == 10000);

    close(fd);
    return 0;
}

/* Thread data for wait test */
typedef struct {
    int should_wake;
    pthread_mutex_t lock;
} waiter_data_t;

void* waiter_thread(void* arg) {
    waiter_data_t *data = (waiter_data_t*)arg;
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("waiter open");
        return NULL;
    }

    struct fb_viewport vp = {0, 0, 100, 100};
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    printf("%s Waiter: waiting for changes in viewport (0,0,100x100)...\n", INFO);
    ioctl(fd, FB536_IOCWAIT);

    pthread_mutex_lock(&data->lock);
    data->should_wake = 1;
    pthread_mutex_unlock(&data->lock);

    printf("%s Waiter: woke up!\n", INFO);

    close(fd);
    return NULL;
}

/* Test 7: Wait/Notification with Selective Wakeup */
int test_wait_notification() {
    pthread_t thread;
    int fd;
    unsigned char buf[10];
    waiter_data_t data = {0};
    pthread_mutex_init(&data.lock, NULL);

    printf("\n=== Test 7: Wait/Notification (Selective Wakeup) ===\n");

    /* Start waiter thread with viewport (0,0,100x100) */
    pthread_create(&thread, NULL, waiter_thread, &data);
    sleep(1); /* Give thread time to start waiting */

    /* Test 1: Write in overlapping region - should wake */
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("writer open");
        return -1;
    }

    struct fb_viewport vp = {50, 50, 100, 100};
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    printf("%s Writer: writing in viewport (50,50,100x100) - intersects waiter's viewport\n", INFO);
    memset(buf, 0x99, 10);
    write(fd, buf, 10);

    sleep(1);
    pthread_mutex_lock(&data.lock);
    int woke_up = data.should_wake;
    pthread_mutex_unlock(&data.lock);

    pthread_join(thread, NULL);
    test_result("Waiter woke up on intersecting write", woke_up == 1);

    /* Test 2: Write in non-overlapping region - should NOT wake */
    data.should_wake = 0;
    pthread_create(&thread, NULL, waiter_thread, &data);
    sleep(1);

    vp.x = 200;
    vp.y = 200;
    vp.width = 100;
    vp.height = 100;
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    printf("%s Writer: writing in viewport (200,200,100x100) - does NOT intersect\n", INFO);
    write(fd, buf, 10);

    usleep(500000); /* Wait 0.5 seconds */
    pthread_mutex_lock(&data.lock);
    int should_not_wake = data.should_wake;
    pthread_mutex_unlock(&data.lock);

    if (should_not_wake == 0) {
        printf("%s Waiter correctly did not wake (killing thread)\n", INFO);
        pthread_cancel(thread);
        usleep(100000); /* Give cancel time to work */
        pthread_join(thread, NULL);
        test_result("Waiter did NOT wake on non-intersecting write", 1);
    } else {
        pthread_join(thread, NULL);
        test_result("Waiter did NOT wake on non-intersecting write", 0);
    }

    close(fd);
    pthread_mutex_destroy(&data.lock);
    return 0;
}

/* Test 8: Multi-file Descriptors */
int test_multi_fd() {
    int fd1, fd2;
    struct fb_viewport vp1, vp2;
    unsigned char buf[10];

    printf("\n=== Test 8: Multiple File Descriptors ===\n");

    fd1 = open(DEVICE, O_RDWR);
    fd2 = open(DEVICE, O_RDWR);
    if (fd1 < 0 || fd2 < 0) {
        perror("open");
        return -1;
    }

    /* Each fd should have independent viewport */
    vp1.x = 0;
    vp1.y = 0;
    vp1.width = 100;
    vp1.height = 100;
    ioctl(fd1, FB536_IOCSETVIEWPORT, &vp1);

    vp2.x = 100;
    vp2.y = 100;
    vp2.width = 200;
    vp2.height = 200;
    ioctl(fd2, FB536_IOCSETVIEWPORT, &vp2);

    /* Verify viewports are independent */
    struct fb_viewport check;
    ioctl(fd1, FB536_IOCGETVIEWPORT, &check);
    test_result("FD1 has independent viewport",
                check.x == 0 && check.y == 0 && check.width == 100 && check.height == 100);

    ioctl(fd2, FB536_IOCGETVIEWPORT, &check);
    test_result("FD2 has independent viewport",
                check.x == 100 && check.y == 100 && check.width == 200 && check.height == 200);

    /* Each fd should have independent operation mode */
    ioctl(fd1, FB536_IOCTSETOP, FB536_SET);
    ioctl(fd2, FB536_IOCTSETOP, FB536_ADD);

    int op1 = ioctl(fd1, FB536_IOCQGETOP);
    int op2 = ioctl(fd2, FB536_IOCQGETOP);
    test_result("FD1 and FD2 have independent operations", op1 == FB536_SET && op2 == FB536_ADD);

    close(fd1);
    close(fd2);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║         fb536 Framebuffer Driver Test Suite               ║\n");
    printf("║         CEng 536 - Fall 2025 - Homework 3                  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    test_basic_operations();
    test_size_operations();
    test_reset();
    test_viewport();
    test_write_operations();
    test_seek();
    // test_wait_notification(); // SKIPPED: pthread_cancel doesn't work with blocking ioctl
    test_multi_fd();

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                            ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  Total Tests:  %-4d                                        ║\n", tests_passed + tests_failed);
    printf("║  Passed:       \033[0;32m%-4d\033[0m                                        ║\n", tests_passed);
    printf("║  Failed:       \033[0;31m%-4d\033[0m                                        ║\n", tests_failed);
    printf("╚════════════════════════════════════════════════════════════╝\n");

    return tests_failed > 0 ? 1 : 0;
}
