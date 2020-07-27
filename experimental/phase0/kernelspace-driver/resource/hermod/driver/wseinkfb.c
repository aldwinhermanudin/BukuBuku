#include <linux/module.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/fb.h>
#include <linux/spi/spi.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#define WS_PANEL_SETTING                               0x00
#define WS_POWER_SETTING                               0x01
#define WS_POWER_OFF                                   0x02
#define WS_POWER_OFF_SEQUENCE_SETTING                  0x03
#define WS_POWER_ON                                    0x04
#define WS_POWER_ON_MEASURE                            0x05
#define WS_BOOSTER_SOFT_START                          0x06
#define WS_DEEP_SLEEP                                  0x07
#define WS_DATA_START_TRANSMISSION_1                   0x10
#define WS_DATA_STOP                                   0x11
#define WS_DISPLAY_REFRESH                             0x12
#define WS_IMAGE_PROCESS                               0x13
#define WS_LUT_FOR_VCOM                                0x20
#define WS_LUT_BLACK                                   0x21
#define WS_LUT_WHITE                                   0x22
#define WS_LUT_GRAY_1                                  0x23
#define WS_LUT_GRAY_2                                  0x24
#define WS_LUT_RED_0                                   0x25
#define WS_LUT_RED_1                                   0x26
#define WS_LUT_RED_2                                   0x27
#define WS_LUT_RED_3                                   0x28
#define WS_LUT_XON                                     0x29
#define WS_PLL_CONTROL                                 0x30
#define WS_TEMPERATURE_SENSOR_COMMAND                  0x40
#define WS_TEMPERATURE_CALIBRATION                     0x41
#define WS_TEMPERATURE_SENSOR_WRITE                    0x42
#define WS_TEMPERATURE_SENSOR_READ                     0x43
#define WS_VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define WS_LOW_POWER_DETECTION                         0x51
#define WS_TCON_SETTING                                0x60
#define WS_TCON_RESOLUTION                             0x61
#define WS_SPI_FLASH_CONTROL                           0x65
#define WS_REVISION                                    0x70
#define WS_GET_STATUS                                  0x71
#define WS_AUTO_MEASUREMENT_VCOM                       0x80
#define WS_READ_VCOM_VALUE                             0x81
#define WS_VCM_DC_SETTING                              0x82
#define WS_FLASH_DISABLE                               0xB9
#define WS_FLASH_MODE                                  0xE5 /* only in demo */

/* Table size if WS_LUT_REPEAT*N, where N is one LUT type below. Add
   one for LUT command on sizes on spec. */
#define WS_LUT_REPEAT                                  20

#define WS_LUT_V_SIZE                                  11
#define WS_LUT_B_SIZE                                  13
#define WS_LUT_W_SIZE                                  13
#define WS_LUT_G1_SIZE                                 13
#define WS_LUT_G2_SIZE                                 13
#define WS_LUT_R0_SIZE                                 13
#define WS_LUT_R1_SIZE                                 13
#define WS_LUT_R2_SIZE                                 13
#define WS_LUT_X_SIZE                                  10

/* for fixed buffer allocation */
#define WS_LUT_MAX_SIZE                                WS_LUT_REPEAT*WS_LUT_B_SIZE

enum ws_eink_devices {
  DEV_WS_213,
  DEV_WS_27,
  DEV_WS_29,
  DEV_WS_42,
  DEV_WS_75,
};

struct ws_eink_device_properties {
  unsigned int width;
  unsigned int height;
  unsigned int bpp;
};

static struct ws_eink_device_properties devices[] =
{
  [DEV_WS_213] = {.width = 128, .height = 250, .bpp = 1},
  [DEV_WS_27]  = {.width = 176, .height = 264, .bpp = 1},
  [DEV_WS_29]  = {.width = 128, .height = 296, .bpp = 1},
  [DEV_WS_42]  = {.width = 300, .height = 400, .bpp = 1},
  [DEV_WS_75]  = {.width = 640, .height = 384, .bpp = 4},
};

