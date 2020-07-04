source of this experimental code are:
  1) https://github.com/notro/fbtft/wiki/How-it-works
  2) https://github.com/notro/fbtft/wiki/fbtft_device
  3) https://github.com/repk/epd
  4) https://github.com/repk/epd/blob/master/rpi/rpi-epd-overlay.dts
  5) https://github.com/notro/fbtft/wiki
  6) https://github.com/notro/fbtft/wiki/FBTFT-on-Raspian

### running the experiment
-  The kernel needs to know the "specification" of the SPI device before the kernel module can start probing. There is 2 ways to inform the kernel. 
- The first method is by hard-coding it in the kernel it self ( this needs a full recompile of the kernel ) or by using the device tree.
- This experiment uses the Device Tree method and the instruction can be found in the `overlay` folder of this experiment
- After loading the device tree, kernel module can be loaded by the usual `insmod` command. once loaded function `ws2in13b_init`, `ws2in13b_probe` will be called sequentially
- When `rmmod` is executed, function `ws2in13b_remove` and `ws2in13b_exit` will be called sequentially