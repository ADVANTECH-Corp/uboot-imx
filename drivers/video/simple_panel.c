// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2016 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 */

#include <common.h>
#include <backlight.h>
#include <dm.h>
#include <panel.h>
#include <asm/gpio.h>
#include <power/regulator.h>
#include <dm/devres.h>
#include <mipi_dsi.h>
#include <mipi_display.h>

struct dsi_ctrl_hdr {
	u8 dtype;	/* data type */
	u8 wait;	/* ms */
	u8 dlen;	/* payload len */
} __packed;

struct dsi_cmd_desc {
	struct dsi_ctrl_hdr dchdr;
	u8 *payload;
};

struct dsi_panel_cmds {
	u8 *buf;
	int blen;
	struct dsi_cmd_desc *cmds;
	int cmd_cnt;
};

struct simple_panel_priv {
	struct udevice *reg;
	struct udevice *backlight;
	struct gpio_desc enable;

	/* Advantech */
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
	struct dsi_panel_cmds *init_cmds;
};

#if defined(CONFIG_VIDEO_MIPI_DSI)
static int panel_simple_dsi_parse_dcs_cmds(struct udevice *dev,
					   const u8 *data, int blen,
					   struct dsi_panel_cmds *pcmds)
{
	int len;
	char *buf, *bp;
	struct dsi_ctrl_hdr *dchdr;
	int i, cnt;

	if (!pcmds)
		return -EINVAL;

	buf = kmemdup(data, blen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* scan dcs commands */
	bp = buf;
	len = blen;
	cnt = 0;
	while (len > sizeof(*dchdr)) {
		dchdr = (struct dsi_ctrl_hdr *)bp;

		if (dchdr->dlen > len) {
			dev_err(dev, "%s: error, len=%d", __func__,
				dchdr->dlen);
			return -EINVAL;
		}

		bp += sizeof(*dchdr);
		len -= sizeof(*dchdr);
		bp += dchdr->dlen;
		len -= dchdr->dlen;
		cnt++;
	}

	if (len != 0) {
		dev_err(dev, "%s: dcs_cmd=%x len=%d error!",
			__func__, buf[0], blen);
		kfree(buf);
		return -EINVAL;
	}

	pcmds->cmds = kcalloc(cnt, sizeof(struct dsi_cmd_desc), GFP_KERNEL);
	if (!pcmds->cmds) {
		kfree(buf);
		return -ENOMEM;
	}

	pcmds->cmd_cnt = cnt;
	pcmds->buf = buf;
	pcmds->blen = blen;

	bp = buf;
	len = blen;
	for (i = 0; i < cnt; i++) {
		dchdr = (struct dsi_ctrl_hdr *)bp;
		len -= sizeof(*dchdr);
		bp += sizeof(*dchdr);
		pcmds->cmds[i].dchdr = *dchdr;
		pcmds->cmds[i].payload = bp;
		bp += dchdr->dlen;
		len -= dchdr->dlen;
	}

	dev_info(dev, "%s: dcs_cmd=%x len=%d, cmd_cnt=%d\n", __func__,
		 pcmds->buf[0], pcmds->blen, pcmds->cmd_cnt);
	return 0;
}

static int panel_simple_dsi_send_cmds(struct mipi_dsi_device *dsi,
				      struct udevice *dev,
				      struct dsi_panel_cmds *cmds)
{
	int i, err;

	if (!cmds)
		return -EINVAL;

	for (i = 0; i < cmds->cmd_cnt; i++) {
		struct dsi_cmd_desc *cmd = &cmds->cmds[i];
		switch (cmd->dchdr.dtype) {
		case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
		case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
		case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		case MIPI_DSI_GENERIC_LONG_WRITE:
			err = mipi_dsi_generic_write(dsi, cmd->payload,
						     cmd->dchdr.dlen);
			break;
		case MIPI_DSI_DCS_SHORT_WRITE:
		case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		case MIPI_DSI_DCS_LONG_WRITE:
			err = mipi_dsi_dcs_write_buffer(dsi, cmd->payload,
							cmd->dchdr.dlen);
			break;
		default:
			return -EINVAL;
		}

		if (err < 0)
			dev_err(dev, "Warren failed to write dcs cmd: %d\n", err);

		if (cmd->dchdr.wait)
			udelay(cmd->dchdr.wait * 1000);
			//msleep(cmd->dchdr.wait);
	}

