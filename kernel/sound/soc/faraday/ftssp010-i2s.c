/*
 * Faraday FTSSP010 I2S ALSA SoC Digital Audio Interface
 *
 * (C) Copyright 2018 Faraday Technology
 * Bing-Yao Luo <bjluo@faraday-tech.com>
 *
 * (C) Copyright 2010 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <asm/io.h>

#include "../codecs/wm8731.h"
#include "ftssp010.h"
#include "ftssp010-i2s.h"

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftssp010_i2s_interrupt(int irq, void *dev_id)
{
	struct ftssp010_i2s *i2s = dev_id;
	unsigned int status;

	status = ioread32(i2s->base + FTSSP010_OFFSET_ISR);

	if (status & FTSSP010_ISR_RFOR) {
		dev_info(i2s->dev, "rx fifo overrun\n");
	}

	if (status & FTSSP010_ISR_TFUR) {
		dev_info(i2s->dev, "tx fifo underrun\n");
	}
	return IRQ_HANDLED;
}

/******************************************************************************
 * struct snd_soc_ops functions
 *****************************************************************************/
/**
 * ftssp010_i2s_startup() - Open CPU DAI
 *
 * Called by soc_pcm_open().
 */
static int ftssp010_i2s_startup(struct snd_pcm_substream *ss,
		struct snd_soc_dai *dai)
{
	struct ftssp010_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = ss->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;

	/*
	 * simple-audio-card framework's asoc_simple_card_hw_params() function
	 * always calls snd_soc_dai_set_sysclk() with clk_id = 0 (WM8731_SYSCLK_MCLK).
	 * But for Faraday SOC platform, wm8731 CODEC is set to use WM8731_SYSCLK_XTAL.
	 * So, we initialize the codec_dai with WM8731_SYSCLK_XTAL here.
	 * Beacuse we do not set mclk-fs value in a369evb.dts, the simple-audio-card framework
	 * will not apply the new setting again.
	 */

	/* set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8731_SYSCLK_XTAL, i2s->sysclk, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(dai->dev, "Failed to set codec DAI system clock\n");
		return ret;
	}

	/* set the I2S system clock as input */
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, i2s->sysclk, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(dai->dev, "Failed to set CPU DAI system clock\n");
		return ret;
	}

	return 0;
}

static int ftssp010_i2s_trigger(struct snd_pcm_substream *ss, int cmd,
		struct snd_soc_dai *dai)
{
	struct ftssp010_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	int cr2, ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		iowrite32(FTSSP010_CR2_SSPRST, i2s->base + FTSSP010_OFFSET_CR2);
	case SNDRV_PCM_TRIGGER_RESUME:
		cr2 = (ioread32(i2s->base + FTSSP010_OFFSET_CR2) |
		       FTSSP010_CR2_SSPEN | FTSSP010_CR2_TXDOE);

		iowrite32(i2s->cr0, i2s->base + FTSSP010_OFFSET_CR0);
		iowrite32(i2s->cr1, i2s->base + FTSSP010_OFFSET_CR1);
		iowrite32(i2s->icr, i2s->base + FTSSP010_OFFSET_ICR);
		if (ss->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			iowrite32((cr2 | FTSSP010_CR2_TXEN | FTSSP010_CR2_TXFCLR),
			          i2s->base + FTSSP010_OFFSET_CR2);
		} else {
			iowrite32((cr2 | FTSSP010_CR2_RXEN | FTSSP010_CR2_RXFCLR),
			          i2s->base + FTSSP010_OFFSET_CR2);
		}

