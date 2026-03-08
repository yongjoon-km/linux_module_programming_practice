#include <linux/init.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/gpio/consumer.h>

#define IO_LED 17
#define IO_OFFSET 512

static struct gpio_desc *led;
static bool st_sync_wait; // If true, when SYN event happens led should be updated
static struct input_dev* idev;
static int led_value; // 0 off, 1 on

static int init_gpio_led(void) {
    int status;
    led = gpio_to_desc(IO_LED + IO_OFFSET);
    if (!led) {
        pr_info("gpio - Error getting pin %d\n", IO_LED);
        return -ENODEV;
    }
    status = gpiod_direction_output(led, 0);
    if (status) {
        pr_info("gpio - Error setting pin %d to output\n", IO_LED);
        return status;
    }

    led_value = 1;
    gpiod_set_value(led, led_value);
    return 0;
}

static void kbd_spy_event(struct input_handle *handle, unsigned int type,
        unsigned int code, int value) {
    struct input_dev* dev = handle->dev;
    if (type == EV_ABS && (code == ABS_X || code == ABS_Y)) {
        pr_info("touchscreen event - device name: %s type: %d code: %d value: %d\n", dev->name, type, code, value);
        st_sync_wait = true;
        if (idev == NULL) {
            idev = dev;
        }
    }
    if (type == EV_SYN && idev == dev) {
        if (st_sync_wait == true && code == SYN_REPORT) {
            pr_info("led change by SYN_REPORT - device name: %s type: %d code: %d value: %d\n", dev->name, type, code, value);
            st_sync_wait = false;
            led_value = !led_value;
            gpiod_set_value(led, led_value);
        }
    }
}

static int kbd_spy_connect(struct input_handler *handler, struct input_dev *dev,
        const struct input_device_id *id) {
    struct input_handle *handle;
    int error;

    handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
    if (!handle) return -ENOMEM;

    handle->dev = dev;
    handle->handler = handler;
    handle->name = "kbd_spy_handle";

    error = input_register_handle(handle);
    if (error) {
        kfree(handle);
        return error;
    }

    error = input_open_device(handle);
    if (error) {
        input_unregister_handle(handle);
        kfree(handle);
        return error;
    }

    pr_info("connect to device %s\n", dev->name);
    return 0;
}

static void kbd_spy_disconnect(struct input_handle *handle) {
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(handle);
}

static const struct input_device_id kbd_spy_ids[] = {
    { .driver_info = 1 },
    { },
};

static struct input_handler kbd_spy_handler = {
    .event = kbd_spy_event,
    .connect = kbd_spy_connect,
    .disconnect = kbd_spy_disconnect,
    .name = "kbd_spy",
    .id_table = kbd_spy_ids,
};

static int __init my_init(void) {
    pr_info("hello world\n");
    init_gpio_led();
    input_register_handler(&kbd_spy_handler);
    return 0;
}

static void __exit my_exit(void) {
    pr_info("bye\n");
    input_unregister_handler(&kbd_spy_handler);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yongjoon");
MODULE_DESCRIPTION("test");