	return 0;
}
#endif

static int simple_panel_enable_backlight(struct udevice *dev)
{
	struct simple_panel_priv *priv = dev_get_priv(dev);
#if defined(CONFIG_VIDEO_MIPI_DSI)
	struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
	struct mipi_dsi_device *device = plat->device;
#endif
	int ret;

	debug("%s: start, backlight = '%s'\n", __func__, priv->backlight->name);
	dm_gpio_set_value(&priv->enable, 1);
	ret = backlight_enable(priv->backlight);
	debug("%s: done, ret = %d\n", __func__, ret);
	if (ret)
		return ret;

#if defined(CONFIG_VIDEO_MIPI_DSI)
	device->lanes = priv->lanes;
	device->format = priv->format;
	device->mode_flags = priv->mode_flags;

	if (device->mode_flags != 0 && \
	    device->format != 0 && \
	    device->lanes != 0)
	{
		ret = mipi_dsi_attach(device);

		if (ret < 0)
		{
			printf("[%s@%d] mipi_dsi_attach failure (%d)\n", __func__, __LINE__, ret);
			return ret;
		}

		if (priv->init_cmds) {
			dev_info(dev, "DSI panel init\n");

			ret = panel_simple_dsi_send_cmds(device, dev, priv->init_cmds);
			if (ret)
				dev_err(dev, "failed to send init cmds\n");
		}
	}
#endif

	return 0;
}

static int simple_panel_set_backlight(struct udevice *dev, int percent)
{
	struct simple_panel_priv *priv = dev_get_priv(dev);
	int ret;

	debug("%s: start, backlight = '%s'\n", __func__, priv->backlight->name);
	dm_gpio_set_value(&priv->enable, 1);
	ret = backlight_set_brightness(priv->backlight, percent);
	debug("%s: done, ret = %d\n", __func__, ret);
	if (ret)
		return ret;

	return 0;
}

static int simple_panel_ofdata_to_platdata(struct udevice *dev)
{
	struct simple_panel_priv *priv = dev_get_priv(dev);
	int ret;
#if defined(CONFIG_VIDEO_MIPI_DSI)
	const void *data;
	int len;
#endif

	if (IS_ENABLED(CONFIG_DM_REGULATOR)) {
		ret = uclass_get_device_by_phandle(UCLASS_REGULATOR, dev,
						   "power-supply", &priv->reg);
		if (ret) {
			debug("%s: Warning: cannot get power supply: ret=%d\n",
			      __func__, ret);
			if (ret != -ENOENT)
				return ret;
		}
	}
	ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev,
					   "backlight", &priv->backlight);
	if (ret) {
		debug("%s: Cannot get backlight: ret=%d\n", __func__, ret);
		return log_ret(ret);
	}
	ret = gpio_request_by_name(dev, "enable-gpios", 0, &priv->enable,
				   GPIOD_IS_OUT);
	if (ret) {
		debug("%s: Warning: cannot get enable GPIO: ret=%d\n",
		      __func__, ret);
		if (ret != -ENOENT)
			return log_ret(ret);
	}

#if defined(CONFIG_VIDEO_MIPI_DSI)
	/* Advantech */
	ret = dev_read_u32(dev, "simple,dsi-lanes", &priv->lanes);
	if (!ret) {
		if (priv->lanes < 1 || priv->lanes > 4) {
			debug("Invalid dsi-lanes: %d\n", priv->lanes);
			return -EINVAL;
		}
	}

	dev_read_u32(dev, "simple,dsi-format", &priv->format);
	dev_read_u32(dev, "simple,dsi-flags", &priv->mode_flags);

	data = ofnode_get_property(dev->node, "panel-init-sequence", &len);
	if (data) {
		priv->init_cmds = devm_kzalloc(dev,
					      sizeof(*priv->init_cmds),
					      GFP_KERNEL);
		if (!priv->init_cmds)
			return -ENOMEM;

		ret = panel_simple_dsi_parse_dcs_cmds(dev, data, len,
						      priv->init_cmds);
		if (ret) {
			dev_err(dev, "failed to parse panel init sequence\n");
			return ret;
		}

	}
#endif

	return 0;
}

static int simple_panel_probe(struct udevice *dev)
{
	struct simple_panel_priv *priv = dev_get_priv(dev);
	int ret;

	if (IS_ENABLED(CONFIG_DM_REGULATOR) && priv->reg) {
		debug("%s: Enable regulator '%s'\n", __func__, priv->reg->name);
		ret = regulator_set_enable(priv->reg, true);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct panel_ops simple_panel_ops = {
	.enable_backlight	= simple_panel_enable_backlight,
	.set_backlight		= simple_panel_set_backlight,
};

static const struct udevice_id simple_panel_ids[] = {
	{ .compatible = "simple-panel" },
	{ .compatible = "auo,b133xtn01" },
	{ .compatible = "auo,b116xw03" },
	{ .compatible = "auo,b133htn01" },
	{ .compatible = "lg,lb070wv8" },
	{ }
};

U_BOOT_DRIVER(simple_panel) = {
	.name	= "simple_panel",
	.id	= UCLASS_PANEL,
	.of_match = simple_panel_ids,
	.ops	= &simple_panel_ops,
	.ofdata_to_platdata	= simple_panel_ofdata_to_platdata,
	.probe		= simple_panel_probe,
	.priv_auto_alloc_size	= sizeof(struct simple_panel_priv),
};
