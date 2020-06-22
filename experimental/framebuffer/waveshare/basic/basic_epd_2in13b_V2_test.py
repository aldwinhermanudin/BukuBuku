#!/usr/bin/python3

import logging
import sys
import os
import time
from PIL import Image,ImageDraw,ImageFont
import traceback
import spidev
import RPi.GPIO


class IODriver:
    # Pin definition
    RST_PIN         = 17
    DC_PIN          = 25
    CS_PIN          = 8
    BUSY_PIN        = 24

    def __init__(self):
        self.GPIO = RPi.GPIO

        # SPI device, bus = 0, device = 0
        self.SPI = spidev.SpiDev(0, 0)

    def digital_write(self, pin, value):
        self.GPIO.output(pin, value)

    def digital_read(self, pin):
        return self.GPIO.input(pin)

    def delay_ms(self, delaytime):
        time.sleep(delaytime / 1000.0)

    def spi_writebyte(self, data):
        self.SPI.writebytes(data)

    def module_init(self):
        self.GPIO.setmode(self.GPIO.BCM)
        self.GPIO.setwarnings(False)
        self.GPIO.setup(self.RST_PIN, self.GPIO.OUT)
        self.GPIO.setup(self.DC_PIN, self.GPIO.OUT)
        self.GPIO.setup(self.CS_PIN, self.GPIO.OUT)
        self.GPIO.setup(self.BUSY_PIN, self.GPIO.IN)
        self.SPI.max_speed_hz = 4000000
        self.SPI.mode = 0b00
        return 0

    def module_exit(self):
        logging.debug("spi end")
        self.SPI.close()

        logging.debug("close 5V, Module enters 0 power consumption ...")
        self.GPIO.output(self.RST_PIN, 0)
        self.GPIO.output(self.DC_PIN, 0)

        self.GPIO.cleanup()


iodriver = IODriver()
# Display resolution
EPD_WIDTH       = 104
EPD_HEIGHT      = 212

class EPDDriver:
    def __init__(self):
        self.reset_pin = iodriver.RST_PIN
        self.dc_pin = iodriver.DC_PIN
        self.busy_pin = iodriver.BUSY_PIN
        self.cs_pin = iodriver.CS_PIN
        self.width = EPD_WIDTH
        self.height = EPD_HEIGHT

    # Hardware reset
    def reset(self):
        iodriver.digital_write(self.reset_pin, 1)
        iodriver.delay_ms(200) 
        iodriver.digital_write(self.reset_pin, 0)
        iodriver.delay_ms(10)
        iodriver.digital_write(self.reset_pin, 1)
        iodriver.delay_ms(200)   

    # if DC = LOW means sending command
    def send_command(self, command):
        iodriver.digital_write(self.dc_pin, 0) # set DC to HIGH
        iodriver.digital_write(self.cs_pin, 0) # set CS to HIGH to stop sending data to SPI
        iodriver.spi_writebyte([command])
        iodriver.digital_write(self.cs_pin, 1)


    # if DC = HIGH means sending data
    def send_data(self, data):
        iodriver.digital_write(self.dc_pin, 1) # set DC to HIGH
        iodriver.digital_write(self.cs_pin, 0) # set CS to LOW to start sending data to SPI
        iodriver.spi_writebyte([data])
        iodriver.digital_write(self.cs_pin, 1) # set CS to HIGH to stop sending data to SPI
        
    def ReadBusy(self):
        logging.debug("e-Paper busy")
        self.send_command(0x71);
        while(iodriver.digital_read(self.busy_pin) == 0): 
            self.send_command(0x71);
            iodriver.delay_ms(100)
        logging.debug("e-Paper busy release")

    def init(self):
        if (iodriver.module_init() != 0):
            return -1
            
        self.reset()
        self.send_command(0x04);  
        self.ReadBusy();#waiting for the electronic paper IC to release the idle signal

        self.send_command(0x00);    #panel setting
        self.send_data(0x0f);   #LUT from OTP,128x296
        self.send_data(0x89);    #Temperature sensor, boost and other related timing settings

        self.send_command(0x61);    #resolution setting
        self.send_data (0x68);  
        self.send_data (0x00);  
        self.send_data (0xD4);

        self.send_command(0X50);    #VCOM AND DATA INTERVAL SETTING
        self.send_data(0x77);   #WBmode:VBDF 17|D7 VBDW 97 VBDB 57
                            # WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
        
        return 0

    # this convert the 2D Image to array ( "frame buffer" )
    def getbuffer(self, image):
        logging.info( "bufsiz =  {a}".format(a=(int(self.width/8) * self.height)) )
        buf = [0xFF] * (int(self.width/8) * self.height)
        image_monocolor = image.convert('1')
        imwidth, imheight = image_monocolor.size
        pixels = image_monocolor.load()
        logging.debug("imwidth = %d, imheight = %d",imwidth,imheight)
        if(imwidth == self.width and imheight == self.height):
            logging.debug("Vertical")
            for y in range(imheight):
                for x in range(imwidth):
                    # Set the bits for the column of pixels at the current position.
                    if pixels[x, y] == 0:
                        buf[int((x + y * self.width) / 8)] &= ~(0x80 >> (x % 8))
        elif(imwidth == self.height and imheight == self.width):
            logging.debug("Horizontal")
            for y in range(imheight):
                for x in range(imwidth):
                    newx = y
                    newy = self.height - x - 1
                    if pixels[x, y] == 0:
                        buf[int((newx + newy*self.width) / 8)] &= ~(0x80 >> (y % 8))
        return buf

    # send the data to ePaper
    def display(self, imageblack, imagered):

        self.send_command(0x10) # 0x10 is command to start sending to the black ink
        for i in range(0, int(self.width * self.height / 8)):
            self.send_data(imageblack[i])
        
        self.send_command(0x13) # 0x13 is command to start sending to the red ink
        for i in range(0, int(self.width * self.height / 8)):
            self.send_data(imagered[i])
        
        self.send_command(0x12) # REFRESH
        iodriver.delay_ms(100)
        self.ReadBusy()

    # send the data to ePaper
    def display_black(self, imageblack):
        self.send_command(0x10) # 0x10 is command to start sending to the black ink
        for i in range(0, int(self.width * self.height / 8)):
            self.send_data(imageblack[i])
        
        self.send_command(0x12) # REFRESH
        iodriver.delay_ms(100)
        self.ReadBusy()
        
    # send the data to ePaper
    def display_red(self, imagered):
        self.send_command(0x13) # 0x13 is command to start sending to the red ink
        for i in range(0, int(self.width * self.height / 8)):
            self.send_data(imagered[i])
        
        self.send_command(0x12) # REFRESH
        iodriver.delay_ms(100)
        self.ReadBusy()
        
    # clear the screen
    def Clear(self):
        self.send_command(0x10)
        for i in range(0, int(self.width * self.height / 8)):
            self.send_data(0xFF)
        
        self.send_command(0x13)
        for i in range(0, int(self.width * self.height / 8)):
            self.send_data(0xFF)
        
        self.send_command(0x12) # REFRESH
        iodriver.delay_ms(100)
        self.ReadBusy()

    def sleep(self):
        self.send_command(0X50) 
        self.send_data(0xf7)
        self.send_command(0X02) 
        self.ReadBusy()
        self.send_command(0x07) # DEEP_SLEEP
        self.send_data(0xA5) # check code
        
        iodriver.module_exit()


