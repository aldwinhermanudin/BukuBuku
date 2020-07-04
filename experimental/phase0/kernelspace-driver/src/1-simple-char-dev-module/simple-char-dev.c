#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#define MAX_DEV 1
#define DEV_ID 0
#define DEVICE_NAME "epdchar"    ///< The device will appear at /dev/epdchar using this value
#define CLASS_NAME  "epd"        ///< The device class -- this is a character device driver

static int mychardev_open(struct inode *inode, struct file *file);
static int mychardev_release(struct inode *inode, struct file *file);
static ssize_t mychardev_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
static ssize_t mychardev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

#define MSG_SIZE 256
static char   message[MSG_SIZE] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened

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

// change permission for /dev/mychar0 to 666 
static int mychardev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init mychardev_init(void)
{
   int err;
   dev_t dev;
   static struct device* temp_dev = NULL; ///< The device-driver device struct pointer

   err = alloc_chrdev_region(&dev, 0, MAX_DEV, DEVICE_NAME);
   if (err < 0){
      printk(KERN_ALERT "MyChar failed to register a major number\n");
      return err;
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

   return 0;
}

static void __exit mychardev_exit(void)
{

    device_destroy(mychardev_class, MKDEV(dev_major, DEV_ID));

    class_unregister(mychardev_class);
    class_destroy(mychardev_class);

    unregister_chrdev_region(MKDEV(dev_major, 0), MINORMASK);
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

   if (count > size_of_message) {
      count = size_of_message;
   }

   if ( count == 0 ){
      return count;
   }

   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buf, message, count);

   if (error_count==0){            // if true then have success
      printk(KERN_INFO "EPDChar: Sent %d characters to the user\n", count);
      size_of_message=0;  // clear the position to the start
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
   int ret;

   pr_info("should write %zu bytes (*ppos=%lld)\n", count, *ppos);

   if ( *ppos >= MSG_SIZE ) return -EINVAL;

   /* Check for end-of-buffer */
   if (*ppos + count >= MSG_SIZE)
      count = MSG_SIZE - *ppos;

   /* Get data from the user space */
   // this will move ppos
   ret = copy_from_user(message + *ppos, buf, count);
   size_of_message = count;
   if (ret < 0)
      return -EFAULT;

    *ppos += count;
   pr_info("got %zu bytes (*ppos=%lld)\n", count, *ppos);
   printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", count);

   return count;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleg Kutkov <elenbert@gmail.com>");

module_init(mychardev_init);
module_exit(mychardev_exit);