# Wareshare Eink Frame Buffer driver

This is a Frame Buffer driver for the Waveshare 7.5" E-Ink
display. The idea was to have a low-power low-cost Linux console. The
driver was built for the Raspeberry Pi (RPi) Linux kernel.

## Important

### Not for continuous use

The [datasheet](https://www.waveshare.com/wiki/7.5inch_e-Paper_HAT)
for the display says the device should be put into low power mode when
not in use. Continuously applying power may damage the device. Power
functionality has not yet been implement and so these drivers are
not yet ready for continous use.

### No partial refresh

According to the [Waveshare
website](https://www.waveshare.com/7.5inch-e-Paper-HAT.htm) the device
does not support partial refresh. This means the device is incredibly
slow for the console. Unusable even. If someone knows how to enable
parital refresh, please [contact me](mailto:ray@react0r.com).

## Installation

1. Get Raspberry Pi Linux source by following the
[instructions](https://www.raspberrypi.org/documentation/linux/kernel/building.md)

2. Copy the driver in with other Frame Buffer drivers:
```
cp wseinkfb.c linux/drivers/video/fbdev/
```

3. Copy the device tree overlay into its build directory:
```
cp wseink-overlay.dts linux/arch/arm/boot/dts/overlays/
```

4. Apply patches to build process:
```
cd linux/
patch ../driver/build-wseink.patch
```

5. Build the kernel. See Raspberry Pi build instructions (linked
above) for details.

## Configuration

### Enabling the Frame Buffer console

On the RPi, the driver is loaded with the SPI bus after `init` has
been called. If no other console is available, the Waveshare device
will likely be used (see `Documenation/fb/fbcon.txt` in the Linux
source for more details). If not, it can enabled with the
`fbcon=map:01` kernel parameter or on the command line with `con2fbmap
2 1`. The former maps the `/dev/fb0` and `dev/fb1` pattern of frame
buffers on to the /dev/ttyN terminals.

### Disable blinking cursor

The cursor will cause the screen to continuously refresh. To avoid
this, disable it with:

```
echo 0 > /sys/class/graphics/fbcon/cursor_blink
```
