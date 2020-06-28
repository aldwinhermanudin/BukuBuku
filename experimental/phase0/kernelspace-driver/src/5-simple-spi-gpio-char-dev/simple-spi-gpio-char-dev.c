
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

#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function

#define  DEVICE_NAME "ebbchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver

#define ERR(...) pr_err("epd: "__VA_ARGS__)

static int    majorNumber;                  ///< Stores the device number -- determined automatically
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

struct ws2in13b_platform_data {
	int gpio_reset;
	int gpio_dc;
	int gpio_busy;
};

static struct ws2in13b_platform_data gpio_data =
{
	.gpio_reset = -1,
	.gpio_dc = -1,
	.gpio_busy = -1,
};
static unsigned char gpio_status = 0;


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

	gpio_data = data;

	printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");
	// Try to dynamically allocate a major number for the device -- more difficult but worth it
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber<0){
		printk(KERN_ALERT "EBBChar failed to register a major number\n");
		return majorNumber;
	}
	printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);

	// Register the device class
	ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
	}
	printk(KERN_INFO "EBBChar: device class registered correctly\n");

	// Register the device driver
	ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
		class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create the device\n");
		return PTR_ERR(ebbcharDevice);
	}
	printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized

	/* setup gpio_dc */
	if (!gpio_is_valid(gpio_data.gpio_dc)){
		printk(KERN_INFO "GPIO_TEST: invalid LED GPIO\n");
		return -ENODEV;
	}
	gpio_request(gpio_data.gpio_dc, "DCGPIO");
	gpio_direction_output(gpio_data.gpio_dc, gpio_status);
	gpio_export(gpio_data.gpio_dc, false);
	/* setup gpio_dc */

	/* setup gpio_reset */
	if (!gpio_is_valid(gpio_data.gpio_reset)){
		printk(KERN_INFO "GPIO_TEST: invalid LED GPIO\n");
		return -ENODEV;
	}
	gpio_request(gpio_data.gpio_reset, "ResetGPIO");
	gpio_direction_output(gpio_data.gpio_reset, gpio_status);
	gpio_export(gpio_data.gpio_reset, false);
	/* setup gpio_reset */

	printk(KERN_INFO "EBBChar: GPIO %d %d initialized\n", gpio_data.gpio_dc, gpio_data.gpio_reset);

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

	device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
	class_unregister(ebbcharClass);                          // unregister the device class
	class_destroy(ebbcharClass);                             // remove the device class
	unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number

	gpio_set_value(gpio_data.gpio_reset, 0);              // Turn the LED off, makes it clear the device was unloaded
	gpio_unexport(gpio_data.gpio_reset);                  // Unexport the LED GPI
	gpio_free(gpio_data.gpio_reset);                      // Free the LED GPIO

	gpio_set_value(gpio_data.gpio_busy, 0);              // Turn the LED off, makes it clear the device was unloaded
	gpio_unexport(gpio_data.gpio_busy);                  // Unexport the LED GPI
	gpio_free(gpio_data.gpio_busy);                      // Free the LED GPIO

	printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");

}


static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   if (gpio_status) {
      error_count = copy_to_user(buffer, "1", 1);
   } else {
      error_count = copy_to_user(buffer, "0", 1);
   }
   if (error_count==0){            // if true then have success
      printk(KERN_INFO "EBBChar: Current GPIO DC is %d \n", gpio_status);
      return 0;  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   if ( len > 0 && buffer[0] == '1'){
      printk(KERN_INFO "EBBChar: Setting GPIO DC to 1 \n");
      gpio_set_value(gpio_data.gpio_dc, 1);
      gpio_set_value(gpio_data.gpio_reset, 1);
	  gpio_status=1;
   } else if ( len > 0 && buffer[0] == '0') {
      printk(KERN_INFO "EBBChar: Setting GPIO DC to 0 \n");
      gpio_set_value(gpio_data.gpio_dc, 0);
      gpio_set_value(gpio_data.gpio_reset, 0);
	  gpio_status=0;
   }

   printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "EBBChar: Device successfully closed\n");
   return 0;
}

module_exit(ws2in13b_exit);

MODULE_DESCRIPTION("Waveshare 2in13b Character Driver");
MODULE_AUTHOR("Aldwin Hermanudin <aldwin@hermanudin.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:ws2in13b");