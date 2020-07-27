#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/version.h>
#include <linux/fb.h>
#include <linux/spi/spi.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/rmap.h>
#include <linux/pagemap.h>
#include <linux/device.h>
#include <linux/bitrev.h>

#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>


#define ERR(...) pr_err("it8951: "__VA_ARGS__)
#define PRERROR(...) pr_err("it8951: "__VA_ARGS__)
#define PRINFO(...) printk(KERN_INFO "it8951: "__VA_ARGS__)

//Built in I80 Command Code
#define IT8951_TCON_SYS_RUN      0x0001
#define IT8951_TCON_STANDBY      0x0002
#define IT8951_TCON_SLEEP        0x0003
#define IT8951_TCON_REG_RD       0x0010
#define IT8951_TCON_REG_WR       0x0011
#define IT8951_TCON_MEM_BST_RD_T 0x0012
#define IT8951_TCON_MEM_BST_RD_S 0x0013
#define IT8951_TCON_MEM_BST_WR   0x0014
#define IT8951_TCON_MEM_BST_END  0x0015
#define IT8951_TCON_LD_IMG       0x0020
#define IT8951_TCON_LD_IMG_AREA  0x0021
#define IT8951_TCON_LD_IMG_END   0x0022

//I80 User defined command code
#define USDEF_I80_CMD_DPY_AREA     0x0034
#define USDEF_I80_CMD_DPY_BUF_AREA 0x0037
#define USDEF_I80_CMD_PWR_SEQ 	   0x0038
#define USDEF_I80_CMD_VCOM		   0x0039

#define USDEF_I80_CMD_GET_DEV_INFO 0x0302

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

#define IT8951_MODE_INIT  0
#define IT8951_MODE_DU    1
#define IT8951_MODE_GC16  2
#define IT8951_MODE_GL16  3
#define IT8951_MODE_GLR16 4
#define IT8951_MODE_GLD16 5
// the following mode does not seems to work ...
#define IT8951_MODE_A2    6
#define IT8951_MODE_DU4   7

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
#define MCSR (MCSR_BASE_ADDR + 0x0000)
#define LISAR (MCSR_BASE_ADDR + 0x0008)

#define IT8951_FB_SET_SCREEN_SLEEP		_IOW('M', 1, int8_t)
#define IT8951_FB_SET_SCREEN_STANDBY		_IOW('M', 2, int8_t)

// #define IT8951_DEBUG

struct it8951_load_img_info
{
	uint16_t endian_type; //little or Big Endian
	uint16_t pixel_format; //bpp
	uint16_t rotate; //Rotate mode
	uint32_t start_fb_addr; //Start address of source Frame buffer
	uint32_t img_buf_base_addr;//Base address of target image buffer

};

//structure prototype 2
struct it8951_area_img_info
{
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
};

struct it8951_dev_info
{
	uint16_t panel_w;
	uint16_t panel_h;
	uint16_t img_buf_addr_l;
	uint16_t img_buf_addr_h;
	uint16_t fw_version[8]; 	//16 Bytes String
	uint16_t lut_version[8]; 	//16 Bytes String
};

struct it8951_epd {
	struct spi_device *spi;

	struct gpio_desc *reset;
	struct gpio_desc *hrdy;

	struct it8951_dev_info dev_info;

	uint32_t img_buf_addr;

	uint8_t bpp;
	
	uint32_t vcom;
	bool running;

	bool little_endian;

	u8 *ssbuf;
};


static void it8951_wait_for_ready(struct it8951_epd *epd, int us)
{
	int waited = 0;
	while (!gpiod_get_value_cansleep(epd->hrdy) && waited < 1000000) {
		usleep_range(us, us * 2);
		waited += us;
	}
#ifdef IT8951_DEBUG
		printk(KERN_INFO "it8951: wait_for_ready %d\n",waited);
#endif
}

/* SPI data transfer */

static inline void it8951_memcpy_swab16(struct it8951_epd *epd, u16 *dst, u16 *src, size_t len)
{
	if (epd->little_endian) {
		int i;
		for (i = 0; i < len; i++) {
			*dst++ = swab16(*src++);
		}
	} else {
		memcpy(dst, src, len);
	}
}

