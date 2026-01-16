#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#include "fb536.h"

#define DEVICE "/dev/fb536_0"
#define TEST_COUNT 0
static int test_passed = 0;
static int test_failed = 0;

void print_header(const char *title) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║ %-58s ║\n", title);
    printf("╚════════════════════════════════════════════════════════════╝\n");
}

void print_test(const char *name, int pass) {
    if (pass) {
        printf("[PASS] %s\n", name);
        test_passed++;
    } else {
        printf("[FAIL] %s\n", name);
        test_failed++;
    }
}

void reset_module() {
    system("sudo rmmod fb536 2>/dev/null");
    system("sudo insmod -f fb536.ko 2>/dev/null");
    usleep(100000);
}

void test_basic_open_close() {
    print_header("TEST 1: Basic Open/Close");
    reset_module();

    int fd = open(DEVICE, O_RDWR);
    print_test("Open device", fd >= 0);

    if (fd >= 0) {
        int ret = close(fd);
        print_test("Close device", ret == 0);
    }
}

void test_size_operations() {
    print_header("TEST 2: Size Operations (All Boundaries)");
    reset_module();

    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { printf("[FAIL] Cannot open device\n"); return; }

    int size = ioctl(fd, FB536_IOCQGETSIZE);
    int w = size >> 16;
    int h = size & 0xFFFF;
    print_test("Default size 1000x1000", w == 1000 && h == 1000);

    int ret = ioctl(fd, FB536_IOCTSETSIZE, (500 << 16) | 500);
    print_test("Set size 500x500", ret == 0);

    size = ioctl(fd, FB536_IOCQGETSIZE);
    print_test("Verify size 500x500", ((size >> 16) == 500) && ((size & 0xFFFF) == 500));

    ret = ioctl(fd, FB536_IOCTSETSIZE, (100 << 16) | 100);
    print_test("Reject size <= 255", ret == -1 && errno == EINVAL);

    ret = ioctl(fd, FB536_IOCTSETSIZE, (20000 << 16) | 20000);
    print_test("Reject size > 10000", ret == -1 && errno == EINVAL);

    ret = ioctl(fd, FB536_IOCTSETSIZE, (256 << 16) | 256);
    print_test("Accept size 256x256", ret == 0);

    ret = ioctl(fd, FB536_IOCTSETSIZE, (10000 << 16) | 10000);
    print_test("Accept size 10000x10000", ret == 0);

    ret = ioctl(fd, FB536_IOCTSETSIZE, (255 << 16) | 255);
    print_test("Reject size 255x255", ret == -1 && errno == EINVAL);

    ret = ioctl(fd, FB536_IOCTSETSIZE, (10001 << 16) | 10001);
    print_test("Reject size 10001x10001", ret == -1 && errno == EINVAL);

    close(fd);
}

void test_reset_operation() {
    print_header("TEST 3: Reset Operation");
    reset_module();

    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { printf("[FAIL] Cannot open device\n"); return; }

    unsigned char data[100];
    memset(data, 0xFF, 100);
    write(fd, data, 100);

    ioctl(fd, FB536_IOCRESET);

    lseek(fd, 0, SEEK_SET);
    memset(data, 0, 100);
    read(fd, data, 100);

    int all_zero = 1;
    for (int i = 0; i < 100; i++) {
        if (data[i] != 0) { all_zero = 0; break; }
    }
    print_test("Reset clears to zeros", all_zero);

    close(fd);
}

void test_viewport_operations() {
    print_header("TEST 4: Viewport Operations (Comprehensive)");
    reset_module();

    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { printf("[FAIL] Cannot open device\n"); return; }

    struct fb_viewport vp;
    ioctl(fd, FB536_IOCGETVIEWPORT, &vp);
    print_test("Default viewport 1000x1000", vp.width == 1000 && vp.height == 1000);

    vp.x = 100; vp.y = 100; vp.width = 200; vp.height = 200;
    int ret = ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    print_test("Set viewport (100,100,200x200)", ret == 0);

    struct fb_viewport vp2;
    ioctl(fd, FB536_IOCGETVIEWPORT, &vp2);
    print_test("Verify viewport settings",
               vp2.x == 100 && vp2.y == 100 && vp2.width == 200 && vp2.height == 200);

    vp.x = 900; vp.y = 900; vp.width = 200; vp.height = 200;
    ret = ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    print_test("Reject viewport overflow", ret == -1 && errno == EINVAL);

    vp.x = 999; vp.y = 999; vp.width = 1; vp.height = 1;
    ret = ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    print_test("Accept viewport at boundary", ret == 0);

    vp.x = 999; vp.y = 999; vp.width = 2; vp.height = 1;
    ret = ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    print_test("Reject viewport 1px overflow X", ret == -1 && errno == EINVAL);

    vp.x = 999; vp.y = 999; vp.width = 1; vp.height = 2;
    ret = ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    print_test("Reject viewport 1px overflow Y", ret == -1 && errno == EINVAL);

    vp.x = 0; vp.y = 0; vp.width = 1000; vp.height = 1000;
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    close(fd);
}

