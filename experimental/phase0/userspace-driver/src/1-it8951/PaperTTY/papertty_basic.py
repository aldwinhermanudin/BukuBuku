#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Colin Nolan, 2020
# Jouko Strömmer, 2018
# Copyright and related rights waived via CC0
# https://creativecommons.org/publicdomain/zero/1.0/legalcode

# As you would expect, use this at your own risk! This code was created
# so you (yes, YOU!) can make it better.
#
# Requires Python 3

# display drivers - note: they are GPL licensed, unlike this file
import drivers.drivers_base as drivers_base
import drivers.drivers_partial as drivers_partial
import drivers.drivers_full as drivers_full
import drivers.drivers_color as drivers_color
import drivers.drivers_colordraw as drivers_colordraw
import drivers.driver_it8951 as driver_it8951
import drivers.drivers_4in2 as driver_4in2

# for ioctl
import fcntl
# for validating type of and access to device files
import os
# for gracefully handling signals (systemd service)
import signal
# for unpacking virtual console data
import struct
# for stdin and exit
import sys
import select
# for sleeping
import time
# for drawing
from PIL import Image, ImageChops, ImageDraw, ImageFont, ImageOps
# for reading stdin data for use with Pillow
from io import BytesIO

def get_drivers():
    """Get the list of available drivers as a dict
    Format: { '<NAME>': { 'desc': '<DESCRIPTION>', 'class': <CLASS> }, ... }"""
    driverdict = {}
    driverlist = [drivers_partial.EPD1in54, drivers_partial.EPD2in13,
                  drivers_partial.EPD2in13v2, drivers_partial.EPD2in9,
                  drivers_partial.EPD2in13d, driver_4in2.EPD4in2,

                  drivers_full.EPD2in7, drivers_full.EPD7in5,
                  drivers_full.EPD7in5v2,

                  drivers_color.EPD4in2b, drivers_color.EPD7in5b,
                  drivers_color.EPD5in83, drivers_color.EPD5in83b,

                  drivers_colordraw.EPD1in54b, drivers_colordraw.EPD1in54c,
                  drivers_colordraw.EPD2in13b, drivers_colordraw.EPD2in7b,
                  drivers_colordraw.EPD2in9b,

                  driver_it8951.IT8951,

                  drivers_base.Dummy, drivers_base.Bitmap]
    for driver in driverlist:
        driverdict[driver.__name__] = {'desc': driver.__doc__, 'class': driver}
    return driverdict

class PaperTTY:
    """The main class - handles various settings and showing text on the display"""
    driver = None
    initialized = None
    partial = None

    def __init__(self, driver):
        """Create a PaperTTY with the chosen driver and settings"""
        self.driver = get_drivers()[driver]['class']()

    def ready(self):
        """Check that the driver is loaded and initialized"""
        return self.driver and self.initialized

    def init_display(self):
        """Initialize the display - call the driver's init method"""
        self.driver.init(partial=self.partial)
        self.initialized = True
        
    def clear(self):
        """Clears the display; set all black, then all white, or use INIT mode, if driver supports it."""
        if self.ready():
            self.driver.clear()
            print('Display reinitialized.')
        else:
            self.error("Display not ready")

def display_image(driver, image, stretch=False, no_resize=False, fill_color="white", rotate=None, mirror=None, flip=None):
    """
    Display the given image using the given driver and options.
    :param driver: device driver (subclass of `WaveshareEPD`)
    :param image: image data to display
    :param stretch: whether to stretch the image so that it fills the screen in both dimentions
    :param no_resize: whether the image should not be resized if it does not fit the screen (will raise `RuntimeError`
    if image is too large)
    :param fill_color: colour to fill space when image is resized but one dimension does not fill the screen
    :param rotate: rotate the image by arbitrary degrees
    :param mirror: flip the image horizontally
    :param flip: flip the image vertically
    :return: the image that was rendered
    """
    if stretch and no_resize:
        raise ValueError('Cannot set "no-resize" with "stretch"')

    if mirror:
        image = ImageOps.mirror(image)
    if flip:
        image = ImageOps.flip(image)
    if rotate:
        image = image.rotate(rotate, expand=True, fillcolor=fill_color)
        
    image_width, image_height = image.size

    if stretch:
        if (image_width, image_height) == (driver.width, driver.height):
            output_image = image
        else:
            output_image = image.resize((driver.width, driver.height))
    else:
        if no_resize:
            if image_width > driver.width or image_height > driver.height:
                raise RuntimeError('Image ({0}x{1}) needs to be resized to fit the screen ({2}x{3})'
                                   .format(image_width, image_height, driver.width, driver.height))
            # Pad only
            output_image = Image.new(image.mode, (driver.width, driver.height), color=fill_color)
            output_image.paste(image, (0, 0))
        else:
            # Scales and pads
            # add empty area with fill_color if image is not the same size as driver's screen size
            output_image = ImageOps.pad(image, (driver.width, driver.height), color=fill_color)

    driver.draw(0, 0, output_image)

    return output_image

if __name__ == '__main__':

    controller = PaperTTY(driver="IT8951")
    controller.init_display()

    # image_location = "04.bmp"
    image_location = "test_pano.jpg"
    image = None
    """ Display an image """
    if image_location is None or image_location == '-':
        # XXX: logging to stdout, in line with the rest of this project
        print('Reading image data from stdin... (set "--image" to load an image from a given file path)')
        image_data = BytesIO(sys.stdin.buffer.read())
        image = Image.open(image_data)
    else:
        image = Image.open(image_location)
    display_image(controller.driver, image)