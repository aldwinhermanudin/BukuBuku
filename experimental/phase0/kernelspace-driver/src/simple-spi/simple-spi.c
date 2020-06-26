
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

// static int ws2in13b_set_reg(struct device *dev, unsigned char address,
// 				unsigned char data)
// {
// 	struct spi_device *spi = to_spi_device(dev);
// 	unsigned char buf[2];

// 	/* MSB must be '1' to indicate write */
// 	buf[0] = address | 0x80;
// 	buf[1] = data;

// 	return spi_write_then_read(spi, buf, 2, NULL, 0);
// }

// static int ws2in13b_get_reg(struct device *dev, unsigned char address,
// 				unsigned char *data)
// {
// 	struct spi_device *spi = to_spi_device(dev);

// 	*data = address & 0x7f;

// 	return spi_write_then_read(spi, data, 1, data, 1);
// }

static int ws2in13b_probe(struct spi_device *spi){
   int ret = 0;
	unsigned char buf;

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	spi->max_speed_hz = 1*1000*1000;
	
   ret = spi_setup(spi);

   buf = 0x04;
   spi_write(spi, &buf,1);

   if (ret < 0){
      return ret;
   }

	return 0;
}

// static int ws2in13b_remove(struct spi_device *spi)
// {
// 	// struct rtc_device *rtc = platform_get_drvdata(spi);

// 	// rtc_device_unregister(rtc);
// 	return 0;
// }

static struct spi_driver ws2in13b_driver = {
	.driver = {
		.name	 = "ws2in13b",
		.owner	= THIS_MODULE,
	},
	.probe	 = ws2in13b_probe,
	// .remove  = ws2in13b_remove,
};

static __init int ws2in13b_init(void)
{
	return spi_register_driver(&ws2in13b_driver);
}
module_init(ws2in13b_init);

static __exit void ws2in13b_exit(void)
{
	spi_unregister_driver(&ws2in13b_driver);
}
module_exit(ws2in13b_exit);

MODULE_DESCRIPTION("Waveshare 2in13b Character Driver");
MODULE_AUTHOR("Aldwin Hermanudin <aldwin@hermanudin.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:ws2in13b");