static inline u16 it8951_swab16(struct it8951_epd *epd, u16 data) {
	if (epd->little_endian) {
		return swab16(data);
	} else {
		return data;
	}
}

static int it8951_spi_transfer(struct it8951_epd *epd, uint16_t preamble, bool dummy, const void *tx, void *rx, uint32_t len) {
	int speed_hz = epd->spi->max_speed_hz; // 12000000; // can't get it works at > 12Mhz
#ifdef IT8951_DEBUG
	int i;
#endif
	int ret;
	u8 *txbuf = NULL, *rxbuf = NULL;
	uint16_t spreamble = it8951_swab16(epd, preamble);

#ifdef IT8951_DEBUG
	if(tx)
		printk(KERN_INFO "it8951: it8951_spi_transfer preamble:%x len:%d tx:%x\n",preamble, len, ((uint16_t *)tx)[0]);
	else
		printk(KERN_INFO "it8951: it8951_spi_transfer preamble:%x len:%d\n",preamble, len);
#endif

	if (tx) {
		txbuf = kmalloc(len, GFP_KERNEL);
		if (!txbuf) {
			ret = -ENOMEM;
			goto out_free;
		}
		it8951_memcpy_swab16(epd, (uint16_t *)txbuf, (uint16_t *)tx, len / 2);
	}

	if (rx) {
		rxbuf = kmalloc(len, GFP_KERNEL);
		if (!rxbuf) {
			ret = -ENOMEM;
			goto out_free;
		}
	}

	it8951_wait_for_ready(epd, 100);

	if (dummy) {
		uint16_t dummy = 0;
		struct spi_transfer tr[3] = {};

		tr[0].tx_buf = &spreamble;
		tr[0].len = 2;
		tr[0].speed_hz = speed_hz;

		tr[1].rx_buf = &dummy;
		tr[1].len = 2;
		tr[1].speed_hz = speed_hz;

		tr[2].tx_buf = txbuf;
		tr[2].rx_buf = rxbuf;
		tr[2].len = len;
		tr[2].speed_hz = speed_hz;

		ret = spi_sync_transfer(epd->spi, tr, 3);
	} else {
		struct spi_transfer tr[2] = {};

		tr[0].tx_buf = &spreamble;
		tr[0].len = 2;
		tr[0].speed_hz = speed_hz;

		tr[1].tx_buf = txbuf;
		tr[1].rx_buf = rxbuf;
		tr[1].len = len;
		tr[1].speed_hz = speed_hz;

		ret = spi_sync_transfer(epd->spi, tr, 2);
	}

	if (rx && !ret) {
		it8951_memcpy_swab16(epd, (uint16_t *)rx, (uint16_t *)rxbuf, len / 2);

#ifdef IT8951_DEBUG
		for(i=0;i<len;i+=4) {
			printk(KERN_INFO "it8951: it8951_spi_transfer preamble:%x len:%d %d:%x %d:%x %d:%x %d:%x\n",preamble, len, i,rxbuf[i],i+1,rxbuf[i+1],i+2,rxbuf[i+2],i+3,rxbuf[i+3]);			
		}
#endif
	}

out_free:
	kfree(rxbuf);
	kfree(txbuf);
	return ret;
}

static void it8951_write_cmd_code(struct it8951_epd *epd, uint16_t cmd_code) {
	it8951_spi_transfer(epd, 0x6000, false, &cmd_code, NULL, 2);
}

static void it8951_write_data(struct it8951_epd *epd, uint16_t data) {
	it8951_spi_transfer(epd, 0x0000, false, &data, NULL, 2);
}

static void it8951_write_n_data(struct it8951_epd *epd, uint8_t *data, uint32_t len)
{
	it8951_spi_transfer(epd, 0x0000, false, data, NULL, len);
}

static uint16_t it8951_read_data(struct it8951_epd *epd) {
	uint16_t data = 0;
	it8951_spi_transfer(epd, 0x1000, true, NULL, &data, 2);
	return data;
}

static void it8951_read_n_data(struct it8951_epd *epd, uint8_t* buf, uint32_t len) {
	it8951_spi_transfer(epd, 0x1000, true, NULL, buf, len);
}

