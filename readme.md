
# BukuBuku

BukuBuku is an Open Source eReader project inspired by [The Open Book Project](https://github.com/joeycastillo/The-Open-Book)

## To do

### Phase 1
Phase 1 is mainly PoC and early development of the e-paper display driver
- [ ] write driver for e-paper display
	- [ ] learn how to write kernel driver
	- [ ] create simple SPI driver ( for RTC chip or small e-paper display )
	- [ ] create simple character device driver using SPI ( for RTC chip or small e-paper display )
	- [ ] create simple character device driver using SPI for an e-paper display
	- [ ] create simple framebuffer driver for an e-paper display
	- [ ] create framebuffer driver for the an e-paper display
- [ ] test [fbpdf](https://github.com/aligrudi/fbpdf) on a framebuffer device
	- [ ] test on TFT display
	- [ ] test on e-paper display
- [ ] test Qt program on a framebuffer device
	- [ ] test on TFT display
	- [ ] test on e-paper display
- [ ] create initial Qt program for e-reader

### Phase 2
Phase 2 focused code optimization and testing it on the final chip

### Phase 3
Phase 3 focused on hardware. Schematic design, first iteration of the board, develop sleep-mode for the device, device's power management.

  
## Links

### Potential Chip
-  [SRMX6SOW00D512E000V15C0 SolidRun LTD | Embedded Computers | DigiKey](https://www.digikey.com/product-detail/en/solidrun-ltd/SRMX6SOW00D512E000V15C0/SRMX6SOW00D512E000V15C0-ND/6021930)
-  [PICO-PI-IMX6UL Wandboard.Org | Embedded Computers | DigiKey](https://www.digikey.com/product-detail/en/wandboard.org/PICO-PI-IMX6UL/1406-0012-ND/7318413?utm_adgroup=Single%20Board%20Computers%20(SBCs))
-  [SA69-0100-0100-C0 UDOO | Embedded Computers | DigiKey](https://www.digikey.com/product-detail/en/udoo/SA69-0100-0100-C0/1485-1012-ND/5618137)
-  [102991306 Seeed Technology Co., Ltd | Embedded Computers | DigiKey](https://www.digikey.com/product-detail/en/seeed-technology-co-ltd/102991306/1597-102991306-ND/10492211)
-  [NPi i.MX6ULL® Linux Dev Board - Seeed | DigiKey](https://www.digikey.com/en/product-highlight/s/seeed/npi-imx6ull-linux-sbc-nand-version-development-board)
-  [NPi i.MX6ULL Development Board - Seeed | DigiKey](https://www.digikey.com/en/product-highlight/s/seeed/npi-i-mx6ull-development-board)
-  [PICO-PI-IMX6UL Wandboard.Org | Embedded Computers | DigiKey](https://www.digikey.com/product-detail/en/wandboard-org/PICO-PI-IMX6UL/1406-0012-ND/7318413)
-  [102991305 Seeed Technology Co., Ltd | Embedded Computers | DigiKey](https://www.digikey.com/product-detail/en/seeed-technology-co-ltd/102991305/1597-102991305-ND/10492223)
-  [PICO-IMX6UL-KIT Wandboard.Org | Embedded Computers | DigiKey](https://www.digikey.com/product-detail/en/wandboard.org/PICO-IMX6UL-KIT/1405-0017-ND/6578333?utm_adgroup=Single%20Board%20Computers%20(SBCs))
-  [NEO power specs | UDOO Forum](https://www.udoo.org/forum/threads/neo-power-specs.4416/#post-19677)
-  [PICO-IMX6UL-KIT Wandboard.Org | Embedded Computers | DigiKey](https://www.digikey.com/product-detail/en/wandboard.org/PICO-IMX6UL-KIT/1405-0017-ND/6578333?utm_adgroup=Single%20Board%20Computers%20(SBCs))
-  [All-in-1 ARM Cortex Embedded Development Board | UDOO Neo](https://www.udoo.org/udoo-neo/)
-  [ConnectCore® for NXP i.MX6 UltraLite - Linux SOM | Digi International](https://www.digi.com/products/embedded-systems/system-on-modules/connectcore-for-i-mx6ul#partnumbers)
  
### Kernel Driver 101
-  [BeagleEPD - eLinux.org](https://elinux.org/BeagleEPD)
-  [Nguyen_Vu.pdf](https://www.theseus.fi/bitstream/handle/10024/74679/Nguyen_Vu.pdf)
-  [c - How to write a simple Linux device driver? - Stack Overflow](https://stackoverflow.com/questions/22632713/how-to-write-a-simple-linux-device-driver)
-  [Implementing I2C device drivers — The Linux Kernel documentation](https://www.kernel.org/doc/html/latest/i2c/writing-clients.html)
-  [Writing a Linux Kernel Module — Part 1: Introduction | derekmolloy.ie](http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/)
-  [Linux Device Drivers Training 06, Simple Character Driver - YouTube](https://www.youtube.com/watch?v=E_xrzGlHbac)
-  [How to build a Linux loadable kernel module that Rickrolls people](https://www.youtube.com/watch?v=CWihl19mJig)

### e-paper Driver/Framebuffer Projects
-  [e-ink · GitHub Topics](https://github.com/topics/e-ink)
-  [NiLuJe/FBInk: FrameBuffer eInker, a small tool & library to print text & images to an eInk Linux framebuffer](https://github.com/NiLuJe/FBInk)
-  [repk/epd: Epaper Display Linux kernel driver for EM027AS012](https://github.com/repk/epd)
-  [How can I add an additional framebuffer device in Linux? - Unix & Linux Stack Exchange](https://unix.stackexchange.com/questions/98389/how-can-i-add-an-additional-framebuffer-device-in-linux)
-  [GregDMeyer/IT8951: Driver for the IT8951 e-paper controller on Raspberry Pi](https://github.com/GregDMeyer/IT8951)
-  [Kernel space: E-paper support for Linux | Network World](https://www.networkworld.com/article/2289160/kernel-space--e-paper-support-for-linux.html)
-  [More Fun with Four-Wire SPI: Drawing to “E-Ink” Displays – Vivonomicon's Blog](https://vivonomicon.com/2018/07/06/more-fun-with-four-wire-spi-drawing-to-e-ink-displays/)
-  [joukos/PaperTTY: PaperTTY - Python module to render a TTY or VNC on e-ink](https://github.com/joukos/PaperTTY)
-  [Driving E-ink display – Essential scrap](http://essentialscrap.com/eink/)
  
### Framebuffer 101
-  [Mine of Information - Linux Framebuffer Drivers](http://moi.vonos.net/linux/framebuffer-drivers/)
-  [Writing a Basic Framebuffer Driver](https://opensourceforu.com/2015/05/writing-a-basic-framebuffer-driver/)
-  [c - How do I go about writing a Linux driver for an LCD screen? - Stack Overflow](https://stackoverflow.com/questions/17768558/how-do-i-go-about-writing-a-linux-driver-for-an-lcd-screen)
-  [linux - How to use /dev/fb0 as a console from userspace, or output text to it - Unix & Linux Stack Exchange](https://unix.stackexchange.com/questions/20458/how-to-use-dev-fb0-as-a-console-from-userspace-or-output-text-to-it)
-  [haphazard.io - Raspberry Pi framebuffer FBTFT ILI9341](https://www.haphazard.io/blog/raspberry-pi-framebuffer-fbtft-ili9341/)

### Framebuffer Projects
- [notro/fbtft: Linux Framebuffer drivers for small TFT LCD display modules](https://github.com/notro/fbtft)
- [juj/fbcp-ili9341: A blazing fast display driver for SPI-based LCD displays for Raspberry Pi A, B, 2, 3 and Zero](https://github.com/juj/fbcp-ili9341)
- [Hackaday/fbcp-ili9341: A blazing fast display driver for SPI-based LCD displays for Raspberry Pi A, B, 2, 3 and Zero](https://hackaday.com/2018/10/21/blazing-fast-raspberry-pi-display-driver-will-melt-your-face-then-teach-you-how/)
-  [aligrudi/fbpdf: A small framebuffer pdf, djvu, epub, xps, and cbz viewer](https://github.com/aligrudi/fbpdf)

### SPI Kernel Driver
-  [Using MCP3008 ADCs with Raspberry Pis](https://jumpnowtek.com/rpi/Using-mcp3008-ADCs-with-Raspberry-Pis.html)
-  [linux/mcp320x.c at master · torvalds/linux](https://github.com/torvalds/linux/blob/master/drivers/iio/adc/mcp320x.c)
-  [Writing an RTC Driver Based on the SPI Bus](https://opensourceforu.com/2014/09/writing-an-rtc-driver-based-on-the-spi-bus/)
-  [DS1347T+T&R Maxim Integrated | Integrated Circuits (ICs) | DigiKey](https://www.digikey.com/product-detail/en/maxim-integrated/DS1347T-T-R/DS1347T-T-RTR-ND/2776906)
-  [linux/rtc-ds1347.c at master · torvalds/linux](https://github.com/torvalds/linux/blob/master/drivers/rtc/rtc-ds1347.c)
-  [https://www.sparkfun.com/products/10160](https://www.sparkfun.com/products/10160)
-  [DS3234 Datasheet](https://www.sparkfun.com/datasheets/BreakoutBoards/DS3234.pdf)
-  [https://github.com/spotify/linux/blob/master/drivers/rtc/rtc-ds3234.c](https://github.com/spotify/linux/blob/master/drivers/rtc/rtc-ds3234.c)

### Potential e-paper display
-  [Waveshare 7.8inch E-Ink Display HAT for Raspberry Pi 1872×1404 Resolution with Embedded Controller IT8951 Communicating via USB/SPI/I80/I2C Interface Supports Partial Refresh](https://www.amazon.com/Waveshare-HAT-Resolution-Interface-Controller/dp/B07VL8Y3CQ/ref=sr_1_1?dchild=1&keywords=waveshare%2B9.7&qid=1592326366&sr=8-1&th=1)

### misc
-  [Writing GUI applications on the Raspberry Pi without a desktop environment](https://medium.com/@avik.das/writing-gui-applications-on-the-raspberry-pi-without-a-desktop-environment-8f8f840d9867)