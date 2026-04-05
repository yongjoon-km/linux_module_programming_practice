# CPU USAGE WITH GPIO AND LED

The kernel has cpu stat data. User space can see this in `/proc/stat`.
Using this information, you can calculate a usage of each cpu thread.
This practice uses raspberry pi 4. It has several gpio pins and I can
connect leds attaching to gpio pins. The led itself doesn't support brightness.
But I can use pwm to adjust led brightness. There are two pwm supported gpio
pins but I use pwm-gpio driver with just normal gpio pins.

## CPU statistics

There are 10 cpu statistics you can see in `/proc/stat`.

1.  user
2.  nice
3.  system
4.  idle
5.  iowait
6.  irq
7.  softirq
8.  steal
9.  guest
10. guest_nice

The `nice` and `guest_nice` were new to me.
There is a command `nice` to run a process in different priority than others.
So the `nice` and `guest_nice` is the cpu usage of running nice tasks.
The `steal` is a stolen cpu usage on virtual os perspective like vmware.
The original host is using the physical cpu rather than for virtual os.
The guest, on the other hand, is the usage for the virtual os.

The statistics are all spent time. So to get usage (%), we need to calculate
two statistics and calculate `usage_diff / total_time_diff`.

To get each cpu information in kernel module, the kernel supports simple macro 
called `for_each_possible_cpu`. Using this, we can iterate each cpu by index.
To get cpu stat from index, you can call `kcpustat_cpu_fetch(...)`.

## Timer in linux kernel module

There are two types of timer in linux kernel module.
One is jiffies timer and another is hrtimer.
The jiffies timer is simple timer we can use in the kernel module.
I used this timer to update cpu usage in my kernel module by 1 second.

There is a global variable `jiffies` It is initialized from kernel boot and
we can calculate later time by adding `HZ` constant. The `HZ` is frequency and
indicates how many times jiffies is updated during 1 seconds. This means we
can get 1 seconds later by adding HZ to the jiffies value.

## .dts file for attaching to my device driver module

I created custom .dts file to describe what gpio pins I use for my driver.
See the `combined-pwm-led.dts` file. It defines 4 gpio nodes using pwm-gpio
driver. And the cpu-leds device node is using the 4 gpio nodes.
After insmod my driver module, If I compile and upload dtbo file to the kernel,
my driver module should be loaded properly.

## Installation

add device tree to use pwm-gpio driver for the gpio leds in raspberry pi 4.
The below is testing purpose for pwm-gpio driver.

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

To compile and upload this driver module use below.

```bash
dtc -I dts -O dtb combined-pwm-led.dts -o combined-pwm-led.dtbo
```

```bash
# dtoverlay is available in raspberry pi os
sudo dtoverlay combined-pwm-led.dtbo
```

```bash
make # build
```

```bash
sudo insmod main.ko # load the module
sudo rmmod main.ko  # unload the module
```

## Test CPU load

Please install stress-ng so that you can add some usage to the cpu.

```bash
stress-ng --random 1 --timeout 300s
```

## Reference
- https://man7.org/linux/man-pages/man5/proc_stat.5.html
- https://github.com/devicetree-org/devicetree-specification/blob/main/source/chapter2-devicetree-basics.rst
