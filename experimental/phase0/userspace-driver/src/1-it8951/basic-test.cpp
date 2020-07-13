/* 
 * simple example to write to IT8951 e-paper display with screen size of 6inch 
 * and width = 800, height = 600
 * the inclusion of OpenCV library for image processing and manipulation causes
 * the overall program to run slow. in the future, should use a smaller and faster
 * library to process images, for quick PoC and readabilty OpenCV should be good
 * 
 * this code is based on PaperTTY library, there are some command that's missing
 * from the original waveshare's IT8951.c code, such as IT8951WaitForDisplayReady()
 * although, the advantage on using that is still unknown.
 * Driver::Clear() also is not implemented yet.
 * 
 */



#include <bcm2835.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <opencv2/opencv.hpp>

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
#define DISPLAY_UPDATE_MODE_GC16  2
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
using DoubleWord = uint32_t;
using Pixel = Word;
using Hertz = uint32_t;
using Pixels = std::vector<Pixel>;

class DeviceInfo {
  
  public:
    static constexpr size_t m_size = 40;
    Word  m_panel_width;          //16 Bytes
    Word  m_panel_height;         //16 Bytes
    Word  m_img_buf_addr_h;       //16 Bytes
    Word  m_img_buf_addr_l;       //16 Bytes
    DoubleWord  m_img_buf_addr;   //32 Bytes
    std::string  m_fw_version;  	//16 Bytes String
    std::string  m_lut_version; 	//16 Bytes String

    DeviceInfo() {};
    DeviceInfo(std::vector<Byte> raw) {
      if (raw.size() != m_size){
        throw std::logic_error("Vector size should be 40");
      }

      m_panel_width   = (raw[0] << 8) | raw[1];
      m_panel_height  = (raw[2] << 8) | raw[3];
      m_img_buf_addr_l = (raw[4] << 8) | raw[5];
      m_img_buf_addr_h = (raw[6] << 8) | raw[7];
      m_img_buf_addr   = m_img_buf_addr_l | (m_img_buf_addr_h << 16);

      int i = 8;
      std::vector<Word> temp_str(8,0);

      // parse firmware version
      for (auto& word : temp_str){
        word = (raw[i] << 8) | raw[i+1];
        i+=2;
      }
      m_fw_version = std::string((char*) temp_str.data(),16);

      // parse LUT version
      for (auto& word : temp_str){
        word = (raw[i] << 8) | raw[i+1];
        i+=2;
      }
      m_lut_version = std::string((char*) temp_str.data(),16);
    }

    void PrintInfo(){
      printf("Panel(W,H) = (%d,%d)\r\n",
              m_panel_width, m_panel_height );
      printf("Image Buffer Address = 0x%x\r\n",
              m_img_buf_addr);
              
      //Show Firmware and LUT Version
      printf("FW Version = %s\r\n", m_fw_version.c_str());
      printf("LUT Version = %s\r\n", m_lut_version.c_str());
    }
};

class Driver {    
    public:
        static const Pixel kWidth = 104;
        static const Pixel kHeight = 212;
        static const Pixel kTotalPixel = (kWidth/8) * kHeight;
        DeviceInfo m_dev_info;
        Driver() = default;
        void Reset();
        void WriteCommand(Word data_out);
        void WriteRegister(Word reg_addr, Word data_out);
        void WriteData(std::vector<Word> data_out);
        DeviceInfo GetDeviceInfo();
        Word GetVCom();
        void SetVCom(Word vcom);
        void Draw(Pixel x, Pixel y, const cv::Mat& image);
        std::vector<Word> ConvertPrepareImageBuffer(const cv::Mat& image);
        void DisplayArea(Pixel x, Pixel y, Pixel width, Pixel height, Byte display_mode);
        void ReadBusy();
        void WaitForDisplayReady();
        void Clear();
        bool InitializeModule();
        void DeinitializeModule();

    private:

        static const Pin kCsPin = 8; // spi comm handled by the bcm2835 library
        static const Pin kRstPin = 17;
        static const Pin kHrdyPin = 24;
        static const int kVCom = 1950;
        void SpiWrite(Word data_out);
        std::vector<Byte> ReadData(unsigned int size);
        Word ReadData();
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
  while(DigitalRead(kHrdyPin) == LOW){
    Delay(100);
  }
}

/*
  Waits for the display to be finished updating.

  It is possible for the controller to be ready for more commands but the
  display to still be refreshing. This will wait for the display to be
  stable.
*/
void Driver::WaitForDisplayReady(){
    // while self.read_register(self.REG_LUTAFSR) != 0:
    //     self.delay_ms(100)

}

void Driver::SpiWrite(Word data_out){
  Byte temp[2] = {0};
  temp[0] = (Byte) (data_out >> 8);     // extract the first byte
  temp[1] = (Byte) data_out;            // extract the second byte
  bcm2835_spi_writenb((char*)&temp, 2); // write 1 byte out
}

std::vector<Byte> Driver::ReadData(unsigned int size){
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
}

Word Driver::ReadData(){
  std::vector<Byte> temp = ReadData(2);
  Word ret = (temp[0] << 8) | temp[1];
  return ret;
}

void Driver::WriteCommand(Word data_out){
  ReadBusy();
  DigitalWrite(kCsPin,LOW); 
  SpiWrite(PREAMBLE_WRITE_CMD);
  ReadBusy();
  SpiWrite(data_out);
  DigitalWrite(kCsPin,HIGH);
}

void Driver::WriteData(std::vector<Word> data_out){

  ReadBusy();
  DigitalWrite(kCsPin,LOW);
  SpiWrite(DUMMY_DATA);
  ReadBusy();
  for (auto& data : data_out){
    SpiWrite(data);
  }
  DigitalWrite(kCsPin,HIGH);  
}

void Driver::WriteRegister(Word reg_addr, Word data_out){
  WriteCommand(IT8951_TCON_REG_WR);
  WriteData({reg_addr});
  WriteData({data_out});
}

DeviceInfo Driver::GetDeviceInfo(){
  DeviceInfo ret;
  WriteCommand(USDEF_I80_CMD_GET_DEV_INFO);
  try{
    ret = DeviceInfo(ReadData(40));
  } catch (const std::exception& e){
    std::cout << "what: " << e.what() << std::endl;
  }
  return ret;
}

Word Driver::GetVCom(){
  ReadBusy();
  WriteCommand(USDEF_I80_CMD_VCOM);
  WriteData({0x0000});
  return ReadData();
}