/* Power management */

void it8951_system_run(struct it8951_epd *epd)
{
	it8951_write_cmd_code(epd, IT8951_TCON_SYS_RUN);
}

void it8951_standby(struct it8951_epd *epd)
{
	it8951_write_cmd_code(epd, IT8951_TCON_STANDBY);
}

void it8951_sleep(struct it8951_epd *epd)
{
	it8951_write_cmd_code(epd, IT8951_TCON_SLEEP);
}

/* registers and commands */

static uint16_t it8951_read_reg(struct it8951_epd *epd, uint16_t reg_addr)
{
	uint16_t data;

	it8951_write_cmd_code(epd, IT8951_TCON_REG_RD);
	it8951_write_data(epd, reg_addr);
	data = it8951_read_data(epd);
	return data;
}

static void it8951_write_reg(struct it8951_epd *epd, uint16_t reg_addr, uint16_t value)
{
	it8951_write_cmd_code(epd, IT8951_TCON_REG_WR);
	it8951_write_data(epd, reg_addr);
	it8951_write_data(epd, value);
}

static void it8951_load_img_start(struct it8951_epd *epd, struct it8951_load_img_info* load_img_info)
{
	uint16_t arg;
	arg = (load_img_info->endian_type << 8 )
	      | (load_img_info->pixel_format << 4)
	      | (load_img_info->rotate);
	it8951_write_cmd_code(epd, IT8951_TCON_LD_IMG);
	it8951_write_data(epd, arg);
}

static void it8951_load_img_end(struct it8951_epd *epd)
{
	it8951_write_cmd_code(epd, IT8951_TCON_LD_IMG_END);
}

static void it8951_get_system_info(struct it8951_epd *epd)
{
	struct it8951_dev_info* dev_info = &epd->dev_info;

	memset(dev_info, 0, sizeof(struct it8951_dev_info));

	it8951_write_cmd_code(epd, USDEF_I80_CMD_GET_DEV_INFO);

	it8951_read_n_data(epd, (uint8_t *)dev_info, sizeof(struct it8951_dev_info));

	PRINFO("panel %dx%d\n",
	       dev_info->panel_w, dev_info->panel_h );
	PRINFO("FW version = %s\n", (uint8_t*)dev_info->fw_version);
	PRINFO("LUT version = %s\n", (uint8_t*)dev_info->lut_version);
}

// static void it8951_print_system_info(struct it8951_epd *epd)
// {
// 	struct it8951_dev_info* dev_info = &epd->dev_info;

// 	PRINFO("panel %dx%d\n",
// 			dev_info->panel_w, dev_info->panel_h );
// 	PRINFO("FW version = %s\n", (uint8_t*)dev_info->fw_version);
// 	PRINFO("LUT version = %s\n", (uint8_t*)dev_info->lut_version);
// }

static void it8951_set_img_buf_base_addr(struct it8951_epd *epd, uint32_t base_addr)
{
	uint16_t h = (uint16_t)((base_addr >> 16) & 0x0000FFFF);
	uint16_t l = (uint16_t)( base_addr & 0x0000FFFF);
	it8951_write_reg(epd, LISAR + 2, h);
	it8951_write_reg(epd, LISAR, l);
}

static void it8951_wait_for_display_ready(struct it8951_epd *epd)
{
	//Check IT8951 Register LUTAFSR => NonZero Busy, 0 - Free
	while (it8951_read_reg(epd, LUTAFSR)) {
		//printk(KERN_INFO "it8951: wait_for_display_ready\n");
		usleep_range(1000, 2000);
	}
}

static void it8951_packed_pixel_write(struct it8951_epd *epd, struct it8951_load_img_info* load_img_info)
{
	uint32_t j = 0;
	//Source buffer address of Host
	uint8_t* frame_buf = (uint8_t*)load_img_info->start_fb_addr;

	//Set Image buffer(IT8951) Base address
	it8951_set_img_buf_base_addr(epd, load_img_info->img_buf_base_addr);
	//Send Load Image start Cmd
	it8951_load_img_start(epd, load_img_info);
	//Host Write Data
	for (j = 0; j < epd->dev_info.panel_h / 2; j++) // 4bits
	{
		it8951_write_n_data(epd, frame_buf, epd->dev_info.panel_w);
		frame_buf += epd->dev_info.panel_w;
	}

	//Send Load Img End Command
	it8951_load_img_end(epd);
}

