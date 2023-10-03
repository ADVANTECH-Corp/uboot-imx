// SPDX-License-Identifier: GPL-2.0+
/*
 * TC358775 MIPI to LVDS bridge driver
 *
 * Copyright (C) 2022, Advantech Co., Ltd. - All Rights Reserved
 * Author: Ted Lin <Ted.Lin@advantech.com>
 */

#include <common.h>
#include <dm.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <linux/err.h>

#define USE_MIPI_DCS 1

#if USE_MIPI_DCS
#define DCS_CMD_LEN 6
typedef u8 cmd_set_table[DCS_CMD_LEN];
static const cmd_set_table panel_init_sequence[] =
{

    {0x3C, 0x01, 0x05, 0x00, 0x03, 0x00},
    {0x14, 0x01, 0x03, 0x00, 0x00, 0x00},
    {0x64, 0x01, 0x04, 0x00, 0x00, 0x00},
    {0x68, 0x01, 0x04, 0x00, 0x00, 0x00},
    {0x6C, 0x01, 0x04, 0x00, 0x00, 0x00},
    {0x70, 0x01, 0x04, 0x00, 0x00, 0x00},
    {0x34, 0x01, 0x1F, 0x00, 0x00, 0x00},
    {0x10, 0x02, 0x1F, 0x00, 0x00, 0x00},
    {0x04, 0x01, 0x01, 0x00, 0x00, 0x00},
    {0x04, 0x02, 0x01, 0x00, 0x00, 0x00},
    {0x50, 0x04, 0x20, 0x01, 0xF0, 0x03},
    {0x54, 0x04, 0x0A, 0x00, 0x0A, 0x00},
    {0x58, 0x04, 0x00, 0x04, 0x2C, 0x01},
    {0x5C, 0x04, 0x04, 0x00, 0x04, 0x00},
    {0x60, 0x04, 0x00, 0x03, 0x1E, 0x00},
    {0x64, 0x04, 0x01, 0x00, 0x00, 0x00},
    {0xA0, 0x04, 0x06, 0x80, 0x44, 0x00},
    {0xA0, 0x04, 0x06, 0x80, 0x04, 0x00},
    {0x04, 0x05, 0x04, 0x00, 0x00, 0x00},
    {0x80, 0x04, 0x00, 0x01, 0x02, 0x03},
    {0x84, 0x04, 0x04, 0x07, 0x05, 0x08},
    {0x88, 0x04, 0x09, 0x0A, 0x0E, 0x0F},
    {0x8C, 0x04, 0x0B, 0x0C, 0x0D, 0x10},
    {0x90, 0x04, 0x16, 0x17, 0x11, 0x12},
    {0x94, 0x04, 0x13, 0x14, 0x15, 0x1B},
    {0x98, 0x04, 0x18, 0x19, 0x1A, 0x06},
    {0x9C, 0x04, 0x31, 0x00, 0x00, 0x00},

};
#else
typedef struct
{
    uint16_t reg;
    uint32_t val;
} i2c_cmd;

static const i2c_cmd i2c_cmd_set[] =
{
    {.reg=0x013C, .val=0x00030005},
    {.reg=0x0114, .val=0x00000003},
    {.reg=0x0164, .val=0x00000004},
    {.reg=0x0168, .val=0x00000004},
    {.reg=0x016C, .val=0x00000004},
    {.reg=0x0170, .val=0x00000004},
    {.reg=0x0134, .val=0x0000001F},
    {.reg=0x0210, .val=0x0000001F},
    {.reg=0x0104, .val=0x00000001},
    {.reg=0x0204, .val=0x00000001},
    {.reg=0x0450, .val=0x03F00100},
    {.reg=0x0454, .val=0x000A000A},
    {.reg=0x0458, .val=0x012C0400},
    {.reg=0x045C, .val=0x00040004},
    {.reg=0x0460, .val=0x001E0300},
    {.reg=0x0464, .val=0x00000000},
    {.reg=0x0504, .val=0x00000004},
    {.reg=0x049C, .val=0x00000031},
    {.reg=0x04A0, .val=0x00000006},
    {.reg=0x0480, .val=0x03020100},
    {.reg=0x0484, .val=0x08050704},
    {.reg=0x0488, .val=0x0F0E0A09},
    {.reg=0x048C, .val=0x100D0C0B},
    {.reg=0x0490, .val=0x12111716},
    {.reg=0x0494, .val=0x1B151413},
    {.reg=0x0498, .val=0x061A1918},

};
#endif

