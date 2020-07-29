# protoBuku

As the name implies this is more like a PoC to show that this idea works. This board will also be used for the early development of the project. The idea of this board is also to show that anyone with basic electronics knowledge can build the device. The board consist of:

- Raspberry Pi Zero W
  - main processor
  - it's cheap, beginner friendly, easy to debug and community support is great
- [6" E-Paper with IT8951 Controller](https://www.waveshare.com/product/displays/e-paper/6inch-e-paper-hat.htm)
  - cheapest e-paper than can do <1 s refresh rate and uses the IT8951 controller
- 6 Tactile Button for Up, Down, Left, Right, Next/Enter and Back/Cancel button.
- Made out of 150mm * 150mm  **perfboard**
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
