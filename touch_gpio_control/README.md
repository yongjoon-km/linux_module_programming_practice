# Touch GPIO Control

Previously I tried to track all input device events by input_handler.
My raspberrypi attached a touchscreen and It publish EV_ABS event whenever I
touch the screen. I tried to catch this specific event and toggle led with gpio.

To test this module, you need to prepare one led and connect to gpio number
accordingly.

## Installation
```bash
make # build
```

```bash
sudo insmod main.ko # load the module
sudo rmmod main.ko  # unload the module
```

## How to detect touchscreen event

The touchscreen event is type of EV_ABS in kernel. So first you need to check it.
But using only this value is not enough because touchscreen event consists of at
least two events code ABS_X and ABS_Y. It might work to just listen to the one code,
however I want to be more concrete for this practice.

The touchscreen device also sent EV_SYN type of event whenever the one unit of
event is sent. For example, one touch follows ABS_X, ABS_Y and SYN_REPORT. So I
can change some trigger whenver ABS_X, ABS_Y event happens and then wait SYN_RE
PORT to toggle led.

## Reference
- /usr/include/linux/input-event-codes.h
