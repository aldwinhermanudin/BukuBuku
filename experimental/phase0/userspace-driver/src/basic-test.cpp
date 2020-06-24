// spi.c
//
// Example program for bcm2835 library
// Shows how to interface with SPI to transfer a byte to and from an SPI device
//
// After installing bcm2835, you can build this 
// with something like:
// gcc -o spi spi.c -l bcm2835
// sudo ./spi
//
// Or you can test it before installing with:
// gcc -o spi -I ../../src ../../src/bcm2835.c spi.c
// sudo ./spi
//
// Author: Mike McCauley
// Copyright (C) 2012 Mike McCauley
// $Id: RF22.h,v 1.21 2012/05/30 01:51:25 mikem Exp $
 
#include <bcm2835.h>
#include <iostream>
#include <vector>
#include <unistd.h>

using Pin = uint8_t;
using Millisecond = unsigned int;
using Byte = uint8_t;
using Pixel = uint32_t;
using Hertz = uint32_t;

enum class Ink : uint8_t {
  BLACK = 0,
  RED = 1
};


class Driver {    
    public:
        static const Pixel kWidth = 104;
        static const Pixel kHeight = 212;
        static const Pixel kTotalPixel = (kWidth/8) * kHeight;
        Driver() = default;
        void Reset();
        void SendCommand(Byte data_out);
        void SendData(Byte data_out);
        void ReadBusy();
        void Display(Ink type, const std::vector<Byte> image);
        void Clear();
        bool InitializeModule();
        void DeinitializeModule();

    private:
        static const Pin kRstPin = RPI_BPLUS_GPIO_J8_11;
        static const Pin kDcPin = RPI_BPLUS_GPIO_J8_22;
        // static const Pin kCsPin = 24; // spi comm handled by the bcm2835 library
        static const Pin kBusyPin = RPI_BPLUS_GPIO_J8_18;
        void DigitalWrite(Pin number, bool value);
        bool DigitalRead(Pin number);
        void Delay(Millisecond delay);
        void SpiWriteByte(Byte data_out);
};

void Driver::Reset(){
  DigitalWrite(kRstPin, HIGH);
  Delay(200);
  DigitalWrite(kRstPin, LOW);
  Delay(10);
  DigitalWrite(kRstPin, HIGH);
  Delay(200); 
}

void Driver::SendCommand(Byte data_out){
  DigitalWrite(kDcPin, LOW); // per e-paper datasheet, set dc pin to LOW to send command
  // DigitalWrite(kCsPin,0); // spi comm handled by the bcm2835 library
  SpiWriteByte(data_out);
  // DigitalWrite(kCsPin,1); // spi comm handled by the bcm2835 library
}

void Driver::SendData(Byte data_out){
  DigitalWrite(kDcPin, HIGH);
  // DigitalWrite(kCsPin,0); // spi comm handled by the bcm2835 library
  SpiWriteByte(data_out);
  // DigitalWrite(kCsPin,1); // spi comm handled by the bcm2835 library
}

void Driver::ReadBusy(){
  std::cout << "e-Paper busy" << std::endl;
  SendCommand(0x71);
  while(DigitalRead(kBusyPin) == LOW){
    SendCommand(0x71);
    Delay(100);
  }
  std::cout << "e-Paper busy release" << std::endl;
}


void Driver::Display(Ink type, const std::vector<Byte> image){

  if ( type == Ink::BLACK ){
    SendCommand(0x10); // 0x10 is command to start sending to the black ink
    for (int i = 0; i < kTotalPixel; i++){
      SendData(image[i]);
    }
  } else if ( type == Ink::RED ){
    SendCommand(0x13); // 0x10 is command to start sending to the red ink
    for (int i = 0; i < kTotalPixel; i++){
      SendData(image[i]);
    }
  }
  SendCommand(0x12); // REFRESH
  Delay(100);
  ReadBusy();
}

void Driver::Clear(){
  SendCommand(0x10);
  for( unsigned int i = 0; i < kTotalPixel; i++ ) SendData(0xff);

  SendCommand(0x13);
  for( unsigned int i = 0; i < kTotalPixel; i++ ) SendData(0xff);

  SendCommand(0x12); // refresh the display
  Delay(100);
  ReadBusy();
}

void Driver::DigitalWrite(Pin number, bool value){
  bcm2835_gpio_write(number, value);
}

bool Driver::DigitalRead(Pin number){
  return bcm2835_gpio_lev(number);
}

void Driver::Delay(Millisecond delay){
  bcm2835_delay(delay);
}

void Driver::SpiWriteByte(Byte data_out){
  bcm2835_spi_writenb((char*)&data_out, 1); // write 1 byte out
}

bool Driver::InitializeModule(){

  if (!bcm2835_init()){
    std::cout << "bcm2835_init failed. Are you running as root??" << std::endl;
    return false;
  }
  if (!bcm2835_spi_begin()){
    std::cout << "bcm2835_spi_begin failed. Are you running as root??" << std::endl;
    return false;
  }
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
  bcm2835_spi_set_speed_hz(1*1000*1000);
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default

  // Set the kRstPin,kDcPin to be an output
  bcm2835_gpio_fsel(kRstPin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(kDcPin, BCM2835_GPIO_FSEL_OUTP);
  // bcm2835_gpio_fsel(kCsPin, BCM2835_GPIO_FSEL_OUTP); // spi comm handled by the bcm2835 library
  
  // Set RPI pin kBusyPin to be an input
  bcm2835_gpio_fsel(kBusyPin, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_set_pud(kBusyPin, BCM2835_GPIO_PUD_UP); //  with a pullup

  Reset();
  SendCommand(0x04);
  ReadBusy(); //waiting for the electronic paper IC to release the idle signal

  SendCommand(0x00); // panel setting
  SendData(0x0f);   // LUT from OTP,128x296
  SendData(0x89);   // Temperature sensor, boost and other related timing settings
  
  SendCommand(0x61);    // resolution setting
  SendData(0x68);
  SendData(0x00);
  SendData(0xD4);
  
  SendCommand(0x50);    // VCOM AND DATA INTERVAL SETTING
  SendData(0x77);       // WBmode:VBDF 17|D7 VBDW 97 VBDB 57
                        // WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7

  return true;
}


void Driver::DeinitializeModule(){

  SendCommand(0x50);
  SendData(0xf7);
  SendCommand(0x02);
  ReadBusy();
  SendCommand(0x07); // deep sleep
  SendData(0xA5); // check code

  std::cout << "spi end" << std::endl;
  bcm2835_spi_end();
  std::cout << "close 5V, Module enters 0 power consumption ..." << std::endl;
  DigitalWrite(kRstPin, LOW);
  DigitalWrite(kDcPin, LOW);
  bcm2835_close();
}

int main(int argc, char **argv){
    Driver epd;
    std::cout << "init" << std::endl;
    if (!epd.InitializeModule()){
      return 1;
    }

    std::cout << "clear" << std::endl;
    epd.Clear();

    usleep(1*1000*1000);

    /*
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
    */

    // this set all screen to white.
    std::vector<Byte> buf(Driver::kTotalPixel, 0xff);
    //buf[0] = 0b01111111 // setting 1 pixel
    // setting first 104 pixel as red
    for (int i = 0 ; i < 13; i++){
        buf[i] = 0x0;
    }

    epd.Display(Ink::RED, buf);
    usleep(2*1000*1000);

    std::cout << "sleep" << std::endl;
    epd.DeinitializeModule();

    return 0;
}
 