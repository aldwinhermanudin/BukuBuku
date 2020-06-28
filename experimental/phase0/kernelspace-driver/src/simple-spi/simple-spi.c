
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

static int ws2in13b_probe(struct spi_device *spi){
	int ret = 0;
	unsigned char buf;

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	spi->max_speed_hz = 1*1000*1000;

	printk(KERN_INFO "Setting-up SPI device\n");
	ret = spi_setup(spi);
	printk(KERN_INFO "Succeeded to register SPI device\n");

	buf = 0x04;
	spi_write(spi, &buf,1);

	if (ret < 0){
		printk(KERN_ALERT "Failed to register SPI device\n");
		return ret;
	}

	printk(KERN_INFO "Succeeded to setup SPI device\n");

return 0;
}

static int ws2in13b_remove(struct spi_device *spi)
{
	printk(KERN_INFO "Removing SPI device\n");
	// struct rtc_device *rtc = platform_get_drvdata(spi);

	// rtc_device_unregister(rtc);
	return 0;
}

static struct spi_driver ws2in13b_driver = {
	.driver = {
		.name	 = "ws2in13b",
		.owner	= THIS_MODULE,
	},
	.probe	 = ws2in13b_probe,
	.remove  = ws2in13b_remove,
};

static __init int ws2in13b_init(void)
{	
	printk(KERN_INFO "Registering SPI device\n");
	return spi_register_driver(&ws2in13b_driver);
}

module_init(ws2in13b_init);

static __exit void ws2in13b_exit(void)
{
	printk(KERN_INFO "De-registering SPI device\n");
	spi_unregister_driver(&ws2in13b_driver);
}
module_exit(ws2in13b_exit);

MODULE_DESCRIPTION("Waveshare 2in13b Character Driver");
MODULE_AUTHOR("Aldwin Hermanudin <aldwin@hermanudin.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:ws2in13b");