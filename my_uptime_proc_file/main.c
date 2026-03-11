#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time64.h>
#include <linux/kernel.h>

#define PROCFS_NAME "my_uptime"

static struct proc_dir_entry *our_proc_file;

struct timespec64 module_start_time;

static ssize_t procfile_read(struct file *file_pointer, char __user *buffer,
        size_t buffer_length, loff_t *offset)
{
    struct timespec64 now;
    ktime_get_real_ts64(&now);
    struct timespec64 diff = timespec64_sub(now, module_start_time);
    
    char s[100];
    snprintf(s, 100, "%lld.%09ld\n", diff.tv_sec, diff.tv_nsec);

    int len = sizeof(s);
    ssize_t ret = len;

    if (*offset >= len) {
        ret = 0;
    } else if (copy_to_user(buffer, s, len)) {
        ret = -1;
    } else {
        pr_info("procfile read %s\n", file_pointer->f_path.dentry->d_name.name);
        *offset += len;
    }

    return ret;
}

static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};

static int __init my_init(void) {
    pr_info("hello world\n");
    ktime_get_real_ts64(&module_start_time);
    our_proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_fops);
    if (our_proc_file == NULL) {
        pr_info("Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    pr_info("/proc/%s created\n", PROCFS_NAME);
    return 0;
}

static void __exit my_exit(void) {
    proc_remove(our_proc_file);
    pr_info("bye\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yongjoon");
MODULE_DESCRIPTION("test");
