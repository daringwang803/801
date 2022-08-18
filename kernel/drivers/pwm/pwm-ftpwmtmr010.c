/*
 * FARADAY (PWM) core
 *
 *
 * Copyright (C) 2018 Mark Fu-Tsung Hsu <mark_hs@faraday-tech.com>
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/math64.h>
#include <linux/of_device.h>

struct ftpwmtmr010_pwm_chip {
	struct pwm_chip chip;
	struct clk      *clk;
	void __iomem    *mmio;
	uint32_t        hz;     /* source clock */
};

struct ftpwmtmr010_regs {
	uint32_t isr;
	uint32_t rsvd[3];

	struct {
		uint32_t ctrl;  /* Control */
#define CTRL_EXTCLK     (1 << 0) /* use external clock */
#define CTRL_START      (1 << 1) /* timer start */
#define CTRL_UPDATE     (1 << 2) /* update timer counter */
#define CTRL_OUT_INVT   (1 << 3) /* update timer counter */
#define CTRL_AUTORELOAD (1 << 4) /* auto-reload timer counter */
#define CTRL_INTEN      (1 << 5) /* interrupt enabled */
#define CTRL_PWMEN      (1 << 8) /* interrupt enabled */

		uint32_t cntb;  /* Count buffer */
		uint32_t cmpb;  /* Compare buffer */
		uint32_t cnto;  /* Count observation */
	} timer[8];

	uint32_t revr;  /* 0x90: Revision register */
};

static inline struct ftpwmtmr010_pwm_chip *to_ftpwmtmr010_pwm_chip(
					struct pwm_chip *chip)
{
        return container_of(chip, struct ftpwmtmr010_pwm_chip, chip);
}

static int ftpwmtmr010_pwm_set_polarity(struct pwm_chip *chip,
				      struct pwm_device *pwm,
				      enum pwm_polarity polarity)
{
	struct ftpwmtmr010_pwm_chip *ftc_pwm_chip = to_ftpwmtmr010_pwm_chip(chip);
	struct ftpwmtmr010_regs *regs = ftc_pwm_chip->mmio;
	unsigned int val;

	val = readl(&regs->timer[pwm->hwpwm].ctrl);

	if (polarity == PWM_POLARITY_INVERSED)
		val |= CTRL_OUT_INVT;
	else
		val &= ~CTRL_OUT_INVT;

	writel(val, &regs->timer[pwm->hwpwm].ctrl);

	return 0;
}
static int ftpwmtmr010_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns)
{
	struct ftpwmtmr010_pwm_chip *ftc_pwm_chip = to_ftpwmtmr010_pwm_chip(chip);
	struct ftpwmtmr010_regs *regs = ftc_pwm_chip->mmio;
	unsigned long long tmp;
	unsigned int ratio_period = 0;
	unsigned int ratio_duty = 0;
	unsigned int freq;

	if(period_ns)   /* ratio_period = 1000000000 / period_ns; */
	{
		tmp = 1000000000;
		do_div(tmp, period_ns);
		ratio_period = tmp;
	}
	if(duty_ns)     /* ratio_duty = 1000000000 / duty_ns; */
	{
		tmp = 1000000000;
		do_div(tmp, duty_ns);
		ratio_duty = tmp;
	}

	if(ratio_period)
	{
		freq = ftc_pwm_chip->hz /ratio_period;
		writel(freq, &regs->timer[pwm->hwpwm].cntb);
	}
	if(ratio_duty)
	{
		freq = ftc_pwm_chip->hz/ratio_duty;
		writel(freq, &regs->timer[pwm->hwpwm].cmpb);
	}

	return 0;
}

static int ftpwmtmr010_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ftpwmtmr010_pwm_chip *ftc_pwm_chip = to_ftpwmtmr010_pwm_chip(chip);
	struct ftpwmtmr010_regs *regs = ftc_pwm_chip->mmio;
	unsigned int val;

	val = readl(&regs->timer[pwm->hwpwm].ctrl);
	val |= CTRL_AUTORELOAD | CTRL_UPDATE | CTRL_START |CTRL_PWMEN;
	writel(val, &regs->timer[pwm->hwpwm].ctrl);

	return 0;
}

static void ftpwmtmr010_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ftpwmtmr010_pwm_chip *ftc_pwm_chip = to_ftpwmtmr010_pwm_chip(chip);
	struct ftpwmtmr010_regs *regs = ftc_pwm_chip->mmio;
	unsigned int val;

	val = readl(&regs->timer[pwm->hwpwm].ctrl);
	val &= ~CTRL_PWMEN;
	writel(val, &regs->timer[pwm->hwpwm].ctrl);
}

static struct pwm_ops ftpwmtmr010_pwm_ops = {
	.config = ftpwmtmr010_pwm_config,
	.enable = ftpwmtmr010_pwm_enable,
	.disable = ftpwmtmr010_pwm_disable,
	.set_polarity = ftpwmtmr010_pwm_set_polarity,
	.owner = THIS_MODULE,
};

static const struct of_device_id ftpwmtmr010_pwm_dt_ids[] = {
        { .compatible = "faraday,ftpwmtmr010-pwm", },
        { }
};
MODULE_DEVICE_TABLE(of, ftpwmtmr010_pwm_dt_ids);

static int ftpwmtmr010_pwm_probe(struct platform_device *pdev)
{
	struct ftpwmtmr010_pwm_chip *pwm;
	struct resource *res;
	int ret;

	pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &ftpwmtmr010_pwm_ops;
	pwm->chip.base = pdev->id;
	pwm->chip.npwm = 8;
	pwm->chip.of_xlate = of_pwm_xlate_with_flags;
	pwm->chip.of_pwm_n_cells = 3;

	/* ftpwmtmr010 clock source */
#ifdef CONFIG_OF
	pwm->clk = devm_clk_get(&pdev->dev, NULL);
#else
	pwm->clk = devm_clk_get(&pdev->dev, "osc");
#endif
	if (IS_ERR(pwm->clk))
		return PTR_ERR(pwm->clk);
	pwm->hz = clk_get_rate(pwm->clk);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	pwm->mmio = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pwm->mmio))
		return PTR_ERR(pwm->mmio);

	dev_dbg(&pdev->dev, "pwm base = %p\n", pwm->mmio);

	ret = pwmchip_add(&pwm->chip);
	if (ret < 0)
		return ret;

	platform_set_drvdata(pdev, pwm);
	dev_dbg(&pdev->dev, "pwm probe successful\n");

	return 0;
}

static int ftpwmtmr010_pwm_remove(struct platform_device *pdev)
{
	struct ftpwmtmr010_pwm_chip *pwm = platform_get_drvdata(pdev);
	int err;

	err = pwmchip_remove(&pwm->chip);
        if (err < 0)
		return err;

	dev_dbg(&pdev->dev, "ftpwmtmr010 pwm driver removed\n");

	return 0;
}

static struct platform_driver ftpwmtmr010_pwm_driver = {
	.driver = {
		.name   = "ftpwmtmr010-pwm",
		.of_match_table = ftpwmtmr010_pwm_dt_ids,
	},
	.probe = ftpwmtmr010_pwm_probe,
	.remove = ftpwmtmr010_pwm_remove,
};
module_platform_driver(ftpwmtmr010_pwm_driver);

MODULE_LICENSE("GPL v2");
