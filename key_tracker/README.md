# Key Tracker

In user space, we can monitor what key is pressed on the keyboard with a simple
script. So we can do in the kernel space for sure. I tried to create a simple
module to print all input device events in the kernel logs.

## Installation
```bash
make # build
```

```bash
sudo insmod main.ko # load the module
sudo rmmod main.ko  # unload the module
```

## input device event handling

The event of input device data is abstracted like the below.

```c
struct input_value {
  __u16 type;
  __u16 code;
  __s32 value;
};
```

The type describes kinds of event for example EV_KEY means state changes of
keyboards, buttons, or other key-lick devices and EV_ABS means absolute axis
value changes like touchscreen touch event.

The code describes what event happens in the type above. For example, type EV_KEY
and code KEY_A means keyboard key A events happens. Whether it is pressed or re
leased are followed by last field, which is value.

The value describes the state or changes of the event. It can be key pressed or
released, mouse pointer moved diff changes.

## how it works

First of all, the input_value is atomic data structure which means it only contains
single piece of the event. For instance, If I move mouse diagonally, there will be
two input_value events EV_REL / REL_X / 5 and EV_REL / REL_Y / -5. I wonder what makes
the two is one event in the kernel.

The kernel has event type EV_SYN for this. EV_SYN has SYN_REPORT code and it means
"done for now". So the kernel can tell the two EV_REL (REL_X, REL_Y) should be treat
as a single action. In this design, If a device is moving in 3 dimension, it works
without an issue.

## Reference
- https://www.kernel.org/doc/html/latest/input/event-codes.html#input-event-codes
- https://www.kernel.org/doc/html/v4.17/driver-api/input.html#input-subsystem
