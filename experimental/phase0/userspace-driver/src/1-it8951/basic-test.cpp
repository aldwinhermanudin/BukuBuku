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

/* Command and Register Constant */
#define DUMMY_DATA                0x0000
#define PREAMBLE_WRITE_CMD        0x6000
#define PREAMBLE_SPI_READ         0x1000

//Built in I80 Command Code
#define IT8951_TCON_SYS_RUN       0x0001
#define IT8951_TCON_STANDBY       0x0002
#define IT8951_TCON_SLEEP         0x0003
#define IT8951_TCON_REG_RD        0x0010
#define IT8951_TCON_REG_WR        0x0011
#define IT8951_TCON_MEM_BST_RD_T  0x0012
#define IT8951_TCON_MEM_BST_RD_S  0x0013
#define IT8951_TCON_MEM_BST_WR    0x0014
#define IT8951_TCON_MEM_BST_END   0x0015
#define IT8951_TCON_LD_IMG        0x0020
#define IT8951_TCON_LD_IMG_AREA   0x0021
#define IT8951_TCON_LD_IMG_END    0x0022

//I80 User defined command code
#define USDEF_I80_CMD_DPY_AREA      0x0034
#define USDEF_I80_CMD_GET_DEV_INFO  0x0302
#define USDEF_I80_CMD_DPY_BUF_AREA  0x0037
#define USDEF_I80_CMD_VCOM		      0x0039

//Panel
#define IT8951_PANEL_WIDTH   1024 //it Get Device information
#define IT8951_PANEL_HEIGHT   758

//Rotate mode
#define IT8951_ROTATE_0     0
#define IT8951_ROTATE_90    1
#define IT8951_ROTATE_180   2
#define IT8951_ROTATE_270   3

//Pixel mode , BPP - Bit per Pixel
#define IT8951_2BPP   0
#define IT8951_3BPP   1
#define IT8951_4BPP   2
#define IT8951_8BPP   3

//Waveform Mode
#define IT8951_MODE_0   0
#define IT8951_MODE_1   1
#define IT8951_MODE_2   2
#define IT8951_MODE_3   3
#define IT8951_MODE_4   4
//Endian Type
#define IT8951_LDIMG_L_ENDIAN   0
#define IT8951_LDIMG_B_ENDIAN   1
//Auto LUT
#define IT8951_DIS_AUTO_LUT   0
#define IT8951_EN_AUTO_LUT    1
//LUT Engine Status
#define IT8951_ALL_LUTE_BUSY 0xFFFF

//-----------------------------------------------------------------------
// IT8951 TCon Registers defines
//-----------------------------------------------------------------------
//Register Base Address
#define DISPLAY_REG_BASE 0x1000               //Register RW access for I80 only
//Base Address of Basic LUT Registers
#define LUT0EWHR  (DISPLAY_REG_BASE + 0x00)   //LUT0 Engine Width Height Reg
#define LUT0XYR   (DISPLAY_REG_BASE + 0x40)   //LUT0 XY Reg
#define LUT0BADDR (DISPLAY_REG_BASE + 0x80)   //LUT0 Base Address Reg
#define LUT0MFN   (DISPLAY_REG_BASE + 0xC0)   //LUT0 Mode and Frame number Reg
#define LUT01AF   (DISPLAY_REG_BASE + 0x114)  //LUT0 and LUT1 Active Flag Reg
//Update Parameter Setting Register
#define UP0SR (DISPLAY_REG_BASE + 0x134)      //Update Parameter0 Setting Reg

#define UP1SR     (DISPLAY_REG_BASE + 0x138)  //Update Parameter1 Setting Reg
#define LUT0ABFRV (DISPLAY_REG_BASE + 0x13C)  //LUT0 Alpha blend and Fill rectangle Value
#define UPBBADDR  (DISPLAY_REG_BASE + 0x17C)  //Update Buffer Base Address
#define LUT0IMXY  (DISPLAY_REG_BASE + 0x180)  //LUT0 Image buffer X/Y offset Reg
#define LUTAFSR   (DISPLAY_REG_BASE + 0x224)  //LUT Status Reg (status of All LUT Engines)

#define BGVR      (DISPLAY_REG_BASE + 0x250)  //Bitmap (1bpp) image color table
//-------System Registers----------------
#define SYS_REG_BASE 0x0000

//Address of System Registers
#define I80CPCR (SYS_REG_BASE + 0x04)
//-------Memory Converter Registers----------------
#define MCSR_BASE_ADDR 0x0200
#define MCSR (MCSR_BASE_ADDR  + 0x0000)
#define LISAR (MCSR_BASE_ADDR + 0x0008)
/* Command and Register Constant */