static void it8951_display_area(struct it8951_epd *epd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t dpy_mode)
{
	it8951_write_cmd_code(epd, USDEF_I80_CMD_DPY_AREA);
	it8951_write_data(epd, x);
	it8951_write_data(epd, y);
	it8951_write_data(epd, w);
	it8951_write_data(epd, h);
	it8951_write_data(epd, dpy_mode);


	it8951_wait_for_ready(epd, w * h / 8); // wait longer for data
}

static uint16_t it8951_get_vcom(struct it8951_epd *epd)
{
	uint16_t ret;
	it8951_wait_for_ready(epd, 100);
	it8951_write_cmd_code(epd, USDEF_I80_CMD_VCOM);
	it8951_write_data(epd, 0x0000);
	ret = it8951_read_data(epd);

	PRINFO("VCOM = %d\n", ret);
	return ret;
}

static void it8951_set_vcom(struct it8951_epd *epd)
{
	it8951_write_cmd_code(epd, USDEF_I80_CMD_VCOM);
	it8951_write_data(epd, 0x0001);
	it8951_write_data(epd, epd->vcom);
}


static inline uint8_t _it8951_rgb_to_4bits(uint32_t rgb) {
	u8 r = (rgb & 0x00ff0000) >> 16;
	u8 g = (rgb & 0x0000ff00) >> 8;
	u8 b =  rgb & 0x000000ff;

	/* ITU BT.601: Y = 0.299 R + 0.587 G + 0.114 B */
	return ((3 * r + 6 * g + b) / 10) >> 4;
}

static void it8951_xrgb8888_to_gray4(uint8_t *dst_vaddr, void *src_vaddr, struct fb_info *info)
{
	unsigned int x, y, w, h;
	u32 *src;
	u8 *dst;

	w = info->var.xres;
	h = info->var.yres;
	
	for (y = 0; y < h; y++) {
		src = src_vaddr + (y * info->fix.line_length);
		dst = dst_vaddr + (y * w ) / 2;

		for (x = 0; x < w; x += 2) {
			*dst++ = _it8951_rgb_to_4bits(*src) + (_it8951_rgb_to_4bits(*(src + 1)) << 4);
			src += 2;
		}
	}
}

static void it8951_dev_init(struct it8951_epd *epd)
{
	// struct it8951_epd *epd = epd_from_tinydrm(tdev);
	//struct drm_framebuffer *fb = pipe->plane.fb;

	printk(KERN_INFO "it8951: initializing\n");

	gpiod_set_value_cansleep(epd->reset, 0);
	msleep(100);
	gpiod_set_value_cansleep(epd->reset, 1);

	msleep(100);

	it8951_get_system_info(epd);

	epd->img_buf_addr = epd->dev_info.img_buf_addr_l | (epd->dev_info.img_buf_addr_h << 16);

	//Set to Enable I80 Packed mode
	it8951_write_reg(epd, I80CPCR, 0x0001);

	if (it8951_get_vcom(epd) != epd->vcom){
		it8951_set_vcom(epd);
		it8951_get_vcom(epd);
	}

	it8951_standby(epd);
	epd->running = false;
}

