/*
 * ALSA SoC PCM Interface for A380
 *
 * (C) Copyright 2018 Faraday Technology
 * Bing-Yao <bjluo@faraday-tech.com>
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

#include <linux/dmaengine.h>
#include <linux/module.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>

#include "ftssp010-i2s.h"

static struct dma_chan *ftssp010_pcm_request_chan(struct snd_soc_pcm_runtime *rtd,
                                                  struct snd_pcm_substream *ss)
{
	struct device *dev = ss->pcm->card->dev;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct ftssp010_i2s *i2s = snd_soc_dai_get_drvdata(cpu_dai);
	struct snd_dmaengine_dai_dma_data *dma_params;
	struct dma_chan *dma_chan;
	dma_filter_fn filter_fn;
	void *filter_data;

	dma_params = snd_soc_dai_get_dma_data(cpu_dai, ss);

	if (ss->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		i2s->playback_dma_slave.id = 0;
		i2s->playback_dma_slave.channels = 0xff;
		i2s->playback_dma_slave.handshake = i2s->playback_dma_data.slave_id;
		filter_data = (void *)&i2s->playback_dma_slave;
	} else {
		i2s->playback_dma_slave.id = 0;
		i2s->capture_dma_slave.channels = 0xff;
		i2s->capture_dma_slave.handshake = i2s->capture_dma_data.slave_id;
		filter_data = (void *)&i2s->capture_dma_slave;
	}

	filter_fn = ftdmac030_chan_filter;
	dma_chan = snd_dmaengine_pcm_request_channel(filter_fn, filter_data);
	if (!dma_chan) {
		dev_err(dev, "Failed to allocate DMA channel\n");
		return NULL;
	}

	dev_info(dev, "%s uses %s for DMA\n",
	         (ss->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "Playback" : "Capture",
	         dma_chan_name(dma_chan));

	return dma_chan;
}

static int ftssp010_pcm_prepare_slave_config(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct dma_slave_config *slave_config)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_dmaengine_dai_dma_data *dma_data;
	int ret;

	dma_data = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

	ret = snd_hwparams_to_dma_slave_config(substream, params, slave_config);
	if (ret)
		return ret;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		slave_config->dst_addr = dma_data->addr;
	else
		slave_config->src_addr = dma_data->addr;

	slave_config->dst_maxburst = dma_data->maxburst;
	slave_config->src_maxburst = dma_data->maxburst;
	slave_config->dst_addr_width = dma_data->addr_width;
	slave_config->src_addr_width = dma_data->addr_width;
	slave_config->slave_id = dma_data->slave_id;

	return 0;
}

/**
 * struct snd_pcm_hardware - Hardware information.
 * @buffer_bytes_max: max size of buffer in bytes
 * @period_bytes_min: min size of period in bytes
 * @period_bytes_max: max size of period in bytes
 * @periods_min: min # of periods in the buffer
 * @periods_max: max # of periods in the buffer
 * @fifo_size: not used
 *
 * This structure will be copied to runtime->hw by snd_soc_set_runtime_hwparams()
 * runtime->hw is in turn used by snd_pcm_hw_constraints_complete()
 */
static struct snd_pcm_hardware ftssp010_pcm_hardware = {
	.info   = SNDRV_PCM_INFO_MMAP
	          | SNDRV_PCM_INFO_MMAP_VALID
	          | SNDRV_PCM_INFO_INTERLEAVED
	          | SNDRV_PCM_INFO_BATCH,
	.buffer_bytes_max   = 0x40000,
	.period_bytes_min   = 1,
	.period_bytes_max   = 0xffffff,
	.periods_min        = 2,
	.periods_max        = 256,
	.fifo_size          = 0,
};

static const struct snd_dmaengine_pcm_config ftssp010_dmaengine_pcm_config = {
	.compat_request_channel = ftssp010_pcm_request_chan,
	.prepare_slave_config   = ftssp010_pcm_prepare_slave_config,
	.pcm_hardware           = &ftssp010_pcm_hardware,
	.prealloc_buffer_size   = 0x40000,
};

int ftssp010_asoc_dma_platform_register(struct device *dev)
{
	return devm_snd_dmaengine_pcm_register(dev,
	                                       &ftssp010_dmaengine_pcm_config,
	                                       SND_DMAENGINE_PCM_FLAG_COMPAT);
}
EXPORT_SYMBOL_GPL(ftssp010_asoc_dma_platform_register);

MODULE_AUTHOR("Bing-Yao Luo <bjluo@faraday-tech.com>");
MODULE_DESCRIPTION("A380 PCM DMA module");
MODULE_LICENSE("GPL");
