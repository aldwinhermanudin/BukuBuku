
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>


#define ERR(...) pr_err("epd: "__VA_ARGS__)

struct ws2in13b_platform_data {
	int gpio_reset;
	int gpio_dc;
	int gpio_busy;
};

#ifdef CONFIG_OF
static const struct of_device_id ws2in13b_dt_match[] = {
	{
		.compatible = "ws2in13b",
	},
	{},
};
MODULE_DEVICE_TABLE(of, ws2in13b_dt_match);

static int ws2in13b_probe_dt(struct device *dev, struct ws2in13b_platform_data *pdata)
{
	struct device_node *node = dev->of_node;
	struct of_device_id const *match;
	int ret = 0;

	/* Check that device tree node is ok */
	if(node == NULL) {
		ERR("Device does not have associated device tree data\n");
		ret = -EINVAL;
		goto out;
	}
	match = of_match_device(ws2in13b_dt_match, dev);
	if(match == NULL) {
		ERR("Unknown device model\n");
		ret = -EINVAL;
		goto out;
	}

	/* Get gpio for /reset */
	pdata->gpio_reset = of_get_named_gpio(node, "reset-gpios", 0);
	if(pdata->gpio_reset < 0) {
		ERR("Cannot get reset GPIO\n");
		ret = -EINVAL;
		goto out;
	}

	/* Get gpio for border */
	pdata->gpio_dc = of_get_named_gpio(node, "dc-gpios", 0);
	if(pdata->gpio_dc < 0) {
		ERR("Cannot get dc GPIO\n");
		ret = -EINVAL;
		goto out;
	}

	/* Get gpio for busy */
	pdata->gpio_busy = of_get_named_gpio(node, "busy-gpios", 0);
	if(pdata->gpio_busy < 0) {
		ERR("Cannot get busy GPIO\n");
		ret = -EINVAL;
		goto out;
	}

	printk(KERN_INFO "Succeeded getting data from DT\n");
out:
	return ret;
}
#else
static int ws2in13b_probe_dt(struct device *dev, struct ws2in13b_platform_data *pdata)
{
	return -EINVAL;
}
#endif

static int ws2in13b_probe(struct spi_device *spi){
	int ret = 0;
	unsigned char buf;
	struct ws2in13b_platform_data *pdata, data;

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

	/*
	 * Get platform data in order to get all gpios config for busy, d/c, and reset.
	 * if not found fetch it from dt.
	 */
	pdata = dev_get_platdata(&spi->dev);
	if(!pdata) {
		pdata = &data;
		ret = ws2in13b_probe_dt(&spi->dev, pdata);
		if(ret < 0) {
			ERR("Fail to get platform data\n");
			return ret;
		}
	}

	printk(KERN_INFO "D/C Pin is %d\n", data.gpio_dc);
	printk(KERN_INFO "Busy Pin is %d\n", data.gpio_busy);
	printk(KERN_INFO "Reset Pin is %d\n", data.gpio_reset);

	printk(KERN_INFO "Succeeded to setup SPI device\n");

	return 0;
}

static int ws2in13b_remove(struct spi_device *spi)
{
	printk(KERN_INFO "Removing SPI device\n");
	return 0;
}

static struct spi_driver ws2in13b_driver = {
	.driver = {
		.name	 = "ws2in13b",
		.of_match_table = of_match_ptr(ws2in13b_dt_match),
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