		dev_dbg(i2s->dev, "%s: START\n", __func__);
		dev_dbg(i2s->dev, "\t[CR0] = %x\n", i2s->cr0);
		dev_dbg(i2s->dev, "\t[CR1] = %x\n", i2s->cr1);
		dev_dbg(i2s->dev, "\t[ICR] = %x\n", i2s->icr);
		if (ss->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			dev_dbg(i2s->dev, "\t[CR2] = %x\n",
			        (cr2 | FTSSP010_CR2_TXEN | FTSSP010_CR2_TXFCLR));
		} else {
			dev_dbg(i2s->dev, "\t[CR2] = %x\n",
			        (cr2 | FTSSP010_CR2_RXEN) | FTSSP010_CR2_RXFCLR);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		cr2 = ioread32(i2s->base + FTSSP010_OFFSET_CR2);
		if (ss->stream == SNDRV_PCM_STREAM_PLAYBACK)
			cr2 &= (~FTSSP010_CR2_TXEN);
		else
			cr2 &= (~FTSSP010_CR2_RXEN);
		cr2 &= (~FTSSP010_CR2_SSPEN);

		iowrite32(i2s->cr0, i2s->base + FTSSP010_OFFSET_CR0);
		iowrite32(i2s->cr1, i2s->base + FTSSP010_OFFSET_CR1);
		iowrite32(i2s->icr, i2s->base + FTSSP010_OFFSET_ICR);
		iowrite32(cr2, i2s->base + FTSSP010_OFFSET_CR2);

		dev_dbg(i2s->dev, "%s: STOP\n", __func__);
		dev_dbg(i2s->dev, "\t[CR0] = %x\n", i2s->cr0);
		dev_dbg(i2s->dev, "\t[CR1] = %x\n", i2s->cr1);
		dev_dbg(i2s->dev, "\t[ICR] = %x\n", i2s->icr);
		dev_dbg(i2s->dev, "\t[CR2] = %x\n", cr2);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/**
 * ftssp010_i2s_hw_params() - Setup CPU DAI resources according to hardware parameters
 *
 * Called by soc_pcm_hw_params().
 */
static int ftssp010_i2s_hw_params(struct snd_pcm_substream *ss,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct ftssp010_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct snd_dmaengine_dai_dma_data *dma_params;
	unsigned int rate = params_rate(params);
	unsigned int channels = params_channels(params);
	unsigned int cr0;
	unsigned int cr1;
	int data_len;
	int padding;
	int sclk;
	int sclkdiv;

	dma_params = snd_soc_dai_get_dma_data(dai, ss);
	cr0 = i2s->cr0;
	switch (channels) {
	case 1:
		cr0 &= ~FTSSP010_CR0_STEREO;
		break;

	case 2:
		cr0 |= FTSSP010_CR0_STEREO;
		break;

	default:
		dev_err(dai->dev, "Invalid number of channels: %d\n",
		        channels);
		return -EINVAL;
	}

	dev_dbg(dai->dev, "%s: [CR0] = %08x\n", __func__, cr0);
	i2s->cr0 = cr0;

	/* bit size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
	case SNDRV_PCM_FORMAT_U8:
		data_len = 8;
		padding = 0;
		dma_params->addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		break;

	case SNDRV_PCM_FORMAT_S16:
	case SNDRV_PCM_FORMAT_U16:
		data_len = 16;
		padding	= 0;
		dma_params->addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		break;

	case SNDRV_PCM_FORMAT_S24:
	case SNDRV_PCM_FORMAT_U24:
		data_len = 24;
		padding = 8;
		dma_params->addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		break;

	case SNDRV_PCM_FORMAT_S32:
	case SNDRV_PCM_FORMAT_U32:
		data_len = 32;
		padding = 0;
		dma_params->addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		break;

	default:
		dev_err(dai->dev, "Format not supported\n");
		return -EINVAL;
	}

	cr1 = FTSSP010_CR1_SDL(data_len - 1) | FTSSP010_CR1_PDL(padding);

	dev_dbg(dai->dev, "%s: sysclk %d, data_len %d, padding %d, rate %d\n",
	        __func__, i2s->sysclk, data_len, padding, rate);

	sclk = 2 * (data_len + padding) * rate;
	sclkdiv = i2s->sysclk / 2 / sclk;

	if (sclk * sclkdiv * 2 != i2s->sysclk) {
		dev_err(dai->dev, "Sample rate %d not supported\n", rate);
		return -EINVAL;
	}

	cr1 |= FTSSP010_CR1_SCLKDIV(sclkdiv - 1);
	dev_dbg(dai->dev, "%s: [CR1] = %08x\n", __func__, cr1);
	i2s->cr1 = cr1;

	return 0;
}

/**
 * ftssp010_i2s_set_dai_fmt() - Setup hardware according to the DAI format
 *
 * Called by snd_soc_dai_set_fmt().
 */
static int ftssp010_i2s_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct ftssp010_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	unsigned int cr0;

	cr0 = FTSSP010_CR0_FFMT_I2S;

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		cr0 |= FTSSP010_CR0_FSDIST(1);
		break;

	case SND_SOC_DAIFMT_RIGHT_J:
		cr0 |= FTSSP010_CR0_FSDIST(0);
		cr0 |= FTSSP010_CR0_FSJSTFY;	/* padding in front of data */
		break;

	case SND_SOC_DAIFMT_LEFT_J:
		cr0 |= FTSSP010_CR0_FSDIST(0);
		break;

	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;

	case SND_SOC_DAIFMT_NB_IF:
		cr0 |= FTSSP010_CR0_FSPO;	/* frame sync active low */
		break;

	default:
		return -EINVAL;
	}

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		/*
		 * codec is clock & frame master ->
		 * interface is clock & frame slave
		 */
		break;

	case SND_SOC_DAIFMT_CBS_CFS:
		/*
		 * codec is clock & frame slave ->
		 * interface is clock & frame master
		 */
		cr0 |= FTSSP010_CR0_MASTER;
		break;

	default:
		return -EINVAL;
	}