struct ws_eink_state {
  struct spi_device *spi;
  struct fb_info *info;
  const struct ws_eink_device_properties *props;
  int rst;
  int dc;
  int busy;
  bool use_flash;
};

/* These are all guesses that don't seem to work. */
static u8 lut_vcom_partial[] =
  { 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  };

static u8 lut_white_partial[] =
  { 0xA0, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  };

static u8 lut_black_partial[] =
  { 0x50, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  };

static u8 lut_xon_partial[] =
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

static void device_write_data(struct ws_eink_state *par, u8 data)
{
  int ret = 0;
  gpio_set_value(par->dc, 1);
  
  ret = spi_write(par->spi, &data, sizeof(data));
  if (ret < 0) {
    printk("spi write data error\n");
    pr_err("%s: write data %02x failed with status %d\n",
	   par->info->fix.id, data, ret);
  }
}

static int device_write_data_buf(struct ws_eink_state *par, const u8 *txbuf,
				size_t size)
{
  gpio_set_value(par->dc, 1);

  return spi_write(par->spi, txbuf, size);
}

static void device_write_cmd(struct ws_eink_state *par, u8 cmd)
{
  int ret = 0;

  gpio_set_value(par->dc, 0);

  ret = spi_write(par->spi, &cmd, sizeof(cmd));
  if (ret < 0)
    pr_err("%s: write command %02x failed with status %d\n",
	   par->info->fix.id, cmd, ret);
}

static void device_wait_until_idle(struct ws_eink_state *par)
{
  while (gpio_get_value(par->busy) == 0)
    mdelay(100);
}

static void device_reset(struct ws_eink_state *par)
{
  /* GPIO cansleep here? is this just a minimum time? */
  gpio_set_value(par->rst, 0);
  mdelay(200);
  gpio_set_value(par->rst, 1);
  mdelay(200);
}

static void set_lut(struct ws_eink_state *par, u8 cmd)
{
  u8 lut[WS_LUT_MAX_SIZE];
  size_t lut_size;
  u8 *fragment;
  size_t fragment_size;

  switch (cmd) {
  case WS_LUT_FOR_VCOM:
    lut_size = WS_LUT_REPEAT*WS_LUT_V_SIZE;
    fragment = lut_vcom_partial;
    fragment_size = sizeof(lut_vcom_partial);
    break;
  case WS_LUT_BLACK:
  case WS_LUT_GRAY_1:
  case WS_LUT_GRAY_2:
  case WS_LUT_RED_0:
  case WS_LUT_RED_1:
  case WS_LUT_RED_2:
  case WS_LUT_RED_3:
    /* Using Red0 as Black on B&W display */
    lut_size = WS_LUT_REPEAT*WS_LUT_B_SIZE;
    fragment = lut_black_partial;
    fragment_size = sizeof(lut_black_partial);
    break;
  case WS_LUT_WHITE:
    lut_size = WS_LUT_REPEAT*WS_LUT_W_SIZE;
    fragment = lut_white_partial;
    fragment_size = sizeof(lut_white_partial);
    break;
  case WS_LUT_XON:
    lut_size = WS_LUT_REPEAT*WS_LUT_X_SIZE;
    fragment = lut_xon_partial;
    fragment_size = sizeof(lut_xon_partial);
    break;
  default:
    dev_warn(&par->spi->dev, "Attempting to set unsupported LUT\n");
    return;
  }

  memset(lut, 0, lut_size);
  memcpy(lut, fragment, fragment_size);
  dev_info(&par->spi->dev, "Send %d bytes with LUT command 0x%x\n", lut_size, cmd);
  device_write_cmd(par, cmd);
  device_write_data_buf(par, lut, lut_size);
}