logging.basicConfig(level=logging.DEBUG)

try:
    logging.info("epd2in13b_V2 Demo")
    
    epd = EPDDriver()
    logging.info("init and Clear")
    epd.init()
    epd.Clear()
    time.sleep(1)
    
    # Drawing on the image
    logging.info("Drawing")    
    font20 = ImageFont.truetype('Font.ttc', 20)
    
    # This is to draw black and red Horizontally
    # logging.info("1.Drawing on the Horizontal image...") 
    # HBlackimage = Image.new('1', (epd.height, epd.width), 255)  # 298*126
    # HRYimage = Image.new('1', (epd.height, epd.width), 255)  # 298*126  ryimage: red or yellow image  
    # drawblack = ImageDraw.Draw(HBlackimage)
    # drawry = ImageDraw.Draw(HRYimage)
    # drawry.text((10, 0), 'hello world', font = font20, fill = 0)
    # drawblack.text((10, 20), '2.13inch e-Paper bc', font = font20, fill = 0)
    # drawblack.line((70, 50, 20, 100), fill = 0)
    # epd.display(epd.getbuffer(HBlackimage), epd.getbuffer(HRYimage))
    # epd.display_red(epd.getbuffer(HRYimage))
    # time.sleep(2)
    # epd.Clear()

    # This is to draw red horizontally
    # LRYimage = Image.new('1', (epd.width, epd.height), 255)  # 126*298
    # drawry = ImageDraw.Draw(LRYimage)
    # drawry.text((2, 0), 'hello world', font = font20, fill = 0)
    # epd.display_red(epd.getbuffer(LRYimage))
    # time.sleep(2)
    # epd.Clear()


    '''
        currently only support full refresh
        #####################################
        #(n-5)                      (4)  (0)#
        #(n-4)                      (5)  (1)#
        #(n-3)                      (6)  (2)#
        #(n-2)                      (7)  (3)#
        #(n-1)                      (8) (..)#
        # (n)                       (9) (13)#
        #####################################
        width  = 104
        height = 212
        buf[0] --> this controls pixel 1 to pixel 8
        buf[0] = 0b01111111 <-- this set pixel 1
        buf[0] = 0b00111111 <-- this set pixel 1 and 2
        bit 0 means setting that pixel to black (or red if data is sent to red ink register)
        
        n = (width/8) * height
        n = (104/8) * 212
        n = 2756
    '''
    buf = [0xFF] * (int(EPD_WIDTH/8) * EPD_HEIGHT)
    # buf[0] = 0b01111111 # setting 1 pixel
    # setting first 104 pixel as red
    for i in range(0,13):
        print ("current buffer: " + str(i))
        buf[i] = 0x0
    epd.display_red(buf)
    time.sleep(2)

    logging.info("Goto Sleep...")
    epd.sleep()
        
except IOError as e:
    logging.info(e)
    
except KeyboardInterrupt:    
    logging.info("ctrl + c:")
    iodriver.module_exit()
    exit()