/* ################################################################## Linux driver part ################################################################## */
static int it8951_update_display(struct fb_info *info)
{
	struct it8951_epd *epd;
	struct it8951_load_img_info load_img_info;
	// int i;
	PRINFO("entering %s()", __FUNCTION__);

	epd = info->par;
	PRINFO("frame size %d", info->fix.smem_len);
	// development purposed
	// for (i = 0; i < 20; i++){
	// 	if (info->screen_base[i] != 0){
	// 		printk(KERN_INFO "%x at %d",  info->screen_base[i],i);
	// 	}
	// }

	it8951_xrgb8888_to_gray4(epd->ssbuf,info->screen_base, info);

	load_img_info.start_fb_addr		= (uint32_t) epd->ssbuf;
	load_img_info.endian_type    	= IT8951_LDIMG_L_ENDIAN;
	load_img_info.pixel_format 		= IT8951_4BPP;
	load_img_info.rotate      		= IT8951_ROTATE_0;
	load_img_info.img_buf_base_addr = epd->img_buf_addr;


	if (!epd->running){
		it8951_system_run(epd);
	}
	epd->running = true;

	
	it8951_wait_for_display_ready(epd);
	it8951_packed_pixel_write(epd, &load_img_info);

	it8951_display_area(epd, 0, 0, epd->dev_info.panel_w, epd->dev_info.panel_h, IT8951_MODE_GC16);

	if (epd->running){
		it8951_standby(epd);
	}
	epd->running = false;


	PRINFO("finished updating display");
	
	PRINFO("exiting %s()", __FUNCTION__);
	return 0;
}

void it8951_fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	// more testing needs to be done on either using this or,
	// ( comment continue after "schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);" )
	PRINFO("entering %s()", __FUNCTION__);
	// int ret;
	// it8951_print_system_info(info->par);
	cfb_fillrect(info, rect);
	schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);

	// using this, preliminary test shows that kernel segfaults when using fbcon
	// ret = it8951_update_display(info);
	// if (ret)
	// 	dev_err(info->device, "%s: failed to update display", __func__);
}

void it8951_fb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	// more testing needs to be done on either using this or,
	// ( comment continue after "schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);" )
	PRINFO("entering %s()", __FUNCTION__);
	// int ret;
	// it8951_print_system_info(info->par);
	cfb_copyarea(info, area);
	schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);

	// using this, preliminary test shows that kernel segfaults when using fbcon
	// ret = it8951_update_display(info);
	// if (ret)
	// 	dev_err(info->device, "%s: failed to update display", __func__);
}

void it8951_fb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	// more testing needs to be done on either using this or,
	// ( comment continue after "schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);" )
	PRINFO("entering %s()", __FUNCTION__);
	// int ret;
	// it8951_print_system_info(info->par);
	cfb_imageblit(info, image);
	schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);

	// using this, preliminary test shows that kernel segfaults when using fbcon
	// ret = it8951_update_display(info);
	// if (ret)
	// 	dev_err(info->device, "%s: failed to update display", __func__);
}

static ssize_t it8951_fb_write(struct fb_info *info, const char __user *buf,
				size_t count, loff_t *ppos)
{
	// fb_sys_write and scheduled_delayed_work 
	// is also used by https://blog.react0r.com/2019/09/20/prototype-low-power-low-cost-linux-terminal-device/
	// when doing cat /dev/urandom > /dev/fb1 this works but,(comment line continue after "return res;")
	ssize_t res = fb_sys_write(info, buf, count, ppos);
	schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);
	return res;

	// not sure if this is better. this is used by 
	// https://github.com/aldwinhermanudin/mangOH/blob/master/experimental/waveshare_eink/linux_kernel_modules/fb_waveshare_eink.c
	// PRINFO("entering %s()", __FUNCTION__);
	// unsigned long p = *ppos;
	// void *dst;
	// int err = 0;
	// unsigned long total_size;

	// if (info->state != FBINFO_STATE_RUNNING)
	// 	return -EPERM;

	// total_size = info->fix.smem_len;

	// if (p > total_size)
	// 	return -EFBIG;

	// if (count > total_size) {
	// 	err = -EFBIG;
	// 	count = total_size;
	// }

	// if (count + p > total_size) {
	// 	if (!err)
	// 		err = -ENOSPC;

	// 	count = total_size - p;
	// }

	// dst = (void __force *)(info->screen_base + p);

	// if (copy_from_user(dst, buf, count))
	// 	err = -EFAULT;

	// if (!err)
	// 	*ppos += count;

	// return (err) ? err : count;
}

static int it8951_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct it8951_epd *epd;
	PRINFO("entering %s()", __FUNCTION__);
	
	epd = info->par;
	switch (cmd) {
		case IT8951_FB_SET_SCREEN_SLEEP:
			it8951_sleep(epd);
			break;
		case IT8951_FB_SET_SCREEN_STANDBY:
			it8951_standby(epd);
			break;
		default:
			return EINVAL;
	}

	return 0;
}

