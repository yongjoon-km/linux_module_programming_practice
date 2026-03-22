#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/timer.h>

static struct timer_list my_timer;

// TODO: Change to dynamic array by number of cpu
static struct kernel_cpustat kcs_prev[4];
static struct kernel_cpustat kcs_curr[4];

static u64 get_cpu_usage(struct kernel_cpustat prev, struct kernel_cpustat curr) {
    u64 idle_time =
        curr.cpustat[CPUTIME_IDLE] - prev.cpustat[CPUTIME_IDLE]
        + curr.cpustat[CPUTIME_IOWAIT] - prev.cpustat[CPUTIME_IOWAIT];
    u64 total_time =
        curr.cpustat[CPUTIME_USER] - prev.cpustat[CPUTIME_USER]
        + curr.cpustat[CPUTIME_NICE] - prev.cpustat[CPUTIME_NICE]
        + curr.cpustat[CPUTIME_SYSTEM] - prev.cpustat[CPUTIME_SYSTEM]
        + curr.cpustat[CPUTIME_SOFTIRQ] - prev.cpustat[CPUTIME_SOFTIRQ]
        + curr.cpustat[CPUTIME_IRQ] - prev.cpustat[CPUTIME_IRQ]
        + curr.cpustat[CPUTIME_STEAL] - prev.cpustat[CPUTIME_STEAL]
        + idle_time
        ;

    return div64_u64((total_time - idle_time) * 100, total_time);
}


static void load_cpu_stat(struct kernel_cpustat *kcs_list) {
    int i;
    for_each_possible_cpu(i) {
		kcpustat_cpu_fetch(&kcs_list[i], i);
	}

}

static void my_callback(struct timer_list *t) {
    load_cpu_stat(kcs_curr);
    int i;
    for_each_possible_cpu(i) {
        u64 cpu_usage = get_cpu_usage(kcs_prev[i], kcs_curr[i]);
        pr_info("print usage of cpu %d, usage: %lld\n", i, cpu_usage);
        // copy kcs_curr to kcs_prev
        kcs_prev[i] = kcs_curr[i];
    }

    mod_timer(&my_timer, jiffies + HZ);
}

static int __init my_init(void) {
    pr_info("hello world\n");
    load_cpu_stat(kcs_prev);
    timer_setup(&my_timer, my_callback, 0);
    mod_timer(&my_timer, jiffies + HZ);
    pr_info("just debug message\n");
    return 0;
}

static void __exit my_exit(void) {
    pr_info("bye\n");
    del_timer_sync(&my_timer);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yongjoon");
MODULE_DESCRIPTION("test");