	dev_dbg(dai->dev, "%s: [CR0] = %08x\n", __func__, cr0);
	i2s->cr0 = cr0;

	return 0;
}

/**
 * ftssp010_i2s_set_dai_sysclk() - Setup system clock
 * @dai:	CPU DAI.
 * @clk_id:	DAI specific clock ID.
 * @freq:	new clock frequency in Hz.
 * @dir:	new clock direction.
 *
 * Called by snd_soc_dai_set_sysclk().
 */
static int ftssp010_i2s_set_dai_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	struct ftssp010_i2s *i2s = snd_soc_dai_get_drvdata(dai);

	dev_dbg(dai->dev, "%s: clk_id %d freq %d\n", __func__,
			  clk_id, freq);
	i2s->sysclk = freq;
	return 0;
}

static int ftssp010_i2s_dai_probe(struct snd_soc_dai *dai)
{
	struct ftssp010_i2s *i2s = snd_soc_dai_get_drvdata(dai);

	snd_soc_dai_init_dma_data(dai, &i2s->playback_dma_data, &i2s->capture_dma_data);

	return 0;
}
/******************************************************************************
 * struct snd_soc_dai functions
 *****************************************************************************/
#define FTSSP010_I2S_RATES	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 \
				| SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 \
				| SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 \
				| SNDRV_PCM_RATE_96000)
#define FTSSP010_I2S_FORMATS	(SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U8 \
				| SNDRV_PCM_FMTBIT_S16 | SNDRV_PCM_FMTBIT_U16 \
				| SNDRV_PCM_FMTBIT_S24 | SNDRV_PCM_FMTBIT_U24 \
				| SNDRV_PCM_FMTBIT_S32 | SNDRV_PCM_FMTBIT_U32)

static struct snd_soc_dai_ops ftssp010_i2s_dai_ops = {
	.startup	= ftssp010_i2s_startup,
	.trigger	= ftssp010_i2s_trigger,
	.hw_params	= ftssp010_i2s_hw_params,
	.set_fmt	= ftssp010_i2s_set_dai_fmt,
	.set_sysclk	= ftssp010_i2s_set_dai_sysclk,
};

/*
 * SoC CPU Digital Audio Interface
 */
struct snd_soc_dai_driver ftssp010_i2s_dai = {
	.name	= "ftssp010-i2s",
	.id	= 0,
	.probe = ftssp010_i2s_dai_probe,
	.playback = {
		.stream_name = "CPU-Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= FTSSP010_I2S_RATES,
		.formats	= FTSSP010_I2S_FORMATS,
	},
	.capture = {
		.stream_name = "CPU-Capture",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= FTSSP010_I2S_RATES,
		.formats	= FTSSP010_I2S_FORMATS,
	},
	.ops	= &ftssp010_i2s_dai_ops,
};
EXPORT_SYMBOL_GPL(ftssp010_i2s_dai);

