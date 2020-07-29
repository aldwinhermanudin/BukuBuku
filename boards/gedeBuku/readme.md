# gedeBuku

This board is the final form of the project for the foreseeable future. The board will be designed in KiCAD and fabricated either from JLCPCB, PCBWay or other PCB fabricator company. This mean everything will be hand-soldered. The board will consist of:

- Toradex's Computer on Module
  - the two choices are:
    - [i.MX 7 Dual](https://www.toradex.com/computer-on-modules/colibri-arm-family/nxp-freescale-imx7)
      - it has the EPD controller. for the far future might be better to use that instead of the IT8951 controller
    - [i.MX 6ULL](https://www.toradex.com/computer-on-modules/colibri-arm-family/nxp-imx6ull)
       -   because it's very power efficient
- [10.3" E-Paper with IT8951 Controller](https://www.waveshare.com/10.3inch-e-Paper-HAT-D.htm)
  - biggest e-paper that has <1s refresh rate
- EPD Driver base on Waveshare IT8951
  - another challenge here is to figure out how to clone the IT8951 external flash to another board.
  - from initial research, the flash holds the firmware including the LUT and stuff
-  Charging and Load-Balance circuit based on  [PowerBoost 1000 Charger](https://www.adafruit.com/product/2465)
-  USB C for charging and data access
-  10.4 Resistive Touch Panel using  [STMPE610](https://www.adafruit.com/product/1571)  Controller
  - later can be changed to a capacitive touch panel