void Driver::SetVCom(Word vcom){
  WriteCommand(USDEF_I80_CMD_VCOM);
  WriteData({0x0001});
  WriteData({vcom});
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

std::vector<Word> Driver::ConvertPrepareImageBuffer(const cv::Mat& image){

  cv::Mat grey_image;

  // convert to grey
  cv::cvtColor(image, grey_image, cv::COLOR_BGR2GRAY);

  // lmabda to convert grey_image to array
  auto mat_to_vec = [](cv::Mat mat) -> std::vector<uchar> {
      std::vector<uchar> array;
      if (mat.isContinuous()) {
          array.assign(mat.data, mat.data + mat.total()*mat.channels());
      } else {
          for (int i = 0; i < mat.rows; ++i) {
              array.insert(array.end(), mat.ptr<uchar>(i), mat.ptr<uchar>(i)+mat.cols*mat.channels());
          }
      }
      return array;
  };

  std::vector<uchar> frame_buffer = mat_to_vec(grey_image);

  // convert original fb to packed fb according
  // to IT8951 protocol and print original fb buffer
  // this might be overflow, but for simplicity this shoudl be good
  std::vector<Word> packed_buffer;
  for (size_t i = 0; i < frame_buffer.size(); i+=4){
      // packed_buffer.push_back( (frame_buffer[i+2] >> 4) | 
      //                          (frame_buffer[i+3] & 0xF0) );

      // packed_buffer.push_back( (frame_buffer[i] >> 4) | 
      //                          (frame_buffer[i+1] & 0xF0) ); 

      // might be overcomplicating things, but for readability this is good
      Word buf_h  = (frame_buffer[i+2] >> 4) | (frame_buffer[i+3] & 0xF0);
      Word buf_l  =   (frame_buffer[i] >> 4) | (frame_buffer[i+1] & 0xF0);
      Word buf    =  ((buf_h << 8) & 0xFF00) | ( buf_l & 0x00FF ); 

      packed_buffer.push_back(buf);
  }

  return packed_buffer;
}

void Driver::Draw(Pixel x, Pixel y, const cv::Mat& image){
  // void Driver::Draw(Pixel x, Pixel y, Pixels image){
  Pixel width = image.size().width;
  Pixel height = image.size().height;

  // Set Image Buffer Base Address
  ReadBusy();
  WriteRegister(LISAR + 2, m_dev_info.m_img_buf_addr_h );
  WriteRegister(LISAR, m_dev_info.m_img_buf_addr_l);

  WriteCommand(IT8951_TCON_LD_IMG_AREA);
  WriteData({ 
              (Word)
                (IT8951_LDIMG_L_ENDIAN << 8 ) |
                (IT8951_4BPP << 4) |
                (IT8951_ROTATE_0)
            });
  WriteData({(Word) x});
  WriteData({(Word) y});
  WriteData({(Word) width});
  WriteData({(Word) height});

  WriteData(ConvertPrepareImageBuffer(image));

  WriteCommand(IT8951_TCON_LD_IMG_END);

  DisplayArea(x,y, width, height, DISPLAY_UPDATE_MODE_GC16);

}

void Driver::DisplayArea(Pixel x, Pixel y, Pixel width, Pixel height, Byte display_mode){

  WriteCommand(USDEF_I80_CMD_DPY_AREA);
  WriteData({x});
  WriteData({y});
  WriteData({width});
  WriteData({height});
  WriteData({display_mode});
}
void Driver::Clear(){
  
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

  // Get the Device Information. Width, Height, Image base addr, FW & LUT version
  m_dev_info = GetDeviceInfo();
  m_dev_info.PrintInfo();
  
  // Set to Enable I80 Packed mode.
  WriteRegister(I80CPCR, 0x0001);

  // everytime the display reset-ed, vcom will default to specific volts ( 2.60 in the current firmware )
  Word current_vcom = GetVCom();
  printf("Current VCOM = -%.02fV\n",(float)current_vcom/1000);
	if (kVCom != current_vcom)
	{
		SetVCom(kVCom);
		printf("VCOM = -%.02fV\n",(float)GetVCom()/1000);
	}

  return true;
}


void Driver::DeinitializeModule(){
  
  std::cout << "spi end" << std::endl;
  bcm2835_spi_end();
  std::cout << "close 5V, Module enters 0 power consumption ..." << std::endl;
  DigitalWrite(kRstPin, LOW);
  // DigitalWrite(kDcPin, LOW);
  bcm2835_close();
}

int main(int argc, char **argv){
    Driver epd;
    cv::Mat image;
    
    image = cv::imread("test_pano.jpg");
    // image = cv::imread("04.bmp");

    // Check for failure
    if (image.empty()){
        std::cout << "Could not open or find the image" << std::endl;
        return 1;
    }
    
    std::cout << "init" << std::endl;
    if (!epd.InitializeModule()){
      return 1;
    }

    // Waveshare 6inch e-paper screen size are width=800 and height=600
    std::pair<int,int> driver_w_h = std::make_pair(800,600);
    cv::Mat driver_fb(driver_w_h.second,driver_w_h.first, CV_8UC3, cv::Scalar(0,0,0));
    
    // copy the image to driver_fb, image size need to be =< driver_fb
    image.copyTo(driver_fb(cv::Rect(0,0,image.cols, image.rows)));
    image = driver_fb;

    epd.Draw(0,0,image);

    usleep(1*1000*1000);

    std::cout << "sleep" << std::endl;
    epd.DeinitializeModule();

    return 0;
}
 