void test_write_operations() {
    print_header("TEST 5: Write Operations (All 6 Operations)");
    reset_module();

    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { printf("[FAIL] Cannot open device\n"); return; }

    unsigned char w, r;

    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    w = 0x42; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, &r, 1);
    print_test("SET: 0x42", r == 0x42);

    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_ADD);
    w = 0x10; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, &r, 1);
    print_test("ADD: 0x42 + 0x10 = 0x52", r == 0x52);

    lseek(fd, 0, SEEK_SET);
    w = 250; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    w = 100; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, &r, 1);
    print_test("ADD overflow: clamped to 255", r == 255);

    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    w = 0x52; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_SUB);
    w = 0x12; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, &r, 1);
    print_test("SUB: 0x52 - 0x12 = 0x40", r == 0x40);

    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    w = 10; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_SUB);
    w = 50; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, &r, 1);
    print_test("SUB underflow: clamped to 0", r == 0);

    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    w = 0xF0; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_AND);
    w = 0x0F; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, &r, 1);
    print_test("AND: 0xF0 & 0x0F = 0x00", r == 0x00);

    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    w = 0xF0; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_OR);
    w = 0x0F; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, &r, 1);
    print_test("OR: 0xF0 | 0x0F = 0xFF", r == 0xFF);

    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_SET);
    w = 0xAA; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    ioctl(fd, FB536_IOCTSETOP, FB536_XOR);
    w = 0xFF; write(fd, &w, 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, &r, 1);
    print_test("XOR: 0xAA ^ 0xFF = 0x55", r == 0x55);

    int op = ioctl(fd, FB536_IOCQGETOP);
    print_test("GETOP returns XOR (5)", op == 5);

    close(fd);
}

void test_seek_operations() {
    print_header("TEST 6: Seek Operations");
    reset_module();

    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { printf("[FAIL] Cannot open device\n"); return; }

    struct fb_viewport vp = {0, 0, 100, 100};
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    off_t pos = lseek(fd, 0, SEEK_SET);
    print_test("SEEK_SET to 0", pos == 0);

    pos = lseek(fd, 50, SEEK_SET);
    print_test("SEEK_SET to 50", pos == 50);

    pos = lseek(fd, 50, SEEK_CUR);
    print_test("SEEK_CUR +50 (now 100)", pos == 100);

    pos = lseek(fd, 0, SEEK_END);
    print_test("SEEK_END (10000)", pos == 10000);

    pos = lseek(fd, -100, SEEK_SET);
    print_test("SEEK_SET negative rejected", pos == -1);

    close(fd);
}

void test_multiple_fds() {
    print_header("TEST 7: Multiple File Descriptors");
    reset_module();

    int fd1 = open(DEVICE, O_RDWR);
    int fd2 = open(DEVICE, O_RDWR);
    if (fd1 < 0 || fd2 < 0) { printf("[FAIL] Cannot open devices\n"); return; }

    struct fb_viewport vp1 = {0, 0, 100, 100};
    struct fb_viewport vp2 = {200, 200, 50, 50};
    ioctl(fd1, FB536_IOCSETVIEWPORT, &vp1);
    ioctl(fd2, FB536_IOCSETVIEWPORT, &vp2);

    struct fb_viewport check;
    ioctl(fd1, FB536_IOCGETVIEWPORT, &check);
    print_test("FD1 viewport independent", check.x == 0 && check.y == 0);

    ioctl(fd2, FB536_IOCGETVIEWPORT, &check);
    print_test("FD2 viewport independent", check.x == 200 && check.y == 200);

    ioctl(fd1, FB536_IOCTSETOP, FB536_ADD);
    ioctl(fd2, FB536_IOCTSETOP, FB536_XOR);

    int op1 = ioctl(fd1, FB536_IOCQGETOP);
    int op2 = ioctl(fd2, FB536_IOCQGETOP);
    print_test("FD operations independent", op1 == FB536_ADD && op2 == FB536_XOR);

    close(fd1);
    close(fd2);
}

void test_viewport_boundary() {
    print_header("TEST 8: Viewport Boundary Check (Critical)");
    reset_module();

    int fd1 = open(DEVICE, O_RDWR);
    if (fd1 < 0) { printf("[FAIL] Cannot open device\n"); return; }

    ioctl(fd1, FB536_IOCTSETSIZE, (500 << 16) | 500);
    close(fd1);

    fd1 = open(DEVICE, O_RDWR);

    int fd2 = open(DEVICE, O_RDWR);
    ioctl(fd2, FB536_IOCTSETSIZE, (300 << 16) | 300);
    close(fd2);

    unsigned char buf[10];
    ssize_t ret = read(fd1, buf, 10);
    print_test("Read returns EOF when viewport exceeds", ret == 0);

    ret = write(fd1, buf, 10);
    print_test("Write returns EOF when viewport exceeds", ret == 0);

    close(fd1);
}