static void init_display(struct ws_eink_state *par)
{
  dev_info(&par->spi->dev, "initializing Waveshare device:\n\t SPI %d bpw, mode %x\n\tPanel mode: 0x%X\n",
	   par->spi->bits_per_word, par->spi->mode,
	   0xCF | (!par->use_flash&1)<<5);

  device_reset(par);

  device_write_cmd(par, WS_BOOSTER_SOFT_START);
  device_write_data(par, 0xC7); /* A: Soft start phase: 40ms, Driving strength: 1, Min. off time: 6.77us */
  device_write_data(par, 0xCC); /* B: Soft start phase: 40ms, Driving strength: 2, Min. off time: 0.77us */
  device_write_data(par, 0x28); /* C: Driving strength: 6, Min. off time: 0.2us */

  device_write_cmd(par, WS_POWER_SETTING);
  device_write_data(par, 0x37); /* default internal power settings */
  device_write_data(par, 0x00); /* voltage level */
  device_write_data(par, 0x08); /* VDH select for red LUT: 3.8V */
  device_write_data(par, 0x08); /* VDL select for red LUT: -3.8V */

  device_write_cmd(par, WS_POWER_ON);
  device_wait_until_idle(par);

  if (!par->use_flash) {
    device_write_cmd(par, WS_SPI_FLASH_CONTROL);
    device_write_data(par, 0x01); /* Enable bypass */

    device_write_cmd(par, WS_FLASH_DISABLE);

    device_write_cmd(par, WS_SPI_FLASH_CONTROL);
    device_write_data(par, 0x00);
  }

  device_write_cmd(par, WS_PANEL_SETTING);
  /* Resolution: 11b, LUT select: !use_flash, Scan: up, Shift: right, Booster: on, Reset: no */
  device_write_data(par, 0xCF | (!par->use_flash&1)<<5); /* TODO: confirm */
  device_write_data(par, 0x00);

  device_write_cmd(par, WS_PLL_CONTROL);
  device_write_data(par, 0x3C); /* PLL clock: 50 Hz */

  device_write_cmd(par, WS_TCON_RESOLUTION);
  device_write_data(par, par->props->width>>8); /* HRES */
  device_write_data(par, par->props->width);
  device_write_data(par, par->props->height>>8); /* VRES */
  device_write_data(par, par->props->height);

  device_write_cmd(par, WS_VCM_DC_SETTING);
  device_write_data(par, 0x1E); /* -3.0V */

  device_write_cmd(par, WS_VCOM_AND_DATA_INTERVAL_SETTING);
  device_write_data(par, 0x77); /* Border selection: White, Data polarity: 1, VCOM and data interval: 10 */
  //device_write_data(par, 0x67); /* Invert colors */

  if (par->use_flash) {
    device_write_cmd(par, WS_FLASH_MODE);
    device_write_data(par, 0x3); /* "Define the flash" */
  }

}

static void display_frame(struct ws_eink_state *par)
{

  if (!par->use_flash) {
    /* Negligible time to send LUT */
    set_lut(par, WS_LUT_FOR_VCOM);
    set_lut(par, WS_LUT_BLACK);
    set_lut(par, WS_LUT_WHITE);
    set_lut(par, WS_LUT_GRAY_1);
    set_lut(par, WS_LUT_GRAY_2);
    set_lut(par, WS_LUT_RED_0);
    set_lut(par, WS_LUT_RED_1);
    set_lut(par, WS_LUT_RED_2);
    set_lut(par, WS_LUT_RED_3);
    set_lut(par, WS_LUT_XON);
  }

  /* Takes about 400 ms to send data with 3.125 Mhz SPI clock */
  device_write_cmd(par, WS_DATA_START_TRANSMISSION_1);
  device_write_data_buf(par, par->info->screen_base, par->info->fix.smem_len);

  /* This doesn't seem to make a difference. */
  //device_write_cmd(par, WS_DATA_STOP);
  //device_wait_until_idle(par);

  //device_write_cmd(par, WS_IMAGE_PROCESS);
  //device_write_data(par, 0x14); /* Action: enable, Selection: all pixels */

  /* Takes about 4s to refresh in use_flash mode */
  device_write_cmd(par, WS_DISPLAY_REFRESH);
  device_wait_until_idle(par);
}

