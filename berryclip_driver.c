#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/fs.h>

#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>

#define DRIVER_AUTHOR "Javier and Francisco"
#define DRIVER_DESC "Device driver for berryclip"

#define DEVICE_LEDS "leds"
#define DEVICE_BUTTONS "buttons"
#define DEVICE_SPEAKER "speaker"

#define GPIO_DEFAULT 512

#define NUM_LEDS 6
#define NUM_GPIOS 9

#define GPIO_RED_LED1    9
#define GPIO_RED_LED2    10
#define GPIO_YELLOW_LED1 11
#define GPIO_YELLOW_LED2 17
#define GPIO_GREEN_LED1  22
#define GPIO_GREEN_LED2  27

#define GPIO_BUTTON1 2
#define GPIO_BUTTON2 3

#define GPIO_SPEAKER 4

#define BUTTONS_BUFF_SIZE 16

static DECLARE_WAIT_QUEUE_HEAD(buttons_waitqueue);

int n_gpios_conf[NUM_GPIOS];

int leds_pin[] = {GPIO_RED_LED1, GPIO_RED_LED2, GPIO_YELLOW_LED1, GPIO_YELLOW_LED2, GPIO_GREEN_LED1, GPIO_GREEN_LED2};
int gpios[] = {GPIO_RED_LED1, GPIO_RED_LED2, GPIO_YELLOW_LED1, GPIO_YELLOW_LED2, GPIO_GREEN_LED1, GPIO_GREEN_LED2, GPIO_BUTTON1, GPIO_BUTTON2, GPIO_SPEAKER};
char *gpio_name[] = {"red_led1", "red_led2", "yellow_led1", "yellow_led2", "green_led1", "green_led2", "button1", "button2", "speaker"};
static uint8_t leds_state = 0;


// === LEDS DEVICE FUNCTIONS ===

static ssize_t leds_read(struct file *file, char __user *buf, size_t len, loff_t *ppos) {
    uint8_t val = leds_state;

    if(*ppos == 0) *ppos+=1;
    else return 0;

    printk(KERN_NOTICE "Val salida = %d\n", val);

    return copy_to_user(buf, &val, 1) ? -EFAULT : 1;
}

static ssize_t leds_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
    uint8_t val;

    if(*ppos == 0) *ppos+=1;
    else return len;

    if(copy_from_user(&val, buf, 1))
	return -EFAULT;

    printk(KERN_NOTICE "Val introducido = %c  %d\n", val, (int)val);

    uint8_t type = (val & 0xC0) >> 6;
    uint8_t bits = val & 0x3F;

    switch(type){
	case 0: leds_state = bits; break;
	case 1: leds_state |= bits; break;
	case 2: leds_state &= ~bits; break;
	case 3: leds_state ^= bits; break;
    }

    printk(KERN_NOTICE "Leds State = %d\n", leds_state);

    for(int i = 0; i < NUM_LEDS; ++i){
	gpio_set_value(GPIO_DEFAULT + leds_pin[i], (leds_state >> i) & 1);
    }

    return 1;
}

//========= BUTTONS DEVICE FUNCTIONS =========

static int buttons_open(struct inode *inode, struct file *flip)
{
    if(flip->f_flags & O_NONBLOCK)
    {

    }else{

    }
    return 0;
}

static int buttons_release(struct inode *inode, struct file *flip)
{
    return 0;
}

static ssize_t buttons_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
    return 0;
}

//========= SPEAKER DEVICE FUNCTIONS =========

static ssize_t speaker_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
    char val;

    if(copy_from_user(&val, buf, 1)) return -EFAULT;
	printk(KERN_NOTICE "Val speaker = %d\n", val);

    gpio_set_value(GPIO_DEFAULT + GPIO_SPEAKER, val != '0' ? 1 : 0);

    return 1;
}

//========= FILE OPERATIONS =========

static const struct file_operations leds_fops =
{
    .owner	= THIS_MODULE,
    .read	= leds_read,
    .write	= leds_write,
};

static const struct file_operations buttons_fops =
{
    .owner      = THIS_MODULE,
    .open	= buttons_open,
    .release	= buttons_release,
    .read       = buttons_read,
};

static const struct file_operations speaker_fops =
{
    .owner      = THIS_MODULE,
    .write      = speaker_write,
};

//========= DEVICE DECLARATION =========

static struct miscdevice leds_dev =
{
    .minor      = MISC_DYNAMIC_MINOR,
    .name       = DEVICE_LEDS,
    .fops       = &leds_fops,
    .mode       = S_IRUGO | S_IWUGO,
};

