cd /root/fb536
make clean
make KBUILD_MODPOST_WARN=1
ls -lh fb536.ko
insmod fb536.ko
dmesg | tail -20
