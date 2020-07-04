
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#define MAX_DEV 1
#define DEV_ID 0
#define DEVICE_NAME "ws2in13bchar"    ///< The device will appear at /dev/ws2in13bchar using this value
#define CLASS_NAME  "ws2in13b"        ///< The device class -- this is a character device driver

#define WS2IN13B_WIDTH 104
#define WS2IN13B_HEIGHT 212
#define WS2IN13B_PIXELS (WS2IN13B_WIDTH/8) * WS2IN13B_HEIGHT

// data that will be used inter-function in this code
static int    numberOpens = 0;              	///< Counts the number of times the device is opened
static char ws2in13b_fb[WS2IN13B_PIXELS];		// act as the simple framebuffer for the display

// forward declation for the character device operations
static int mychardev_open(struct inode *inode, struct file *file);
static int mychardev_release(struct inode *inode, struct file *file);
static ssize_t mychardev_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t mychardev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static const struct file_operations mychardev_fops = {
    .owner      = THIS_MODULE,
    .open       = mychardev_open,
    .release    = mychardev_release,
    .read       = mychardev_read,
    .write      = mychardev_write
};

struct mychar_device_data {
    struct cdev cdev;
};

static int dev_major = 0;
static struct class *mychardev_class = NULL;
static struct mychar_device_data mychardev_data;

static struct spi_device *ws2in13b_spi;

#define ERR(...) pr_err("epd: "__VA_ARGS__)

struct ws2in13b_platform_data {
	int gpio_reset;
	int gpio_dc;
	int gpio_busy;
};

/* display specific operation */
static struct ws2in13b_platform_data gpio_data =
{
	.gpio_reset = -1,
	.gpio_dc = -1,
	.gpio_busy = -1,
};

static void ws2in13b_reset(void){
	gpio_set_value(gpio_data.gpio_reset, 1);
	mdelay(200);
	gpio_set_value(gpio_data.gpio_reset, 0);
	mdelay(10);
	gpio_set_value(gpio_data.gpio_reset, 1);
	mdelay(200);
}

static void ws2in13b_send_cmd(unsigned char buf){
	gpio_set_value(gpio_data.gpio_dc, 0);
	spi_write(ws2in13b_spi, &buf,1);
}

static void ws2in13b_send_data(unsigned char buf){
	gpio_set_value(gpio_data.gpio_dc, 1);
	spi_write(ws2in13b_spi, &buf,1);
}

static void ws2in13b_read_busy(void){
	printk(KERN_ALERT "%s: e-Paper busy\n", __FUNCTION__);
	ws2in13b_send_cmd(0x71);

	while(gpio_get_value(gpio_data.gpio_busy) == 0){
		ws2in13b_send_cmd(0x71);
		cpu_relax();
	}
	printk(KERN_ALERT "%s: e-Paper busy release\n", __FUNCTION__);
}

enum ink_type {
	red_ink,
	black_ink
};

static void ws2in13b_display(enum ink_type type, const char* img, size_t len){
	int i;

	if ( type == black_ink ){
		ws2in13b_send_cmd(0x10);
		for (i = 0; i < len; i++){
			ws2in13b_send_data(img[i]);
		}
	} else if ( type == red_ink ){
		ws2in13b_send_cmd(0x13);
		for (i = 0; i < len; i++){
			ws2in13b_send_data(img[i]);
		}
	}
	ws2in13b_send_cmd(0x12); // REFRESH
	mdelay(100);
	ws2in13b_read_busy();
}

static void ws2in13b_clear(void){
	unsigned int i;
	ws2in13b_send_cmd(0x10);
	for( i = 0; i < WS2IN13B_PIXELS; i++ ) ws2in13b_send_data(0xff);

	ws2in13b_send_cmd(0x13);
	for( i = 0; i < WS2IN13B_PIXELS; i++ ) ws2in13b_send_data(0xff);

	ws2in13b_send_cmd(0x12); // refresh the display
	mdelay(100);
	ws2in13b_read_busy();
}

static void ws2in13b_init(void){
	
	ws2in13b_reset();
	ws2in13b_send_cmd(0x04);
	ws2in13b_read_busy(); //waiting for the electronic paper IC to release the idle signal

	ws2in13b_send_cmd(0x00); // panel setting
	ws2in13b_send_data(0x0f);   // LUT from OTP,128x296
	ws2in13b_send_data(0x89);   // Temperature sensor, boost and other related timing settings

	ws2in13b_send_cmd(0x61);    // resolution setting
	ws2in13b_send_data(0x68);
	ws2in13b_send_data(0x00);
	ws2in13b_send_data(0xD4);

	ws2in13b_send_cmd(0x50);    // VCOM AND DATA INTERVAL SETTING
	ws2in13b_send_data(0x77);       // WBmode:VBDF 17|D7 VBDW 97 VBDB 57
						// WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
}

