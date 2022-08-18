/*
 *  linux/arch/arm/mach-faraday/audio.c
 *
 *  Copyright (C) 2009 Faraday Technology
 *  Copyright (C) 2011 Dante Su <dantesu@faraday-tech.com>
 *  Copyright (C) 2015 B.C. Chen <bcchen@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <asm/clkdev.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <mach/hardware.h>

#ifdef CONFIG_PINMUX_DMB3
#include <linux/mfd/wm8350/core.h>
#endif

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "../../../../sound/soc/codecs/wm8731.h"

#define CFG_I2S_MCLK	CONFIG_SND_SOC_WM8731_MCLK

static int platform_snd_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	/*
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	u16 reg;
	*/
	printk("platform snd startup\n");
	
#if 0
	/*
	 * Clear PWROFF and CLKOUTPD, everything else as-is
	 *
	 * The A369 audio clock routing is shown as bellow:
	 *
	 * 1. SCLK_CFG1: BIT4 is set
	 *    12.288MHz Crystal --> (XTI/MCLK) WM8731
	 *    WM8731 (CLKOUT)   --> FTSSP010
	 * 2. SCLK_CFG1: BIT4 is cleared
	 *    12.288MHz Crystal --> (XTI/MCLK) WM8731
	 *    APB               --> FTSSP010
	 * However The wm8731 codec driver in Linux power-down CLKOUT by default.
	 * And thus we have to turn it on right here.
	 */
	reg = snd_soc_read(codec, WM8731_PWR) & 0xff3f;
	snd_soc_write(codec, WM8731_PWR, reg);
#else
	/*
	 * Dante Su 2012.03.07
	 * Power on everything except Line-IN to eliminate noise at 1st playback with aplay (i.e. alsa-utils + alsa-lib).
	 */
	snd_soc_write(codec, WM8731_PWR, 0x0001);
#endif

	return 0;
}

static void platform_snd_shutdown(struct snd_pcm_substream *substream)
{
	/*
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;

	mutex_lock(&codec->mutex);

	snd_soc_write(codec, WM8731_PWR, 0x7f);

	mutex_unlock(&codec->mutex);
	*/
}

static int platform_snd_hw_params(struct snd_pcm_substream *substream, 
                                  struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8731_SYSCLK_XTAL, CONFIG_SND_SOC_WM8731_MCLK,
		SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	/* set the I2S system clock as input */
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, CFG_I2S_MCLK,
		SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops platform_snd_ops = {
	.startup   = platform_snd_startup,
	.shutdown  = platform_snd_shutdown,
	.hw_params = platform_snd_hw_params,
};

/* platform machine dapm widgets */
static const struct snd_soc_dapm_widget wm8731_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
	SND_SOC_DAPM_LINE("Line Jack", NULL),
};

/* platform machine audio map (connections to the codec pins) */
static const struct snd_soc_dapm_route audio_map[] = {
	
	/* headphone connected to LHPOUT1, RHPOUT1 */
	{"Headphone Jack", NULL, "LHPOUT"},
	{"Headphone Jack", NULL, "RHPOUT"},

	/* speaker connected to LOUT, ROUT */
	{"Ext Spk", NULL, "ROUT"},
	{"Ext Spk", NULL, "LOUT"},

	/* mic is connected to MICIN (via right channel of headphone jack) */
	{"MICIN", NULL, "Mic Jack"},

	/* Same as the above but no mic bias for line signals */
	{"LLINEIN", NULL, "Line Jack"},
	{"RLINEIN", NULL, "Line Jack"},
};

static int platform_wm8731_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = snd_soc_codec_get_dapm(codec);
	/* Add platform specific widgets */
	snd_soc_dapm_new_controls(dapm, wm8731_dapm_widgets, ARRAY_SIZE(wm8731_dapm_widgets));

	/* Set up platform specific audio path audio_map */
	snd_soc_dapm_add_routes(dapm, audio_map, ARRAY_SIZE(audio_map));

	snd_soc_dapm_sync(dapm);
	return 0;
}

/* Digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link platform_wm8731_dai_links[] = {
	{
		.name = "WM8731",
		.stream_name = "WM8731 PCM",
		.cpu_dai_name = "ftssp010-i2s",			/* search key for 'dai(i.e. I2S)', see soc-core.c: soc_bind_dai_link(...)' */
		.codec_dai_name = "wm8731-hifi",		/* search key for 'codec_dai', see soc-core.c: see soc_bind_dai_link(...) */
//		.platform_name = "ftssp010-pcm-audio",	/* search key for 'pcm(i.e. DMA)', see soc-core.c: see soc_bind_dai_link(...) */
		.codec_name = "wm8731.0-001b",			/* search key for 'codec', see soc-core.c: see soc_bind_dai_link(...) */
		.init = platform_wm8731_init,
		.ops = &platform_snd_ops,
	},
};

/* Audio machine driver */
static struct snd_soc_card platform_wm8731_card = {
	.owner = THIS_MODULE,
	.name = "asoc",
	.dai_link = platform_wm8731_dai_links,
	.num_links = ARRAY_SIZE(platform_wm8731_dai_links),
};

static int platform_wm8731_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &platform_wm8731_card;
	int ret = 0;

	card->dev = &pdev->dev;

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(&pdev->dev, "devm_snd_soc_register_card() failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static struct of_device_id faraday_audio_of_match[] = {
	{ .compatible = "platform-wm8731", },
	{ },
};

static struct platform_driver platform_wm8731_driver = {
	.driver = {
		.name = "platform-wm8731",
		.owner = THIS_MODULE,
		.of_match_table = faraday_audio_of_match,
	},
	.probe = platform_wm8731_probe,
};

module_platform_driver(platform_wm8731_driver);

MODULE_AUTHOR("B.C. Chen <bcchen@faraday-tech.com>");
MODULE_DESCRIPTION("ALSA SoC for Faraday SoC based boards with wm8731 codec");
MODULE_LICENSE("GPL");
