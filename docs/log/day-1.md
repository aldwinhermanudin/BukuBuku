# Day 1
Day 1 of the effort to document every development of the project (basically a diary). The goal of this is to capture ideas in-case it got lost.

Thing I have in mind are:
 - next development step
 - create a proper structure for the directories.
 - types of future boards to produce

## Next Development Step
The next step are perfectly log  under `experimental/phase0/kernelspace-driver/readme.md`, which are
  - add resource directory and add a readme.md under it.
  - use tindrm_it8951 as the main driver, copy/link tinydrm_it8951 under /driver/tinydrm_it8951 of this project
  - improve tinydrm_it8951
    - read vcom from device tree
    - read spi max speed from device tree
    - add support for different e-paper size and resolution

Thing I want to keep in mind is that I need to finish up `experimental/phase0/kernelspace-driver/src/7-framebuffer` before working on the tinydrm_it8951. The reason for this is, to have a better understanding of the framebuffer driver first before I can confidently venture into the next level. The to-do list are listed on the `readme.md`

## Directory Structuring
In my spare-time, I have thought about the structure of the project. For know what I have in mind is that this repository will host all the source code for the project including, **drivers, schematic, documents, software, etc**. My Idea is to have the following structure:

    ├── apps
    │	└── bacaBuku
    ├── boards
    │   ├── development-kit-a
    │   └── development-kit-b
    ├── docs
    │   ├── log
    │   └── resources
    ├── drivers
    │   └── tinydrm_it8951
    ├── experimental
    │   └── phase0
    └── readme.md

Here are the brief explanation for each folder:
 - **apps**
	 - the source code of the software that parses and display the ebooks are stored under here.
	 - one of the readily available software out there is [koreader](https://github.com/koreader/koreader)
		 - koreader seems to be complicated to port to another device. Porting koreader is something that I have in mind for a while. might be better to revisit this effort later
	 - other ideas that came up is to make it from scratch, let's called it **bacaBuku**, using :
		 - [lvgl](https://lvgl.io/) as the graphic display.
			 - lvgl seem to have a good support for touch screen, might be useful for further development down the line.
		 - [mupdf](https://mupdf.com/) as the ebook parser
		 - Using C++17 (because why not, I love C++)
 - **boards**
	 - all the schematic, design, BoM, for variety of boards design will be placed here
	 - basically, this is where if anyone want's to replicate the hardware, they go through here.
	 - currently it has two types of boards (development-kit-a, and development-kit-b) but will be changed soon.
 - **docs**
	 - docs will hosts all the documentation including images for the interweb, this log documentation, all-in-one document to build the device from scratch, etc.
 - **drivers**
	 - any type of driver that are used in the final device will be placed here.
	 - that include, kernel-space driver for the IT8951 chip, a touch-screen device (maybe?), user-space driver for GPIO, etc
 - **experimental**
	 - every experiment, things to learn, basic knowledge, foundational concept, etc will be placed here

## Future Boards
For the foreseeable future the plan is to produce 2 types of board, namely, **protoBuku** and **gedeBuku**:

### protoBuku
As the name implies this is more like a PoC to show that this idea works. This board will also be used for the early development of the project. The idea of this board is also to show that anyone with basic electronics knowledge can build the device. The board will consist of:
	
 - Raspberry Pi Zero W
	 - main processor
	 - it's cheap, beginner friendly, easy to debug and community support is great
 - [6" E-Paper with IT8951 Controller](https://www.waveshare.com/product/displays/e-paper/6inch-e-paper-hat.htm)
	 - cheapest e-paper than can do <1 s refresh rate and uses the IT8951 controller
 - 6 Tactile Button for Up, Down, Left, Right, Next/Enter and Back/Cancel button.
 - Made out of 150mm * 150mm **perfboard**
	 - will probably be cut down to certain size
 - [PowerBoost 1000 Charger](https://www.adafruit.com/product/2465)
	 - this is for the charing with load-balance
 - [USB C Breakout](https://www.adafruit.com/product/4090)
	 - because USB Micro B is so last decade ago
	 - an Idea is to also hook this to the Pi Zero USB OTG line, that way to copy data back and forth (for ebooks and code) can be though this port
	 - So this port when plugged, will charged the Device through the PowerBoost 1000 Charger and also access the Pi Zero through the USB OTG
 - [ADS1115 16-Bit ADC](https://www.adafruit.com/product/1085)
	 - smallest ADC chip
	 - only used for sensing battery level

### gedeBuku
This board is the final form of the project for the foreseeable future. The board will be design in KiCAD and fabricated either from JLCPCB, PCBWay or other PCB fabricator company. This mean everything will be hand-soldered down. The board will consist of:

- Toradex's Computer on Module
	- the two choices are:
		- [i.MX 7 Dual](https://www.toradex.com/computer-on-modules/colibri-arm-family/nxp-freescale-imx7)
			- it has the EPD controller. for the far future might be better to use that instead of the IT8951 controller
		- [i.MX 6ULL](https://www.toradex.com/computer-on-modules/colibri-arm-family/nxp-imx6ull)
			- because it's very power efficient
- [10.3" E-Paper with IT8951 Controller](https://www.waveshare.com/10.3inch-e-Paper-HAT-D.htm)
	- biggest e-paper that has <1s refresh rate
- EPD Driver base on Waveshare IT8951
	- another challenge here is to figure out how to clone the IT8951 external flash to another board.
	- from initial research, the flash holds the firmware including the LUT and stuff
- Charging and Load-Balance circuit based on [PowerBoost 1000 Charger](https://www.adafruit.com/product/2465)
- USB C for charging and data access
- 10.4 Resistive Touch Panel using [STMPE610](https://www.adafruit.com/product/1571) Controller
	- later can be changed to a capacitive touch panel
	