struct tc358775_priv
{
    unsigned int addr;
    unsigned int lanes;
    enum mipi_dsi_pixel_format format;
    unsigned long mode_flags;
    struct gpio_desc reset;
    struct gpio_desc standby;
    struct gpio_desc lcd0_vdd_gpio;
    struct display_timing timing;
    struct udevice *backlight;
    int dsi_attached;
};

#if USE_MIPI_DCS
static int tc358775_send_mipi_initcmd(struct mipi_dsi_device *device)
{
    size_t i;
    const u8 *cmd;
    size_t count = sizeof(panel_init_sequence) / DCS_CMD_LEN;
    int ret = 0;

    for (i = 0; i < count ; i++)
    {
        cmd = panel_init_sequence[i];
        ret = mipi_dsi_generic_write(device, cmd, DCS_CMD_LEN);
        if (ret < 0)
            return ret;
    }

    return ret;
};

#else
static int tc358775_i2c_reg_write(struct udevice *dev, uint16_t reg_addr, uint32_t val)
{
    int ret = 0;
    ret = i2c_set_chip_offset_len(dev, 2);
    if (ret)
        goto err;

    ret = dm_i2c_write(dev, reg_addr, (const uint8_t *)&val, 4);
    if (ret)
    {
        debug("dm_i2c_write error, ret = %d\n", ret);
    }
err:
    return ret;

}

static int tc358775_i2c_reg_read(struct udevice *dev, uint8_t addr, uint8_t *data)
{
    uint8_t valb;
    int err;

    err = dm_i2c_read(dev, addr, &valb, 1);
    if (err)
        return err;

    *data = (int)valb;
    return 0;
}

static int tc358775_send_i2c_initcmd(struct udevice *dev)
{
    size_t i;
    size_t count = sizeof(i2c_cmd_set) / sizeof(i2c_cmd);
    int ret = 0;

    for (i = 0; i < count ; i++)
    {
        ret = tc358775_i2c_reg_write(dev, i2c_cmd_set[i].reg, i2c_cmd_set[i].val);
        if (ret < 0)
            return ret;
    }
    return ret;
};
#endif
static int tc358775_exit_standby(struct udevice *dev)
{
    struct tc358775_priv *priv = dev_get_priv(dev);
    int ret = 0;

    ret = dm_gpio_set_value(&priv->standby, 0);
    if (ret)
    {
        debug("gpio set value err\n");
        return ret;
    }
    mdelay(20);

    ret = dm_gpio_set_value(&priv->reset, 0);
    if (ret)
    {
        debug("gpio set value err\n");
        return ret;
    }
    return 0;
}

static int tc358775_init_dsi(struct udevice *dev)
{
    struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
    struct mipi_dsi_device *device = plat->device;
    struct tc358775_priv *priv = dev_get_priv(dev);
    int ret;

    tc358775_exit_standby(dev);

    ret = mipi_dsi_attach(device);

    if (ret < 0)
    {
        debug("Failed to attach mipi dsi (%d)\n", ret);
        return ret;
    }

    if (priv->dsi_attached == 0)
    {
        device->mode_flags |= MIPI_DSI_MODE_LPM;
#if USE_MIPI_DCS
        ret = tc358775_send_mipi_initcmd(device);
        if (ret < 0)
            return -EIO;
#else
        tc358775_send_i2c_initcmd(dev);
#endif
        priv->dsi_attached = 1;
    }
    return 0;
}

static int tc358775_enable(struct udevice *dev)
{
    struct tc358775_priv *priv = dev_get_priv(dev);
    struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
    struct mipi_dsi_device *device = plat->device;
    int ret = 0;

    ret = dm_gpio_set_value(&priv->lcd0_vdd_gpio, 1);
    if (ret)
    {
        debug("gpio set value err\n");
        return ret;
    }

    ret = dm_gpio_set_value(&priv->standby, 1);
    if (ret)
    {
        debug("gpio set value err\n");
        return ret;
    }

    ret = dm_gpio_set_value(&priv->reset, 1);
    if (ret)
    {
        debug("gpio set value err\n");
        return ret;
    }

    return 0;
}

static int tc358775_enable_backlight(struct udevice *dev)
{
    struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
    struct mipi_dsi_device *device = plat->device;
    struct tc358775_priv *priv = dev_get_priv(dev);
    int ret;

    printf("tc358775_enable_backlight \n");
    tc358775_init_dsi(dev);

    ret = backlight_enable(priv->backlight);
    if (ret)
        return ret;
    return 0;
}