int init_gpio_from_of(struct device *dev, const char *gpio_name, int dir, int *init_gpio)
{
  struct device_node *np = dev->of_node;
  int gpio;
  int ret;

  gpio = of_get_named_gpio(np, gpio_name, 0);
  if (!gpio_is_valid(gpio)) {
    dev_err(dev, "No valid gpio found for %s\n", gpio_name);
    return -ENODEV;
  }

  ret = devm_gpio_request(dev, gpio, "sysfs");
  if (ret) {
    dev_err(dev, "Failed to request gpio %s\n", gpio_name);
    return ret;
  }

  if (dir)
    gpio_direction_input(gpio);
  else
    gpio_direction_output(gpio, 0);
  gpio_export(gpio, true);

  *init_gpio = gpio;
  return 0;
}

static ssize_t ws_eink_fb_write(struct fb_info *info,
				const char __user *buf, size_t count,
				loff_t *ppos)
{
  ssize_t res = fb_sys_write(info, buf, count, ppos);
  schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);
  return res;
}

static void ws_eink_fb_fillrect(struct fb_info *info,
				 const struct fb_fillrect *rect)
{
  cfb_fillrect(info, rect);
  schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);
}

static void ws_eink_fb_copyarea(struct fb_info *info,
				 const struct fb_copyarea *area)
{
  cfb_copyarea(info, area);
  schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);
}

static void ws_eink_fb_imageblit(struct fb_info *info,
				  const struct fb_image *image)
{
  cfb_imageblit(info, image);
  schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);
}

/*
int ws_eink_fb_release(struct fb_info *info, int user)
{
  struct ws_eink_state *par = info->par;
  display_frame(par);
  return 0;
}
*/

static struct fb_ops ws_eink_ops = {
  .owner	= THIS_MODULE,
  .fb_read	= fb_sys_read,
  .fb_write	= ws_eink_fb_write,
  .fb_fillrect	= ws_eink_fb_fillrect,
  .fb_copyarea	= ws_eink_fb_copyarea,
  .fb_imageblit	= ws_eink_fb_imageblit,
};

static const struct of_device_id ws_eink_of_match[] = {
  { .compatible = "waveshare,213", .data = (void *)DEV_WS_213 },
  { .compatible = "waveshare,27", .data = (void *)DEV_WS_27 },
  { .compatible = "waveshare,29", .data = (void *)DEV_WS_29 },
  { .compatible = "waveshare,42", .data = (void *)DEV_WS_42 },
  { .compatible = "waveshare,75", .data = (void *)DEV_WS_75 },
  {},
};
MODULE_DEVICE_TABLE(of, ws_eink_of_match);

static struct spi_device_id waveshare_eink_tbl[] = {
  { "waveshare_213", DEV_WS_213 },
  { "waveshare_27",  DEV_WS_27 },
  { "waveshare_29",  DEV_WS_29 },
  { "waveshare_42",  DEV_WS_42 },
  { "waveshare_75",  DEV_WS_75 },
  { },
};
MODULE_DEVICE_TABLE(spi, waveshare_eink_tbl);

static void ws_eink_deferred_io(struct fb_info *info,
				struct list_head *pagelist)
{
  display_frame(info->par);
}

static struct fb_deferred_io ws_eink_defio = {
  .delay	= HZ*3,
  .deferred_io	= ws_eink_deferred_io,
};