using Pin = uint8_t;
using Millisecond = unsigned int;
using Byte = uint8_t;
using Word = uint16_t;
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
        void WriteCommand(Word data_out);
        void SendCommand(Byte data_out);
        void SendData(Byte data_out);
        void ReadBusy();
        void Display(Ink type, const std::vector<Byte> image);
        void Clear();
        bool InitializeModule();
        void DeinitializeModule();

    private:
        // static const Pin kRstPin = RPI_BPLUS_GPIO_J8_11;
        static const Pin kDcPin = RPI_BPLUS_GPIO_J8_22;
        // static const Pin kCsPin = RPI_BPLUS_GPIO_J8_8; // spi comm handled by the bcm2835 library
        static const Pin kBusyPin = RPI_BPLUS_GPIO_J8_18;

        static const Pin kCsPin = 8; // spi comm handled by the bcm2835 library
        static const Pin kRstPin = 17;
        static const Pin kHrdyPin = 24;
        static const int kVCom = 1950;
        void SpiWrite(Word data_out);
        std::vector<Byte> SpiRead(unsigned int size);
        void DigitalWrite(Pin number, bool value);
        bool DigitalRead(Pin number);
        void Delay(Millisecond delay);
};

/* Waits for the busy pin to drop.
 *
 * When the busy pin is high the controller is busy and may drop any
 * commands that are sent to it.
 */
void Driver::ReadBusy(){
  std::cout << "e-Paper busy" << std::endl;
  while(DigitalRead(kBusyPin) == LOW){
    Delay(100);
  }
  std::cout << "e-Paper busy release" << std::endl;
}

void Driver::SpiWrite(Word data_out){
  Byte temp[2] = {0};
  temp[0] = (Byte) (data_out >> 8);     // extract the first byte
  temp[1] = (Byte) data_out;            // extract the second byte
  bcm2835_spi_writenb((char*)&temp, 2); // write 1 byte out
}

std::vector<Byte> Driver::SpiRead(unsigned int size){
  std::vector<Byte> ret;

  ReadBusy();
  DigitalWrite(kCsPin,LOW);
  SpiWrite(PREAMBLE_SPI_READ);
  ReadBusy();
  SpiWrite(DUMMY_DATA);
  ReadBusy();

	for(unsigned int i=0; i < size; i++){
    ret.push_back(bcm2835_spi_transfer(DUMMY_DATA));
	}

  DigitalWrite(kCsPin,HIGH);

  return ret;


	// uint32_t i;
	
	// uint16_t wPreamble = 0x1000;

	// LCDWaitForReady();
	
	// bcm2835_gpio_write(CS,LOW);

	// bcm2835_spi_transfer(wPreamble>>8);
	// bcm2835_spi_transfer(wPreamble);
	
	// LCDWaitForReady();
	
	// pwBuf[0]=bcm2835_spi_transfer(0x00);//dummy
	// pwBuf[0]=bcm2835_spi_transfer(0x00);//dummy
	
	// LCDWaitForReady();
	
	// for(i=0;i<ulSizeWordCnt;i++)
	// {
	// 	pwBuf[i] = bcm2835_spi_transfer(0x00)<<8;
	// 	pwBuf[i] |= bcm2835_spi_transfer(0x00);
	// }
	
	// bcm2835_gpio_write(CS,HIGH); 

}

