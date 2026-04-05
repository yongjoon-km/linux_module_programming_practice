#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

struct cpu_led {
    struct pwm_device   *pwm;
    char                label[32];
    int                 cpu_index;
};

struct cpu_leds_priv {
    struct cpu_led          leds[4];
    int                     num_leds;
    struct timer_list       timer;
    struct kernel_cpustat   prev[4];
    struct kernel_cpustat   curr[4];
    bool                    shutting_down;
};

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
    struct cpu_leds_priv *priv = from_timer(priv, t, timer);
    struct pwm_state state;
    int i;

    if (priv->shutting_down)
        return;

    load_cpu_stat(priv->curr);
    for (i = 0; i < priv->num_leds; i++) {
        int cpu = priv->leds[i].cpu_index;
        u64 usage = get_cpu_usage(priv->prev[cpu], priv->curr[cpu]);

        pwm_get_state(priv->leds[i].pwm, &state);
        state.duty_cycle = state.period * usage / 100;
        pwm_apply_atomic(priv->leds[i].pwm, &state);
        pr_info("cpu%d: %llu%% -> duty %llu/%llu\n", cpu, usage, state.duty_cycle, state.period);
        priv->prev[cpu] = priv->curr[cpu];
    }

    mod_timer(&priv->timer, jiffies + HZ);
}

static int cpu_leds_probe(struct platform_device *pdev)
{
    struct cpu_leds_priv *priv;
    struct device *dev = &pdev->dev;
    const char *names[] = { "led0", "led1", "led2", "led3" };
    int i;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->num_leds = 4;

    for (i = 0; i < priv->num_leds; i++) {
        priv->leds[i].pwm = pwm_get(dev, names[i]);
        if (IS_ERR(priv->leds[i].pwm)) {
            pr_err("pwm_get(%s) failed: %ld\n", names[i], PTR_ERR(priv->leds[i].pwm));
            while (--i >= 0)
                pwm_put(priv->leds[i].pwm);
            return PTR_ERR(priv->leds[i].pwm);
        }
        priv->leds[i].cpu_index = i;
        struct pwm_state state;
        pwm_init_state(priv->leds[i].pwm, &state);
        state.period = 1000000;
        state.duty_cycle = 500000;
        state.enabled = true;
        pwm_apply_might_sleep(priv->leds[i].pwm, &state);

        pr_info("led[%d] label=%s ready\n", i, priv->leds[i].label);
    }

    platform_set_drvdata(pdev, priv);
    load_cpu_stat(priv->prev);
    timer_setup(&priv->timer, my_callback, 0);
    mod_timer(&priv->timer, jiffies + HZ);
    return 0;
}

static void cpu_leds_remove(struct platform_device *pdev)
{
    pr_info("remove invoked\n");
    struct cpu_leds_priv *priv = platform_get_drvdata(pdev);
    struct pwm_state state;
    int i;

    priv->shutting_down = true;

    del_timer_sync(&priv->timer);

    for (i = 0; i < priv->num_leds; i++) {
        pwm_get_state(priv->leds[i].pwm, &state);
        state.duty_cycle = 0;
        state.enabled = false;
        pwm_apply_atomic(priv->leds[i].pwm, &state);
        pwm_put(priv->leds[i].pwm);
    }

    pr_info("cpu_leds: removed cleanly\n");
}

static void cpu_leds_shutdown(struct platform_device *pdev)
{
    pr_info("shutdown invoked\n");
    cpu_leds_remove(pdev);
}

static const struct of_device_id of_my_pwm_leds_match[] = {
	{ .compatible = "my-cpu-led", },
	{},
};

MODULE_DEVICE_TABLE(of, of_my_pwm_leds_match);

static struct platform_driver my_pwm_driver = {
	.probe		= cpu_leds_probe,
    .remove     = cpu_leds_remove,
    .shutdown   = cpu_leds_shutdown,
	.driver		= {
		.name	= "cpu-led",
		.of_match_table = of_my_pwm_leds_match,
	},
};

module_platform_driver(my_pwm_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yongjoon");
MODULE_DESCRIPTION("test");
