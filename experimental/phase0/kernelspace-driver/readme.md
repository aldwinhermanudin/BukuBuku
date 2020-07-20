## setup
current setup for the experimental code under this directory are:
- Raspbery Pi Zero W
  - code test and compiling are done in-device. (no cross-compiling)
  - uses the following operating system
        Raspberry Pi OS (32-bit) Lite
        Minimal image based on Debian Buster
        Version         :May 2020
        Release date    :2020-05-27
        Kernel version  :4.19
        Size            :432 MB
- [Waveshare 2in13B](https://www.waveshare.com/wiki/2.13inch_e-Paper_HAT_(B)) for experiment #0 to #6 

## requirements
to start developing kernel driver, download the current linux kernel header that the system is using.
checking current kernel version can be done by `uname -r`
for Raspberry Pi downloading the current linux kernel header is by:
    `sudo apt-get install linux-headers-rpi`

## notes 
- should refer to experimental #1 for proper character device driver
- experiment #6 is the final experiment for Waveshare 2in13B. future experiments should refer to this first.

## todo
- [ ] ~~fix segfault on experimental #1, #2 and #5~~ ( for some reason, if the module is not loaded on boot, on the first insmod-rmmod, it will segfault on the rmmod, this is because of the character device )
- [x] updated experimental #1 with the proper char device driver method
- [x] finish experimental #5
- [x] add experimental code #6 to update ws2in13b screen through character device driver
- [ ] use tindrm_it8951 as the main driver, copy/link tinydrm_it8951 under /driver/tinydrm_it8951 of this project
- [ ] improve tinydrm_it8951 
	- [ ] read vcom from device tree
	- [ ] read spi max speed from device tree
	- [ ] add support for different e-paper size and resolution