static void ws2in13b_deinit(void){

	ws2in13b_send_cmd(0x50);
	ws2in13b_send_data(0xf7);

	ws2in13b_send_cmd(0x02);
	ws2in13b_read_busy();

	ws2in13b_send_cmd(0x07); // deep sleep
	ws2in13b_send_data(0xA5); // check code

	gpio_set_value(gpio_data.gpio_reset, 0);
	gpio_set_value(gpio_data.gpio_dc, 0);
}
/* display specific operation */

/* fetching data from DT */
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
/* fetching data from DT */

// change permission for /dev/mychar0 to 666 
static int mychardev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

// setup the character device
static int setup_char_dev(void){
	int ret;
	dev_t dev;
	static struct device* temp_dev = NULL; ///< The device-driver device struct pointer

	// dynamically create character device, by asking the OS for a major number
	ret = alloc_chrdev_region(&dev, 0, MAX_DEV, DEVICE_NAME);
	if (ret < 0){
		printk(KERN_ALERT "MyChar failed to register a major number\n");
		return ret;
	}

	dev_major = MAJOR(dev);

	mychardev_class = class_create(THIS_MODULE, CLASS_NAME);

	if (IS_ERR(mychardev_class)){                // Check for error and clean up if there is
		unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(mychardev_class);          // Correct way to return an error on a pointer
	}

	mychardev_class->dev_uevent = mychardev_uevent;

	cdev_init(&mychardev_data.cdev, &mychardev_fops);
	mychardev_data.cdev.owner = THIS_MODULE;

	cdev_add(&mychardev_data.cdev, MKDEV(dev_major, DEV_ID), 1);

	temp_dev = device_create(mychardev_class, NULL, MKDEV(dev_major, DEV_ID), NULL, DEVICE_NAME "%d", DEV_ID);

	// this error handling have not been tested yet
	if (IS_ERR(temp_dev)){               // Clean up if there is an error

		class_destroy(mychardev_class);          // Repeated code but the alternative is goto statements
		
		unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
		
		printk(KERN_ALERT "Failed to create the device\n");
		pr_err("%s:%d error code %ld\n", __func__, __LINE__, PTR_ERR(temp_dev));
		return PTR_ERR(temp_dev);
	}
	/* setup character device */
	return 0;
}

static int setup_gpio(void){

	/* setup gpio */
	/* setup gpio_dc */
	if (!gpio_is_valid(gpio_data.gpio_dc)){
		printk(KERN_INFO "GPIO_TEST: invalid D/C GPIO\n");
		return -ENODEV;
	}
	gpio_request(gpio_data.gpio_dc, "sysfs");
	gpio_direction_output(gpio_data.gpio_dc, 0);
	gpio_export(gpio_data.gpio_dc, false);
	/* setup gpio_dc */

	/* setup gpio_reset */
	if (!gpio_is_valid(gpio_data.gpio_reset)){
		printk(KERN_INFO "GPIO_TEST: invalid Rest GPIO\n");
		return -ENODEV;
	}
	gpio_request(gpio_data.gpio_reset, "sysfs");
	gpio_direction_output(gpio_data.gpio_reset, 0);
	gpio_export(gpio_data.gpio_reset, false);
	/* setup gpio_reset */

	/* setup gpio_busy */
	if (!gpio_is_valid(gpio_data.gpio_busy)){
		printk(KERN_INFO "GPIO_TEST: invalid Busy GPIO\n");
		return -ENODEV;
	}
	gpio_request(gpio_data.gpio_busy, "sysfs");
	gpio_direction_input(gpio_data.gpio_busy);
	gpio_export(gpio_data.gpio_busy, false);

	printk(KERN_INFO "GPIO_TEST: The BUSY state is currently: %d\n", gpio_get_value(gpio_data.gpio_busy));
	/* setup gpio_reset */
	/* setup gpio */

	return 0;
}

static int ws2in13b_probe(struct spi_device *spi){
	/* object to setup display */
	unsigned int i;

	/* SPI driver temporary objects */
	int ret = 0;
	struct ws2in13b_platform_data *pdata;

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	spi->max_speed_hz = 1*1000*1000;

	printk(KERN_INFO "Setting-up SPI device\n");
	ret = spi_setup(spi);
	printk(KERN_INFO "Succeeded to register SPI device\n");

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
		pdata = &gpio_data;
		ret = ws2in13b_probe_dt(&spi->dev, pdata);

		if(ret < 0) {
			ERR("Fail to get platform data\n");
			return ret;
		}
	}

	printk(KERN_INFO "D/C Pin is %d\n", gpio_data.gpio_dc);
	printk(KERN_INFO "Busy Pin is %d\n", gpio_data.gpio_busy);
	printk(KERN_INFO "Reset Pin is %d\n", gpio_data.gpio_reset);
	printk(KERN_INFO "Succeeded to setup SPI device\n");
	ws2in13b_spi = spi;

	ret = setup_char_dev();
	if ( ret != 0 ) return ret;

	ret = setup_gpio();
	if ( ret != 0 ) return ret;
	
	printk(KERN_INFO "%s: initializing up Waveshare 2in13b display\n",__FUNCTION__);
	for (i = 0; i < WS2IN13B_PIXELS; i++){
		ws2in13b_fb[i] = 0xff;
	}
	ws2in13b_init();
	printk(KERN_INFO "%s: initialize process done\n",__FUNCTION__);

	return 0;
}