static const struct snd_soc_component_driver ftssp010_i2s_component = {
	.name = "ftssp010-i2s",
};

/**
 * ftssp010_i2s_probe() - Get hardware resource of FTSSP010
 *
 * Called by soc_probe().
 */
static int ftssp010_i2s_probe(struct platform_device *pdev)
{
	struct ftssp010_i2s *i2s;
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct clk *clk;
	int irq;
	int ret;

	i2s = devm_kzalloc(dev, sizeof(*i2s), GFP_KERNEL);
	if (!i2s) {
		dev_err(dev, "Failed to allocate private data\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2s->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(i2s->base))
		return PTR_ERR(i2s->base);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_irq(dev, irq, ftssp010_i2s_interrupt, 0,
	                       "ftssp010-i2s", i2s);
	if (ret) {
		dev_err(dev, "Failed to request irq %d\n", irq);
		return ret;
	}

	i2s->irq = irq;
	i2s->dev = dev;

	i2s->icr = FTSSP010_ICR_TFDMA | FTSSP010_ICR_TFTHOD(12) |
	           FTSSP010_ICR_RFDMA | FTSSP010_ICR_RFTHOD(4);

	/*
	 * setup dma parameters
	 */
	i2s->playback_dma_data.addr     = res->start + FTSSP010_OFFSET_DATA;
	i2s->playback_dma_data.maxburst = 1;

	i2s->capture_dma_data.addr     = res->start + FTSSP010_OFFSET_DATA;
	i2s->capture_dma_data.maxburst = 1;

	/*
	 * setup sysclk parameter
	 */
	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get sspclock\n");
		return -ENOENT;
	}

	clk_prepare_enable(clk);
	i2s->sysclk = clk_get_rate(clk);
	dev_info(&pdev->dev, "%s: sclk rate %d\n", __func__, i2s->sysclk);

	/* Register with ASoC */
	platform_set_drvdata(pdev, i2s);

	ret = devm_snd_soc_register_component(dev,
	                                      &ftssp010_i2s_component,
	                                      &ftssp010_i2s_dai, 1);
	if (ret) {
		dev_err(dev, "Failed to register DAI\n");
		return ret;
	}

#ifndef CONFIG_USE_OF
	ret = ftssp010_asoc_dma_platform_register(dev);
#else
	ret = devm_snd_dmaengine_pcm_register(dev,
	                                      NULL,
	                                      SND_DMAENGINE_PCM_FLAG_COMPAT);
#endif
	if (ret) {
		dev_err(dev, "Failed to register dmaengine pcm\n");
		return ret;
	}
	dev_info(dev, "irq %d, mapped at %pK\n", irq, i2s->base);

	return 0;
}

/**
 * ftssp010_i2s_remove() - Release hardware resource of FTSSP010
 *
 * Called by soc_remove().
 */
static int ftssp010_i2s_remove(struct platform_device *pdev)
{
	struct ftssp010_i2s *i2s = platform_get_drvdata(pdev);

	/* disable FTSSP010 */
	iowrite32(0, i2s->base + FTSSP010_OFFSET_CR2);

	return 0;
}

static const struct of_device_id ftssp010_i2s_of_match[] = {
	{ .compatible = "faraday,ftssp010-i2s", .data = NULL},
	{},
};

MODULE_DEVICE_TABLE(of, ftssp010_i2s_of_match);

static struct platform_driver ftssp010_i2s_driver = {
	.probe = ftssp010_i2s_probe,
	.remove = ftssp010_i2s_remove,
	.driver = {
		.name = "ftssp010-i2s",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ftssp010_i2s_of_match),
	},
};
module_platform_driver(ftssp010_i2s_driver);

MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_DESCRIPTION("FTSSP010 I2S ASoC Digital Audio Interface");
MODULE_LICENSE("GPL");