static int tc358775_set_backlight(struct udevice *dev, int percent)
{
    struct tc358775_priv *priv = dev_get_priv(dev);
    int ret;

    printf("tc358775_set_backlight \n");
    ret = backlight_set_brightness(priv->backlight, percent);
    if (ret)
        return ret;
    return 0;
}

static int tc358775_get_display_timing(struct udevice *dev,
                                       struct display_timing *timings)
{
    struct mipi_dsi_panel_plat *plat = dev_get_platdata(dev);
    struct mipi_dsi_device *device = plat->device;
    struct tc358775_priv *priv = dev_get_priv(dev);

    memcpy(timings, &priv->timing, sizeof(*timings));

    if (device)
    {
        device->lanes = priv->lanes;
        device->format = priv->format;
        device->mode_flags = priv->mode_flags;
    }
    return 0;
}

static int tc358775_panel_ofdata_to_platdata(struct udevice *dev)
{
    struct tc358775_priv *priv = dev_get_priv(dev);
    int ret;

    ret = uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev, "backlight", &priv->backlight);
    if (ret)
    {
        debug("%s: Cannot get backlight: ret=%d\n", __func__, ret);
        return log_ret(ret);
    }

    ret = gpio_request_by_name(dev, "lcd0vdd-gpios", 0, &priv->lcd0_vdd_gpio, GPIOD_IS_OUT);
    if (ret)
    {
        debug("%s: Warning: cannot get lcd0_vdd GPIO: ret=%d\n", __func__, ret);
        return log_ret(ret);
    }

    ret = gpio_request_by_name(dev, "standby-gpios", 0, &priv->standby, GPIOD_IS_OUT);
    if (ret)
    {
        debug("%s: Warning: cannot get standby GPIO: ret=%d\n", __func__, ret);
        return log_ret(ret);
    }

    ret = gpio_request_by_name(dev, "reset-gpios", 0, &priv->reset, GPIOD_IS_OUT);
    if (ret)
    {
        debug("%s: Warning: cannot get reset GPIO: ret=%d\n", __func__, ret);
        return log_ret(ret);
    }

    return 0;
}

static int tc358775_probe(struct udevice *dev)
{
    struct tc358775_priv *priv = dev_get_priv(dev);
    int ret;

    printf("tc358775_probe  \n");
    priv->dsi_attached = 0;
    priv->addr = dev_read_addr(dev);

    if (priv->addr  == 0)
    {
        debug("%s: Can't find device\n", __func__);
        return -ENODEV;
    }

    ret = dev_read_u32(dev, "tc,dsi-lanes", &priv->lanes);
    if (ret)
    {
        debug("Failed to get dsi-lanes property (%d)\n", ret);
        return ret;
    }
    if (priv->lanes < 1 || priv->lanes > 4)
    {
        debug("Invalid dsi-lanes: %d\n", priv->lanes);
        return -EINVAL;
    }

    ret = dev_read_u32(dev, "tc,dsi-format", &priv->format);
    if (ret)
    {
        debug("Failed to get dsi format property (%d)\n", ret);
        return ret;
    }

    ret = dev_read_u32(dev, "tc,dsi-flags", &priv->mode_flags);
    if (ret)
    {
        debug("Failed to get dsi mode flags property (%d)\n", ret);
        return ret;
    }

    if (ofnode_decode_display_timing(dev_ofnode(dev), 0, &priv->timing))
    {
        debug("%s: Failed to decode display timing\n", __func__);
        return -EINVAL;
    }

    tc358775_enable(dev);

    return 0;
}

static const struct panel_ops tc358775_ops =
{
    .enable_backlight = tc358775_enable_backlight,
    .get_display_timing = tc358775_get_display_timing,
    .set_backlight		= tc358775_set_backlight,
};

static const struct udevice_id tc358775_ids[] =
{
    { .compatible = "tc,tc358775" },
    { }
};

U_BOOT_DRIVER(tc358775_mipi2hdmi) =
{
    .name			          = "tc358775_mipi2lvds",
    .id			  	          = UCLASS_PANEL,
    .of_match		          = tc358775_ids,
    .ops			          = &tc358775_ops,
    .ofdata_to_platdata	      = tc358775_panel_ofdata_to_platdata,
    .probe			          = tc358775_probe,
    .platdata_auto_alloc_size = sizeof(struct mipi_dsi_panel_plat),
    .priv_auto_alloc_size	  = sizeof(struct tc358775_priv),
};