static struct fb_ops it8951_ops = {
	.owner			= THIS_MODULE,
	.fb_read		= fb_sys_read,
	.fb_write		= it8951_fb_write,
	.fb_fillrect	= it8951_fb_fillrect,
	.fb_copyarea	= it8951_fb_copyarea,
	.fb_imageblit	= it8951_fb_imageblit,
	.fb_ioctl 		= it8951_fb_ioctl,
};

static void it8951_deferred_io(	struct fb_info *info,
								struct list_head *pagelist )
{
	int ret = it8951_update_display(info);
	if (ret)
		dev_err(info->device, "%s: failed to update display", __func__);
}

static struct fb_deferred_io it8951_defio = {
	.delay			= HZ / 4,
	.deferred_io	= it8951_deferred_io,
};

#ifdef CONFIG_OF
static const struct of_device_id it8951_dt_match[] = {
	{
		.compatible = "it8951",
	},
	{},
};
MODULE_DEVICE_TABLE(of, it8951_dt_match);

static int it8951_probe_dt(struct it8951_epd *epd)
{
	struct device *dev = &epd->spi->dev;
	struct device_node *node = dev->of_node;
	struct of_device_id const *match;
	int ret = 0;

	/* Check that device tree node is ok */
	if(node == NULL) {
		ERR("Device does not have associated device tree data\n");
		ret = -EINVAL;
		goto out;
	}
	match = of_match_device(it8951_dt_match, dev);
	if(match == NULL) {
		ERR("Unknown device model\n");
		ret = -EINVAL;
		goto out;
	}

	// get vcom value from device tree
	ret = of_property_read_u32(node, "vcom", &(epd->vcom));
	if (ret == 0) {
		printk(KERN_INFO "[IT8951]: Succeeded getting vcom from DT %d\n", epd->vcom);
	} else {
		/* Couldn't find the entry */
		ERR("Cannot get vcom data\n");
		ret = -EINVAL;
		goto out;
	}

	// get max_speed_hz value from device tree
	ret = of_property_read_u32(node, "spi-max-frequency", &(epd->spi->max_speed_hz));
	if (ret == 0) {
		printk(KERN_INFO "[IT8951]: Succeeded setting SPI speed from DT %d MHz\n", (epd->spi->max_speed_hz/1000000));
	} else {
		/* Couldn't find the entry */
		ERR("Cannot get vcom data\n");
		ret = -EINVAL;
		goto out;
	}

	printk(KERN_INFO "Succeeded getting data from DT\n");
out:
	return ret;
}
#else
static int it8951_probe_dt(struct it8951_epd *epd)
{
	return -EINVAL;
}
#endif

static int it8951_probe(struct spi_device *spi){
	int ret = 0;
	
	struct fb_info *info;
	struct it8951_epd *epd;
	
	int vmem_size;

	struct device *dev = &spi->dev;

	// setup framebuffer and allocate it8951_epd object
	info = framebuffer_alloc(sizeof(struct it8951_epd), &(spi->dev));
	if (!info){
		return -ENOMEM;
	}

	epd = info->par;	// assign the allocated it8951_epd to an alias

	epd->spi = spi;	// save spi objet to somewhere else

	// setup REST GPIO
	epd->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(epd->reset)) {
		ret = PTR_ERR(epd->reset);
		if (ret != -EPROBE_DEFER)
			PRERROR("Failed to get gpio 'reset'\n");
		return ret;
	}

	// setup HRDY gpio
	epd->hrdy = devm_gpiod_get(dev, "hrdy", GPIOD_IN);
	if (IS_ERR(epd->hrdy)) {
		ret = PTR_ERR(epd->hrdy);
		if (ret != -EPROBE_DEFER)
			PRERROR("Failed to get gpio 'reset'\n");
		return ret;
	}

	/*
	 * Get platform data in order to get all gpios config for busy, d/c, and reset.
	 * if not found fetch it from dt. always get from device tree
	 */
	ret = it8951_probe_dt(epd);
	if(ret < 0) {
		ERR("Fail to get platform data\n");
		return ret;
	}

	// setup SPI device
	PRINFO("Setting-up SPI device\n");
	epd->spi->mode = SPI_MODE_0;
	epd->spi->bits_per_word = 8;
	ret = spi_setup(epd->spi);
	if (ret < 0){
		PRERROR("Failed to setup SPI device\n");
		return ret;
	}
	PRINFO("Succeeded to setup SPI device\n");
	
