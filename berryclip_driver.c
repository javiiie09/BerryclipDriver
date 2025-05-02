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

#define BUTTONS_BUFF_SIZE 64

int gpios_configured[NUM_GPIOS];

int leds_pin[] = {GPIO_RED_LED1, GPIO_RED_LED2, GPIO_YELLOW_LED1, GPIO_YELLOW_LED2, GPIO_GREEN_LED1, GPIO_GREEN_LED2};
int gpios[] = {GPIO_RED_LED1, GPIO_RED_LED2, GPIO_YELLOW_LED1, GPIO_YELLOW_LED2, GPIO_GREEN_LED1, GPIO_GREEN_LED2, GPIO_BUTTON1, GPIO_BUTTON2, GPIO_SPEAKER};
char *gpio_name[] = {"red_led1", "red_led2", "yellow_led1", "yellow_led2", "green_led1", "green_led2", "button1", "button2", "speaker"};

static uint8_t leds_state = 0;

// === GLOBAL BUTTONS VARIABLES ===

static DECLARE_WAIT_QUEUE_HEAD(buttons_waitqueue);

static char buffer[BUTTONS_BUFF_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;

DEFINE_SEMAPHORE(buttons_sem, 1);
DEFINE_SPINLOCK(buffer_lock);

static struct workqueue_struct *buttons_wq;
static struct delayed_work debounce_work_b1;
static struct delayed_work debounce_work_b2;
static int irq_b1 = -1, irq_b2 = -1;

static int debounce = 50;

module_param(debounce, int, S_IRUGO);


// === LEDS DEVICE FUNCTIONS ===

static ssize_t leds_read(struct file *file, char __user *buf, size_t len, loff_t *ppos) {
    uint8_t val = leds_state;

    if(*ppos == 0) *ppos+=1;
    else return 0;

    return copy_to_user(buf, &val, 1) ? -EFAULT : 1;
}

static ssize_t leds_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
    uint8_t val;

    if(*ppos == 0) *ppos+=1;
    else return len;

    if(copy_from_user(&val, buf, 1)) return -EFAULT;

    uint8_t type = (val & 0xC0) >> 6;
    uint8_t bits = val & 0x3F;

    switch(type){
	case 0: leds_state = bits; break;
	case 1: leds_state |= bits; break;
	case 2: leds_state &= ~bits; break;
	case 3: leds_state ^= bits; break;
    }

    for(int i = 0; i < ARRAY_SIZE(leds_pin); ++i){
	    gpio_set_value(GPIO_DEFAULT + leds_pin[i], (leds_state >> i) & 1);
    }

    return 1;
}

//========= BUTTONS DEVICE FUNCTIONS =========

static int buttons_open(struct inode *inode, struct file *flip)
{
    if(flip->f_flags & O_NONBLOCK){
	    if(down_trylock(&buttons_sem)) return -EBUSY;
    }else{
	    if(down_interruptible(&buttons_sem)) return -ERESTARTSYS;
    }
    return 0;
}

static int buttons_release(struct inode *inode, struct file *flip)
{
    up(&buttons_sem);
    return 0;
}

static ssize_t buttons_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
    if(buffer_head == buffer_tail){
	    if(file->f_flags & O_NONBLOCK) return -EAGAIN;

	    if(wait_event_interruptible(buttons_waitqueue, buffer_head != buffer_tail)) return -ERESTARTSYS;
    }

    if(len < 1) return -EINVAL;
    
    if(copy_to_user(buf, &buffer[buffer_tail], 1)) return -EFAULT;

    buffer_tail = (buffer_tail + 1) % sizeof(buffer);

    return 1;
}

//========= SPEAKER DEVICE FUNCTIONS =========

static ssize_t speaker_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
    char val;

    if(copy_from_user(&val, buf, 1)) return -EFAULT;

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

// ========= IRQ AND WORKQUEUE FUNCTIONS =========

static irqreturn_t button1_irq_handler(int irq, void *dev_id)
{
    if(buttons_wq) queue_delayed_work(buttons_wq, &debounce_work_b1, msecs_to_jiffies(debounce));
    return IRQ_HANDLED;
}

static irqreturn_t button2_irq_handler(int irq, void *dev_id)
{
    if(buttons_wq) queue_delayed_work(buttons_wq, &debounce_work_b2, msecs_to_jiffies(debounce));
    return IRQ_HANDLED;
}

static void debounce_work_func_b1(struct work_struct *work)
{

    if(!gpio_get_value(GPIO_DEFAULT + GPIO_BUTTON1)){
	    spin_lock(&buffer_lock);
    	buffer[buffer_head++] = '1';
	    buffer_head %= BUTTONS_BUFF_SIZE;
	    spin_unlock(&buffer_lock);
    }
    wake_up_interruptible(&buttons_waitqueue);
}

static void debounce_work_func_b2(struct work_struct *work)
{
    if(!gpio_get_value(GPIO_DEFAULT + GPIO_BUTTON2)){
	    spin_lock(&buffer_lock);
    	buffer[buffer_head++] = '2';
	    buffer_head %= BUTTONS_BUFF_SIZE;
    	spin_unlock(&buffer_lock);
    }
    wake_up_interruptible(&buttons_waitqueue);
}


// ========= DEVICES CONFIGURATION =========