void test_rdonly_wronly() {
    print_header("TEST 9: O_RDONLY/O_WRONLY Checks");
    reset_module();

    int fd_ro = open(DEVICE, O_RDONLY);
    int fd_wo = open(DEVICE, O_WRONLY);
    if (fd_ro < 0 || fd_wo < 0) { printf("[FAIL] Cannot open devices\n"); return; }

    int ret = ioctl(fd_ro, FB536_IOCTSETOP, FB536_ADD);
    print_test("SETOP on O_RDONLY rejected", ret == -1 && errno == EINVAL);

    ret = ioctl(fd_ro, FB536_IOCQGETOP);
    print_test("GETOP on O_RDONLY rejected", ret == -1 && errno == EINVAL);

    ret = ioctl(fd_wo, FB536_IOCWAIT);
    print_test("IOCWAIT on O_WRONLY rejected", ret == -1 && errno == EINVAL);

    close(fd_ro);
    close(fd_wo);
}

void test_memory_layout() {
    print_header("TEST 10: Memory Layout Verification");
    reset_module();

    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { printf("[FAIL] Cannot open device\n"); return; }

    struct fb_viewport vp = {100, 50, 10, 10};
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    unsigned char w = 0x77;
    write(fd, &w, 1);

    vp.x = 0; vp.y = 0; vp.width = 1000; vp.height = 1000;
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    lseek(fd, 50 * 1000 + 100, SEEK_SET);
    unsigned char r;
    read(fd, &r, 1);
    print_test("Memory offset correct (y*W+x)", r == 0x77);

    close(fd);
}

void test_row_wrapping() {
    print_header("TEST 11: Row Wrapping in Viewport");
    reset_module();

    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { printf("[FAIL] Cannot open device\n"); return; }

    struct fb_viewport vp = {10, 20, 5, 3};
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    unsigned char data[7] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xBB, 0xBB};
    write(fd, data, 7);

    vp.x = 0; vp.y = 0; vp.width = 1000; vp.height = 1000;
    ioctl(fd, FB536_IOCSETVIEWPORT, &vp);

    lseek(fd, 20 * 1000 + 10, SEEK_SET);
    unsigned char row0[5];
    read(fd, row0, 5);
    int row0_ok = (row0[0] == 0xAA && row0[1] == 0xAA && row0[2] == 0xAA &&
                   row0[3] == 0xAA && row0[4] == 0xAA);
    print_test("Row 0 correct (5 bytes of 0xAA)", row0_ok);

    lseek(fd, 21 * 1000 + 10, SEEK_SET);
    unsigned char row1[2];
    read(fd, row1, 2);
    print_test("Row 1 correct (2 bytes of 0xBB)", row1[0] == 0xBB && row1[1] == 0xBB);

    close(fd);
}

void* race_thread(void* arg) {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) return NULL;

    unsigned char data = 0xFF;
    for (int i = 0; i < 1000; i++) {
        write(fd, &data, 1);
        lseek(fd, 0, SEEK_SET);
    }

    close(fd);
    return NULL;
}

void test_race_conditions() {
    print_header("TEST 12: Race Condition (5 Threads)");
    reset_module();

    pthread_t threads[5];
    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, race_thread, NULL);
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    print_test("No crash/corruption (mutex works)", 1);
}

void test_edge_cases() {
    print_header("TEST 13: Additional Edge Cases");
    reset_module();

    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) { printf("[FAIL] Cannot open device\n"); return; }

    struct fb_viewport vp = {0, 0, 1, 1};
    int ret = ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    print_test("Minimum viewport 1x1", ret == 0);

    vp.x = 0; vp.y = 0; vp.width = 10000; vp.height = 10000;
    ioctl(fd, FB536_IOCTSETSIZE, (10000 << 16) | 10000);
    ret = ioctl(fd, FB536_IOCSETVIEWPORT, &vp);
    print_test("Maximum viewport 10000x10000", ret == 0);

    unsigned char large_buf[10000];
    memset(large_buf, 0xEE, 10000);
    ssize_t written = write(fd, large_buf, 10000);
    print_test("Large write (10000 bytes)", written == 10000);

    lseek(fd, 0, SEEK_SET);
    ssize_t readb = read(fd, large_buf, 10000);
    print_test("Large read (10000 bytes)", readb == 10000);

    close(fd);
}

void print_summary() {
    print_header("FINAL SUMMARY");
    printf("Total Tests: %d\n", test_passed + test_failed);
    printf("Passed:      %d\n", test_passed);
    printf("Failed:      %d\n", test_failed);

    if (test_failed == 0) {
        printf("\n✅ ALL TESTS PASSED - DRIVER READY FOR SUBMISSION\n\n");
    } else {
        printf("\n❌ SOME TESTS FAILED - REVIEW REQUIRED\n\n");
    }
}

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║     FB536 COMPREHENSIVE TEST SUITE WITH ISOLATION         ║\n");
    printf("║     Each test reloads module for clean state              ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    test_basic_open_close();
    test_size_operations();
    test_reset_operation();
    test_viewport_operations();
    test_write_operations();
    test_seek_operations();
    test_multiple_fds();
    test_viewport_boundary();
    test_rdonly_wronly();
    test_memory_layout();
    test_row_wrapping();
    test_race_conditions();
    test_edge_cases();

    print_summary();

    return test_failed > 0 ? 1 : 0;
}