#ifdef __LITTLE_ENDIAN 
	epd->little_endian = true;
#else
	epd->little_endian = false;
#endif	
	epd->bpp = 32;

	it8951_dev_init(epd);

	vmem_size = epd->dev_info.panel_w * epd->dev_info.panel_h * epd->bpp / 8;
	info->screen_base = vzalloc(vmem_size);
	if (!info->screen_base) {
		ret = -ENOMEM;
		goto screen_base_fail;
	}

	info->fbops = &it8951_ops;

	info->fix.type				= FB_TYPE_PACKED_PIXELS;
	info->fix.visual			= FB_VISUAL_TRUECOLOR;
	info->fix.smem_len			= vmem_size;
	info->fix.xpanstep			= 0;
	info->fix.ypanstep			= 0;
	info->fix.ywrapstep			= 0;
	info->fix.line_length		= epd->dev_info.panel_w * epd->bpp / 8;

	info->var.red.offset = 16;
	info->var.red.length = 8;
	info->var.green.offset = 8;
	info->var.green.length = 8;
	info->var.blue.offset = 0;
	info->var.blue.length = 8;
	info->var.transp.offset = 0;
	info->var.transp.length = 0;

	info->var.xres				= epd->dev_info.panel_w;
	info->var.yres				= epd->dev_info.panel_h;
	info->var.xres_virtual		= epd->dev_info.panel_w;
	info->var.yres_virtual		= epd->dev_info.panel_h;
	info->var.bits_per_pixel	= epd->bpp;

	info->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;

	info->fbdefio = &it8951_defio;
	fb_deferred_io_init(info);

	epd->ssbuf	= vzalloc(vmem_size);

	if (!epd->ssbuf) {
		ret = -ENOMEM;
		goto ssbuf_alloc_fail;
	}

	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(&(epd->spi->dev), "framebuffer registration failed");
		goto fbreg_fail;
	}

	spi_set_drvdata(spi, info);

	if (ret) {
		dev_err(&spi->dev, "display initialization failed");
		goto disp_init_fail;
	}

	printk(KERN_INFO
		"fb%d: %s frame buffer device,\n\tusing %d Byte of video memory\n",
		info->node, info->fix.id, vmem_size);

	return 0;

	disp_init_fail:
		framebuffer_release(info);
	fbreg_fail:
		vfree(epd->ssbuf);
	ssbuf_alloc_fail:
		vfree(info->screen_base);
	screen_base_fail:
		vfree(info->screen_base);

	return ret;
}

static int it8951_remove(struct spi_device *spi)
{
	struct fb_info *info = spi_get_drvdata(spi);
	struct it8951_epd *epd = info->par;

	printk(KERN_INFO "Removing SPI device\n");

	fb_deferred_io_cleanup(info);
	unregister_framebuffer(info);
	vfree(info->screen_base);
	vfree(epd->ssbuf);
	framebuffer_release(info);

	PRINFO("Resources released");

	return 0;
}

static struct spi_driver it8951_driver = {
	.driver = {
		.name	 = "it8951",
		.of_match_table = of_match_ptr(it8951_dt_match),
		.owner	= THIS_MODULE,
	},
	.probe	 = it8951_probe,
	.remove  = it8951_remove,
};

static __init int it8951_init(void)
{	
	printk(KERN_INFO "Registering SPI device\n");
	return spi_register_driver(&it8951_driver);
}

module_init(it8951_init);

static __exit void it8951_exit(void)
{
	printk(KERN_INFO "De-registering SPI device\n");
	spi_unregister_driver(&it8951_driver);
}
module_exit(it8951_exit);

MODULE_DESCRIPTION("IT8951 Framebuffer Driver");
MODULE_AUTHOR("Aldwin Hermanudin <aldwin@hermanudin.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:it8951");