static int r_devices_config(void)
{
    int ret = 0;
    ret = misc_register(&leds_dev);
    if(ret < 0){
	    printk(KERN_ERR "misc_register for leds device failed\n");
	    return ret;
    }else{
	    printk(KERN_NOTICE "misc_register OK... leds device minor=%d\n", leds_dev.minor);
    }

    ret = misc_register(&buttons_dev);
    if(ret < 0){
	    printk(KERN_ERR "misc_register for buttons device failed\n");
	    misc_deregister(&leds_dev);
	    return ret;
    }else{
	    printk(KERN_NOTICE "misc_register OK... buttons device minor=%d\n", buttons_dev.minor);
    }

    ret = misc_register(&speaker_dev);
    if(ret < 0){
        printk(KERN_ERR "misc_register for speaker device failed\n");
        misc_deregister(&buttons_dev);
        misc_deregister(&leds_dev);
        return ret;
    }else{
	    printk(KERN_NOTICE "misc_register OK... speaker device minor=%d\n", speaker_dev.minor);
    }

    return ret;
}

// ========= GPIO AND IRQS CONFIGURATION =========

static void free_gpios(void)
{
    for(int i = 0; i < ARRAY_SIZE(gpios); ++i){
	    if(gpios_configured[i]) gpio_free(GPIO_DEFAULT + gpios[i]);
    }
}

static void free_irqs(void)
{
    if(buttons_wq) destroy_workqueue(buttons_wq);

    if(irq_b1 >= 0) free_irq(irq_b1, NULL);
    if(irq_b2 >= 0) free_irq(irq_b2, NULL);
}

static int irqs_config(void)
{
    int ret;

    irq_b1 = gpio_to_irq(GPIO_DEFAULT + GPIO_BUTTON1);
    irq_b2 = gpio_to_irq(GPIO_DEFAULT + GPIO_BUTTON2);

    printk(KERN_INFO "berryclip: irq_btn1=%d, irq_btn2=%d\n", irq_b1, irq_b2);

    if(irq_b1 < 0 || irq_b2 < 0) return -EINVAL;

    ret = request_irq(irq_b1, button1_irq_handler, IRQF_TRIGGER_FALLING, "btn1_irq", NULL);
    if(ret < 0){
	    printk(KERN_ERR "berryclip: Error requesting IRQ for button1 %d\n", ret);
	    return ret;
    }
    ret = request_irq(irq_b2, button2_irq_handler, IRQF_TRIGGER_FALLING, "btn2_irq", NULL);
    if(ret < 0){
	    printk(KERN_ERR "berryclip: Error requesting IRQ for button2 %d\n", ret);
	    free_irq(irq_b1, NULL);
	    return ret;
    }

    INIT_DELAYED_WORK(&debounce_work_b1, debounce_work_func_b1);
    INIT_DELAYED_WORK(&debounce_work_b2, debounce_work_func_b2);
    if(!(buttons_wq = create_singlethread_workqueue("buttons_wq"))){
	    free_irq(irq_b1, NULL);
	    free_irq(irq_b2, NULL);
	    return -ENOMEM;
    }
    return 0;
}

static int GPIO_config(void)
{
    int ret = 0;

    for(int i = 0; i < ARRAY_SIZE(gpios); ++i){
        ret = gpio_request(GPIO_DEFAULT + gpios[i], gpio_name[i]);
        if(ret < 0){
            printk(KERN_ERR "GPIO request failure: %s with GPIO %d", gpio_name[i], GPIO_DEFAULT + gpios[i]);
            return ret;
        }
        if(strstr(gpio_name[i], "button") == NULL){
            ret = gpio_direction_output(GPIO_DEFAULT + gpios[i], 0);
        }else{
            ret = gpio_direction_input(GPIO_DEFAULT + gpios[i]);
        }
        if(ret < 0){
            printk(KERN_ERR "GPIO set direction failed: %s with GPIO %d", gpio_name[i], GPIO_DEFAULT + gpios[i]);
            return ret;
        }
        gpios_configured[i] = 1;
    }

    return 0;
}

//========= INIT AND CLEANUP FUNCTIONS =========

static void cleanup_driver(void)
{
    printk(KERN_NOTICE "Cleaning up module '%s'\n", KBUILD_MODNAME);

    free_irqs();
    free_gpios();

    misc_deregister(&leds_dev);
    misc_deregister(&buttons_dev);
    misc_deregister(&speaker_dev);

    printk(KERN_NOTICE "Done. Bye from module '%s'\n", KBUILD_MODNAME);

    return;
}

static int init_driver(void)
{
    int res;
    
    printk(KERN_NOTICE "%s: module loading. Debounce time = %d\n", KBUILD_MODNAME, debounce);

    if((res = GPIO_config())) {
        printk(KERN_ERR "%s: gpio config failed\n", KBUILD_MODNAME);
	    free_gpios();
        return res;
    }

    if((res = irqs_config())){
	    printk(KERN_ERR "%s: gpio config failed\n", KBUILD_MODNAME);
        return res;
    }

    if ((res = r_devices_config())) {
        printk(KERN_ERR "%s: device config failed\n", KBUILD_MODNAME);
        return res;
    }

    return 0;
}

module_init (init_driver);
module_exit (cleanup_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