static int ws_eink_spi_probe(struct spi_device *spi)
{
  struct fb_info *info;
  const struct spi_device_id *spi_id;
  const struct ws_eink_device_properties *props;
  struct ws_eink_state *par;
  u8 *vmem;
  int vmem_size;
  struct device *dev = &spi->dev;
  const struct of_device_id *match;
  int rst_gpio, dc_gpio, busy_gpio;
  int ret = 0;
  
  match = of_match_device(ws_eink_of_match, dev);
  if (match) {
    props = &devices[(kernel_ulong_t)match->data];
  } else {
    spi_id = spi_get_device_id(spi);
    if (!spi_id) {
      dev_err(dev, "device id not supported!\n");
      return -EINVAL;
    }
    props = &devices[spi_id->driver_data];
  }

  ret = init_gpio_from_of(dev, "ws,rst-gpios", 0, &rst_gpio);
  if (ret) return ret;
	
  ret = init_gpio_from_of(dev, "ws,dc-gpios", 0, &dc_gpio);
  if (ret) return ret;
	
  ret = init_gpio_from_of(dev, "ws,busy-gpios", 1, &busy_gpio);
  if (ret) return ret;

  /* plus one to include size % 8 */
  vmem_size = props->width * props->height * props->bpp / 8;
  vmem = vzalloc(vmem_size);
  if (!vmem)
    return -ENOMEM;

  info = framebuffer_alloc(sizeof(struct ws_eink_state), dev);
  if (!info) {
    ret = -ENOMEM;
    goto fballoc_fail;
  }

  info->screen_base = (u8 __force __iomem *)vmem;
  info->fbops = &ws_eink_ops;

  /* why WARN_ON here? */
  WARN_ON(strlcpy(info->fix.id, "wsinkfb", sizeof(info->fix.id)) >=
	  sizeof(info->fix.id));
  info->fix.type		= FB_TYPE_PACKED_PIXELS;
  info->fix.visual	= FB_VISUAL_STATIC_PSEUDOCOLOR;
  info->fix.smem_len	= vmem_size;
  info->fix.xpanstep	= 0;
  info->fix.ypanstep	= 0;
  info->fix.ywrapstep	= 0;
  info->fix.line_length	= props->width * props->bpp / 8; // +1?

  info->var.xres		= props->width;
  info->var.yres		= props->height;
  info->var.xres_virtual	= props->width;
  info->var.yres_virtual	= props->height;
  info->var.bits_per_pixel	= props->bpp;
  info->var.nonstd		= FB_NONSTD_REV_PIX_IN_B;

  info->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;

  info->fbdefio = &ws_eink_defio;
  fb_deferred_io_init(info);

  par = info->par;
  par->info		= info;
  par->spi		= spi;
  par->props		= props;
  par->rst		= rst_gpio;
  par->dc		= dc_gpio;
  par->busy		= busy_gpio;
  par->use_flash  	= true; /* TODO: make available to sysfs  */

  ret = register_framebuffer(info);
  if (ret < 0) {
    dev_err(dev, "framebuffer registration failed");
    goto fbreg_fail;
  }

  spi_set_drvdata(spi, par);

  init_display(par);

  /* wipe screen */
  memset(vmem, 0x33, vmem_size);
  display_frame(par);

  dev_dbg(dev,
	  "fb%d: %s frame buffer device,\n\tusing %d KiB of video memory\n",
	  info->node, info->fix.id, vmem_size);

  return 0;

fbreg_fail:
  fb_deferred_io_cleanup(info);
  framebuffer_release(info);

fballoc_fail:
  vfree(vmem);

  return ret;
}

static int ws_eink_spi_remove(struct spi_device *spi)
{  
  struct ws_eink_state *par = spi_get_drvdata(spi);
  struct fb_info *info = par->info;
  unregister_framebuffer(info);
  fb_deferred_io_cleanup(info);
  //fb_dealloc_cmap(&info->cmap);
  iounmap(info->screen_base);
  framebuffer_release(info);

  return 0;
}

static struct spi_driver ws_eink_driver = {
  .driver = {
    .name	= "wseinkfb",
    .owner	= THIS_MODULE,
    .of_match_table = ws_eink_of_match,
  },

  .id_table	= waveshare_eink_tbl,
  .probe	= ws_eink_spi_probe,
  .remove 	= ws_eink_spi_remove,
};
module_spi_driver(ws_eink_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raymond Ball");
MODULE_DESCRIPTION("FB driver for Waveshare eink displays");
MODULE_VERSION("0.2");
