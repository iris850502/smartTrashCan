#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>

#define DRIVER_NAME "tcrt5000_irq_driver"
#define DEVICE_NAME "tcrt5000_sensor"

static int gpio_pin = 594; // GPIO23 (TCRT5000 DO腳)
static int irq_number;
static struct class *tcrt5000_class;
static struct device *tcrt5000_device;

static irqreturn_t tcrt5000_irq_handler(int irq, void *dev_id)
{
    int value = gpio_get_value(gpio_pin);

    if (value == 0) { 
        // 注意：TCRT5000靠近通常是輸出Low，所以這裡用0代表有垃圾靠近
        printk(KERN_INFO "[TCRT5000 IRQ] Trash detected: Garbage close!\n");
    } else {
        printk(KERN_INFO "[TCRT5000 IRQ] Trash area clear: No garbage detected.\n");
    }

    return IRQ_HANDLED;
}

static ssize_t tcrt5000_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int value = gpio_get_value(gpio_pin);
    return sprintf(buf, "%d\n", value);
}

static DEVICE_ATTR(tcrt5000_status, 0444, tcrt5000_show, NULL);

static int __init tcrt5000_init(void)
{
    int ret;

    printk(KERN_INFO "TCRT5000: Initializing...\n");

    ret = gpio_request(gpio_pin, "tcrt5000_gpio");
    if (ret) {
        printk(KERN_ERR "TCRT5000: Failed to request GPIO %d\n", gpio_pin);
        return ret;
    }

    ret = gpio_direction_input(gpio_pin);
    if (ret) {
        printk(KERN_ERR "TCRT5000: Failed to set GPIO %d as input\n", gpio_pin);
        gpio_free(gpio_pin);
        return ret;
    }

    irq_number = gpio_to_irq(gpio_pin);
    if (irq_number < 0) {
        printk(KERN_ERR "TCRT5000: Failed to get IRQ number\n");
        gpio_free(gpio_pin);
        return irq_number;
    }

    ret = request_irq(irq_number, tcrt5000_irq_handler,
                      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                      DRIVER_NAME, NULL);
    if (ret) {
        printk(KERN_ERR "TCRT5000: Failed to request IRQ\n");
        gpio_free(gpio_pin);
        return ret;
    }

    tcrt5000_class = class_create(DRIVER_NAME); // 注意！新版本class_create只要一個參數
    if (IS_ERR(tcrt5000_class)) {
        free_irq(irq_number, NULL);
        gpio_free(gpio_pin);
        return PTR_ERR(tcrt5000_class);
    }

    tcrt5000_device = device_create(tcrt5000_class, NULL, 0, NULL, DEVICE_NAME);
    if (IS_ERR(tcrt5000_device)) {
        class_destroy(tcrt5000_class);
        free_irq(irq_number, NULL);
        gpio_free(gpio_pin);
        return PTR_ERR(tcrt5000_device);
    }

    ret = device_create_file(tcrt5000_device, &dev_attr_tcrt5000_status);
    if (ret) {
        device_destroy(tcrt5000_class, 0);
        class_destroy(tcrt5000_class);
        free_irq(irq_number, NULL);
        gpio_free(gpio_pin);
        return ret;
    }

    printk(KERN_INFO "TCRT5000 IRQ driver loaded\n");
    return 0;
}

static void __exit tcrt5000_exit(void)
{
    device_remove_file(tcrt5000_device, &dev_attr_tcrt5000_status);
    device_destroy(tcrt5000_class, 0);
    class_destroy(tcrt5000_class);
    free_irq(irq_number, NULL);
    gpio_free(gpio_pin);

    printk(KERN_INFO "TCRT5000 IRQ driver unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("你的名字");
MODULE_DESCRIPTION("TCRT5000 Trash Detector with IRQ (for Pi 5)");
MODULE_VERSION("1.1");

module_init(tcrt5000_init);
module_exit(tcrt5000_exit);

