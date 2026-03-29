# CPU USAGE WITH GPIO AND LED

TBD

## Installation

add device tree to use pwm-gpio driver for the gpio leds in raspberry pi 4.

```bash
dtc -I dts -O dtb pwm-gpio17.dts -o pwm-gpio17.dtbo
```

```bash
# dtoverlay is available in raspberry pi os
sudo dtoverlay pwm-gpio17.dtbo
```

Check the device tree is added properly

```bash
ls /proc/device-tree/pwm-gpio17/
```

Check pwm-gpio.c probed and registered a chip

```bash
ls /sys/class/pwm
```

You can test led brightness using file system like the below.

```bash
echo 0       > /sys/class/pwm/pwmchip0/export          # export channel 0 to make it accessible
echo 1000000 > /sys/class/pwm/pwmchip0/pwm0/period     # set period first (1kHz)
echo 500000  > /sys/class/pwm/pwmchip0/pwm0/duty_cycle # then duty_cycle (50%)
echo 1       > /sys/class/pwm/pwmchip0/pwm0/enable     # now enable works
```

For cleanup

```bash
echo 0 > /sys/class/pwm/pwmchip0/pwm0/enable
echo 0 > /sys/class/pwm/pwmchip0/unexport
```



```bash
make # build
```

```bash
sudo insmod main.ko # load the module
sudo rmmod main.ko  # unload the module
```

## Reference
- https://github.com/devicetree-org/devicetree-specification/blob/main/source/chapter2-devicetree-basics.rst