static int ws2in13b_remove(struct spi_device *spi)
{
	printk(KERN_INFO "%s: removing SPI device resource \n",__FUNCTION__);
	
	printk(KERN_INFO "%s: sleeping display \n",__FUNCTION__);
	ws2in13b_deinit();

	/* destroy Character Device */
	printk(KERN_INFO "%s: destroying character device \n",__FUNCTION__);
    device_destroy(mychardev_class, MKDEV(dev_major, DEV_ID));

    class_unregister(mychardev_class);
    class_destroy(mychardev_class);

    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
	/* destroy Character Device */

	/* destroy GPIO */
	printk(KERN_INFO "%s: destroying GPIO \n",__FUNCTION__);
	printk(KERN_INFO "%s: freeing D/C GPIO \n",__FUNCTION__);
	gpio_set_value(gpio_data.gpio_dc, 0);              // Turn the LED off, makes it clear the device was unloaded
	gpio_unexport(gpio_data.gpio_dc);                  // Unexport the LED GPI
	gpio_free(gpio_data.gpio_dc);                      // Free the LED GPIO

	printk(KERN_INFO "%s: freeing Reset GPIO \n",__FUNCTION__);
	gpio_set_value(gpio_data.gpio_reset, 0);              // Turn the LED off, makes it clear the device was unloaded
	gpio_unexport(gpio_data.gpio_reset);                  // Unexport the LED GPI
	gpio_free(gpio_data.gpio_reset);                      // Free the LED GPIO

	printk(KERN_INFO "%s: freeing Busy GPIO \n",__FUNCTION__);
	gpio_unexport(gpio_data.gpio_busy);                  // Unexport the LED GPI
	gpio_free(gpio_data.gpio_busy);                      // Free the LED GPIO
	/* destroy GPIO */

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

static int __init mychardev_init(void)
{
	printk(KERN_INFO "Registering SPI device\n");
	return spi_register_driver(&ws2in13b_driver);
}

static void __exit mychardev_exit(void)
{
	printk(KERN_INFO "De-registering SPI device\n");
	spi_unregister_driver(&ws2in13b_driver);
}

static int mychardev_open(struct inode *inode, struct file *file)
{
    printk("EPDChar: Device open\n");
    numberOpens++;
    printk(KERN_INFO "EPDChar: Device has been opened %d time(s)\n", numberOpens);
    return 0;
}

static int mychardev_release(struct inode *inode, struct file *file)
{
    printk("EPDChar: Device close\n");
    return 0;
}

static ssize_t mychardev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{  
   int error_count = 0;

   printk("Reading device: %zu\n", MINOR(file->f_path.dentry->d_inode->i_rdev));

   if (count > WS2IN13B_PIXELS) {
      count = WS2IN13B_PIXELS;
   }

   if ( count == 0 ){
      return count;
   }

   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buf, ws2in13b_fb, count);

   if (error_count==0){            // if true then have success
      printk(KERN_INFO "EPDChar: Sent %d characters to the user\n", count);
      return count; 
   }
   else {
      printk(KERN_INFO "EPDChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

static ssize_t mychardev_write(struct file *filp,
                            const char __user *buf, 
                            size_t count, 
                            loff_t *ppos)
{
	int 			ret;
	unsigned int	i;
	static char   	message[WS2IN13B_PIXELS];

	pr_info("should write %zu bytes (*ppos=%lld)\n", count, *ppos);

	if (count == 0 ) return 0;

	if ( *ppos >= WS2IN13B_PIXELS ) return -EINVAL;

	/* Check for end-of-buffer */
	if (*ppos + count >= WS2IN13B_PIXELS)
		count = WS2IN13B_PIXELS - *ppos;

	/* Get data from the user space */
	// this will move ppos
	ret = copy_from_user(message + *ppos, buf, count);

	if (ret < 0)
		return -EFAULT;

	*ppos += count;
	pr_info("got %zu bytes (*ppos=%lld)\n", count, *ppos);
	printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", count);

	// very in-efficient way to copy temporary data buffer to the display "framebuffer"
	for (i = 0; i < count; i++){
	  ws2in13b_fb[i] = message[i];
	}

	// clear the display for both red and black ink before drawing anything
	ws2in13b_clear();

	// for this example only draw in red ink
	ws2in13b_display(red_ink, ws2in13b_fb, WS2IN13B_PIXELS);

	return count;
}

MODULE_DESCRIPTION("Waveshare 2in13b KISS Character Driver");
MODULE_AUTHOR("Aldwin Hermanudin <aldwin@hermanudin.com>");
MODULE_LICENSE("GPL");

module_init(mychardev_init);
module_exit(mychardev_exit);