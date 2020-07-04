for some reason, if the module is not loaded on boot, on the first insmod-rmmod, it will segfault on the rmmod, this issues roots from experimental #1 
source of this experimental code are below:
  1) https://github.com/repk/epd/blob/master/epd_g1.c
  2) https://imxdev.gitlab.io/tutorial/How_to_add_DT_support_for_a_driver/

## notes
- this is the final experiment for disply Waveshare 2in13B
- this experiment has the same capability that the original waveshare code has (can be found under `../../../userspace-driver/src/basic-test.cpp`)

## running the experiment
1) prepare the same overlay as with experiment #4
2) load the kernel module using the `insmod` command
3) after loading this will initialize the display. Using a logic analyzer, there should be some SPI communication going on
4) for this experiment, the display can only draw in red ink
5) drawing on the display can be done by running: `printf "\x00\x00" > /dev/ws2in13bchar0`
   this will make the first 64 pixels to turn to red. 
6) 64 pixels comes from "\x00\x00" which means 2 bytes of all-zero (for proof, try run `printf "\x00\x00" | xxd -b`). 
   1 byte is equal to 8 bit, and 1 bit controls 1 pixel. 
   there's more detailed explanation in `../../../userspace-driver/src/basic-test.cpp`