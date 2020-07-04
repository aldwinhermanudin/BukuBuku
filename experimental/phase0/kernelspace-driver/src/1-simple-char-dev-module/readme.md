for some reason, if the module is not loaded on boot, on the first insmod-rmmod, it will segfault on the rmmod.
this source code is based on:
  - Linux Device Drivers Development: Develop customized drivers for embedded Linux by John Madieu
    - Chapter 4: Character Device Drivers
  - Linux Device Driver Development Cookbook Develop custom drivers for your embedded Linux applications by Rodolfo Giometti
    - Chapter 3: Working with Char Drivers
  - https://olegkutkov.me/2018/03/14/simple-linux-character-device-driver/
  - http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/
 
testing the code is done by 
  1. running **userspace-test.c**: run `make` and then `bash test`
  2. running `head` and `printf`
      - write to the device by: `printf "this_is_the_payload" > /dev/epdchar0`
      - read 20 bytes from the device by: `head -c 20 /dev/epdchar0`
