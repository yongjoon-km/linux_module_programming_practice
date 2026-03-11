# My Uptime Proc File 

Under /proc directory, kernel can create a file and provice information about
running kernel module or system like `/proc/uptime` or `/proc/version`.
In this practice, I created my own proc file called my_uptime which simply
print how many time pass after the my_uptime module is loaded.

## Installation
```bash
make # build
```

```bash
sudo insmod main.ko # load the module
sudo rmmod main.ko  # unload the module
```

## How to create a proc file

First you need to define function and create a `struct proc_ops` object. And you
can create proc file by calling `proc_create` function. I just attached simple
read function to display uptime when userspace tried to read `/proc/my_uptime`

I was intrigued when I realized read function is for output and write function
is input on kernel side. This is because we see these functions from userspace.

## How make proc file readible

The read function signature is the below for proc_ops.

```c
ssize_t (*proc_read)(struct file *, char __user *, size_t, loff_t *);
```
The second argument is a buffer and we need to put string to it so that user
can see when they read our proc file. The problem is as you can see `__user`
means it is userspace buffer and we are working on kernel space. The two space
are segregated.

To copy our result into userspace buffer, we need to use `copy_to_user`
function. I can access this by include `linux/uaccess.h` and it allows us to
access userspace memory.

There is another method to use `seq_file` and I didn't use it in this practice
because printing uptime is not that complicated. The seq_file provides
structured way to implement proc file for complex data printing etc.

## Reference
- https://sysprog21.github.io/lkmpg/#the-proc-filesystem
- https://lwn.net/Articles/22355/
