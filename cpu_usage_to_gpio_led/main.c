#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

static struct timer_list my_timer;

// TODO: Change to dynamic array by number of cpu
static struct kernel_cpustat kcs_prev[4];
static struct kernel_cpustat kcs_curr[4];

static struct pwm_device* pwm;

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

static int my_pwm_probe(struct platform_device *pdev)
{
    struct pwm_state state;
    int ret;

    pr_info("hello world\n");
    load_cpu_stat(kcs_prev);
    timer_setup(&my_timer, my_callback, 0);
    mod_timer(&my_timer, jiffies + HZ);
    pr_info("just debug message\n");

    pwm = pwm_get(&pdev->dev, "led");
    if (IS_ERR(pwm)) {
        pr_info("Failed to get pwm device\n");
        return -EINVAL;
    }

    // configure initial state — LED off at start
    pwm_init_state(pwm, &state);
    state.period     = 1000000;   // 1ms = 1kHz
    state.duty_cycle = 500000;
    state.enabled    = true;
    ret = pwm_apply_might_sleep(pwm, &state);
    if (ret) {
        pr_err("cpu_led: pwm_apply_state failed: %d\n", ret);
        pwm_put(pwm);
        del_timer_sync(&my_timer);
        return ret;
    }

    pr_info("Successful to get pwm device!\n");

	return 0;
}

static void my_pwm_shutdown(struct platform_device *pdev)
{
    pr_info("bye\n");
    del_timer_sync(&my_timer);
    pwm_put(pwm);
}

/*
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
    pwm_put(pwm);
}
*/

static const struct of_device_id of_my_pwm_leds_match[] = {
	{ .compatible = "my-cpu-led", },
	{},
};

MODULE_DEVICE_TABLE(of, of_my_pwm_leds_match);

static struct platform_driver my_pwm_driver = {
	.probe		= my_pwm_probe,
    .shutdown   = my_pwm_shutdown,
	.driver		= {
		.name	= "cpu-led",
		.of_match_table = of_my_pwm_leds_match,
	},
};

module_platform_driver(my_pwm_driver);

/*
module_init(my_init);
module_exit(my_exit);
*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yongjoon");
MODULE_DESCRIPTION("test");