void Driver::WriteCommand(Word data_out){
  ReadBusy();
  DigitalWrite(kCsPin,LOW); 
  SpiWrite(PREAMBLE_WRITE_CMD);
  ReadBusy();
  SpiWrite(data_out);
  DigitalWrite(kCsPin,HIGH);
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

void Driver::Reset(){
  DigitalWrite(kRstPin, LOW);
  Delay(500);
  DigitalWrite(kRstPin, HIGH);
  Delay(500);
}

void Driver::SendCommand(Byte data_out){
  DigitalWrite(kDcPin, LOW); // per e-paper datasheet, set dc pin to LOW to send command
  // DigitalWrite(kCsPin,0); // spi comm handled by the bcm2835 library
  // SpiWriteByte(data_out);
  // DigitalWrite(kCsPin,1); // spi comm handled by the bcm2835 library
}

void Driver::SendData(Byte data_out){
  DigitalWrite(kDcPin, HIGH);
  // DigitalWrite(kCsPin,0); // spi comm handled by the bcm2835 library
  // SpiWriteByte(data_out);
  // DigitalWrite(kCsPin,1); // spi comm handled by the bcm2835 library
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

bool Driver::InitializeModule(){

  // if (!bcm2835_init()){
  //   std::cout << "bcm2835_init failed. Are you running as root??" << std::endl;
  //   return false;
  // }
  // if (!bcm2835_spi_begin()){
  //   std::cout << "bcm2835_spi_begin failed. Are you running as root??" << std::endl;
  //   return false;
  // }
  // bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
  // bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
  // bcm2835_spi_set_speed_hz(1*1000*1000);
  // bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
  // bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default

  // // Set the kRstPin,kDcPin to be an output
  // bcm2835_gpio_fsel(kRstPin, BCM2835_GPIO_FSEL_OUTP);
  // bcm2835_gpio_fsel(kDcPin, BCM2835_GPIO_FSEL_OUTP);
  // // bcm2835_gpio_fsel(kCsPin, BCM2835_GPIO_FSEL_OUTP); // spi comm handled by the bcm2835 library
  
  // // Set RPI pin kBusyPin to be an input
  // bcm2835_gpio_fsel(kBusyPin, BCM2835_GPIO_FSEL_INPT);
  // bcm2835_gpio_set_pud(kBusyPin, BCM2835_GPIO_PUD_UP); //  with a pullup

  // Reset();
  // SendCommand(0x04);
  // ReadBusy(); //waiting for the electronic paper IC to release the idle signal

  // SendCommand(0x00); // panel setting
  // SendData(0x0f);   // LUT from OTP,128x296
  // SendData(0x89);   // Temperature sensor, boost and other related timing settings
  
  // SendCommand(0x61);    // resolution setting
  // SendData(0x68);
  // SendData(0x00);
  // SendData(0xD4);
  
  // SendCommand(0x50);    // VCOM AND DATA INTERVAL SETTING
  // SendData(0x77);       // WBmode:VBDF 17|D7 VBDW 97 VBDB 57
  //                       // WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7


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
  bcm2835_spi_set_speed_hz(2*1000*1000);                        // set to 1 MHz
  bcm2835_spi_chipSelect(BCM2835_SPI_CS_NONE);                  // Let the library control it manually

  // // Set the kRstPin,kDcPin to be an output
  bcm2835_gpio_fsel(kRstPin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(kCsPin, BCM2835_GPIO_FSEL_OUTP); // spi comm handled by the bcm2835 library

  // Set RPI pin kBusyPin to be an input
  bcm2835_gpio_fsel(kHrdyPin, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_set_pud(kHrdyPin, BCM2835_GPIO_PUD_UP); //  with a pullup

  DigitalWrite(kCsPin, HIGH);

  // Reset the device to its initial state.
  Reset();

  WriteCommand(USDEF_I80_CMD_GET_DEV_INFO);

  std::vector<Byte> dev_info = SpiRead(40);

  for (const auto& data : dev_info){
    std::cout << data << " | ";
  }
  std::cout << std::endl;

  return true;
}


void Driver::DeinitializeModule(){

  // SendCommand(0x50);
  // SendData(0xf7);
  // SendCommand(0x02);
  // ReadBusy();
  // SendCommand(0x07); // deep sleep
  // SendData(0xA5); // check code

  // std::cout << "spi end" << std::endl;
  // bcm2835_spi_end();
  // std::cout << "close 5V, Module enters 0 power consumption ..." << std::endl;
  // DigitalWrite(kRstPin, LOW);
  // // DigitalWrite(kDcPin, LOW);
  // bcm2835_close();
  
  std::cout << "spi end" << std::endl;
  bcm2835_spi_end();
  std::cout << "close 5V, Module enters 0 power consumption ..." << std::endl;
  DigitalWrite(kRstPin, LOW);
  // DigitalWrite(kDcPin, LOW);
  bcm2835_close();
}

int main(int argc, char **argv){
    // Driver epd;
    // std::cout << "init" << std::endl;
    // if (!epd.InitializeModule()){
    //   return 1;
    // }

    // std::cout << "clear" << std::endl;
    // epd.Clear();

    // usleep(1*1000*1000);

    // /*
    //     currently only support full refresh
    //     #####################################
    //     #(n-5)                      (4)  (0)#
    //     #(n-4)                      (5)  (1)#
    //     #(n-3)                      (6)  (2)#
    //     #(n-2)                      (7)  (3)#
    //     #(n-1)                      (8) (..)#
    //     # (n)                       (9) (13)#
    //     #####################################
    //     width  = 104
    //     height = 212
    //     buf[0] --> this controls pixel 1 to pixel 8
    //     buf[0] = 0b01111111 <-- this set pixel 1
    //     buf[0] = 0b00111111 <-- this set pixel 1 and 2
    //     bit 0 means setting that pixel to black (or red if data is sent to red ink register)
        
    //     n = (width/8) * height
    //     n = (104/8) * 212
    //     n = 2756
    // */

    // // this set all screen to white.
    // std::vector<Byte> buf(Driver::kTotalPixel, 0xff);
    // //buf[0] = 0b01111111 // setting 1 pixel
    // // setting first 104 pixel as red
    // for (int i = 0 ; i < 13; i++){
    //     buf[i] = 0x0;
    // }

    // epd.Display(Ink::RED, buf);
    // usleep(2*1000*1000);

    // std::cout << "sleep" << std::endl;
    // epd.DeinitializeModule();

    Driver epd;
    std::cout << "init" << std::endl;
    if (!epd.InitializeModule()){
      return 1;
    }
    
    usleep(1*1000*1000);

    std::cout << "sleep" << std::endl;
    epd.DeinitializeModule();

    return 0;
}
 