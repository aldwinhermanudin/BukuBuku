## setup
current setup for the experimental code under this directory are:
- Raspbery Pi Zero W
  - code test and compiling are done in here. (no cross-compiling)
- Waveshare 2in13B

## requirements
to start developing kernel driver, download the current linux kernel header that the system is using.
checking current kernel version can be done by `uname -r`
for Raspberry Pi downloading the current linux kernel header is by:
    `sudo apt-get install linux-headers-rpi`

## todo
- [ ] fix segfault on experimental #1, #2 and #5
- [ ] finish experimental #5
- [ ] add experimental code to udpate ws2in13b screen through character device driver