static struct miscdevice buttons_dev =
{
    .minor      = MISC_DYNAMIC_MINOR,
    .name       = DEVICE_BUTTONS,
    .fops       = &buttons_fops,
    .mode       = S_IRUGO,
};

static struct miscdevice speaker_dev =
{
    .minor      = MISC_DYNAMIC_MINOR,
    .name       = DEVICE_SPEAKER,
    .fops       = &speaker_fops,
    .mode       = S_IWUGO,
};

static void dev_config_mesg(struct miscdevice dev, char *name, int ret)
{
    if(ret < 0)
    {
	printk(KERN_ERR "misc_register for %s failed\n", name);
    }else{
	printk(KERN_NOTICE "misc_register OK... %s minor=%d\n", name, dev.minor);
    }

}

static int r_devices_config(void)
{
    int ret = 0;
    ret = misc_register(&leds_dev);

    dev_config_mesg(leds_dev, DEVICE_LEDS, ret);

    ret = misc_register(&buttons_dev);
    dev_config_mesg(buttons_dev, DEVICE_BUTTONS, ret);

    ret = misc_register(&speaker_dev);
    dev_config_mesg(speaker_dev, DEVICE_SPEAKER, ret);

    return ret;
}

static int r_GPIO_config(void)
{
    int ret = 0;

    /*for (int i = 0; i < NUM_LEDS; ++i){
	ret = gpio_request(GPIO_DEFAULT + leds_pin[i], "led");
	if(ret < 0){
	    printk(KERN_ERR "GPIO request failure: led with GPIO %d", GPIO_DEFAULT + leds_pin[i]);
	    return ret;
	}
	ret = gpio_direction_output(GPIO_DEFAULT + leds_pin[i], 0);
	if(ret < 0){
            printk(KERN_ERR "GPIO set direction output failure: led with GPIO %d", GPIO_DEFAULT + leds_pin[i]);
            return ret;
        }
    }

    gpio_request(GPIO_DEFAULT + GPIO_SPEAKER, "speaker");
    gpio_direction_output(GPIO_DEFAULT + GPIO_SPEAKER, 0);

    gpio_request(GPIO_DEFAULT + GPIO_BUTTON1, "button1");
    gpio_request(GPIO_DEFAULT + GPIO_BUTTON2, "button2");
    gpio_direction_output(GPIO_DEFAULT + GPIO_BUTTON1, 0);
    gpio_direction_output(GPIO_DEFAULT + GPIO_BUTTON2, 0);*/

    for(int i = 0; i < NUM_GPIOS; ++i){
	ret = gpio_request(GPIO_DEFAULT + gpios[i], gpio_name[i]);
	if(ret < 0){
	    printk(KERN_ERR "GPIO request failure: %s with GPIO %d", gpio_name[i], GPIO_DEFAULT + gpios[i]);
	    return ret;
	}
	ret = gpio_direction_output(GPIO_DEFAULT + gpios[i], 0);
	if(ret < 0){
            printk(KERN_ERR "GPIO set direction output failure: %s with GPIO %d", gpio_name[i] ,GPIO_DEFAULT + gpios[i]);
            return ret;
        }
	n_gpios_conf[i] = 1;
    }

    return 0;
}

//========= INIT AND CLEANUP FUNCTIONS =========

static void cleanup_driver(void)
{

    printk(KERN_NOTICE "Cleaning up module '%s'\n", KBUILD_MODNAME);

    if(leds_dev.this_device) misc_deregister(&leds_dev);
    if(buttons_dev.this_device) misc_deregister(&buttons_dev);
    if(speaker_dev.this_device) misc_deregister(&speaker_dev);


    for(int i = 0; i < NUM_GPIOS; ++i){
	if(n_gpios_conf[i] == 1) gpio_free(GPIO_DEFAULT + gpios[i]);
    }

    printk(KERN_NOTICE "Done. Bye from module '%s'\n", KBUILD_MODNAME);

    return;
}

static int init_driver(void)
{
    int res;

    printk(KERN_NOTICE "%s: module loading\n", KBUILD_MODNAME);

    printk(KERN_NOTICE "%s:  devices config\n", KBUILD_MODNAME);

    if ((res = r_devices_config())) {
        printk(KERN_ERR "%s:  failed\n", KBUILD_MODNAME);
		cleanup_driver();
        return res;
    }

    printk(KERN_NOTICE "%s:  GPIO config\n", KBUILD_MODNAME);

    if((res = r_GPIO_config())) {
        printk(KERN_ERR "%s:  failed\n", KBUILD_MODNAME);
        cleanup_driver();
        return res;
    }

    return 0;
}

module_init (init_driver);
module_exit (cleanup_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
