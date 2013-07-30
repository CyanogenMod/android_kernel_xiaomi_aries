/*
 *  Copyright (C) 2011-2012, LG Eletronics,Inc. All rights reserved.
 *      HITACHI LCD device driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/gpio.h>
#include <linux/syscore_ops.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_hitachi.h"
#include "mdp4.h"

static struct msm_panel_common_pdata *mipi_hitachi_pdata;

static struct dsi_buf hitachi_tx_buf;
static struct dsi_buf hitachi_rx_buf;

static int mipi_hitachi_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int ret = 0;

	pr_info("%s started\n", __func__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	ret = mipi_dsi_cmds_tx(&hitachi_tx_buf,
			mipi_hitachi_pdata->power_on_set_1,
			mipi_hitachi_pdata->power_on_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (ret < 0) {
		pr_err("%s: failed to transmit power_on_set_1 cmds\n", __func__);
		return ret;
	}

	pr_info("%s finished\n", __func__);
	return 0;
}

static int mipi_hitachi_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int ret = 0;

	pr_info("%s started\n", __func__);

	if (mipi_hitachi_pdata->bl_pwm_disable)
		mipi_hitachi_pdata->bl_pwm_disable();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	ret = mipi_dsi_cmds_tx(&hitachi_tx_buf,
			mipi_hitachi_pdata->power_off_set_1,
			mipi_hitachi_pdata->power_off_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (ret < 0) {
		pr_err("%s: failed to transmit power_off_set_1 cmds\n", __func__);
		return ret;
	}

	pr_info("%s finished\n", __func__);
	return 0;
}

static void mipi_hitachi_lcd_shutdown(void)
{
	int ret = 0;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	ret = mipi_dsi_cmds_tx(&hitachi_tx_buf,
			mipi_hitachi_pdata->power_off_set_1,
			mipi_hitachi_pdata->power_off_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (ret < 0) {
		pr_err("%s: failed to transmit power_off_set_1 cmds\n", __func__);
	}

	pr_info("%s finished\n", __func__);
}

static int mipi_hitachi_backlight_on_status(void)
{
	return (mipi_hitachi_pdata->bl_on_status());
}

static void mipi_hitachi_set_backlight_board(struct msm_fb_data_type *mfd)
{
	int level;

	level = (int)mfd->bl_level;
	mipi_hitachi_pdata->backlight_level(level, 0, 0);
}

struct syscore_ops panel_syscore_ops = {
	.shutdown = mipi_hitachi_lcd_shutdown,
};

static int mipi_hitachi_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_hitachi_pdata = pdev->dev.platform_data;
		return 0;
	}

	pr_info("%s start\n", __func__);

	msm_fb_add_device(pdev);

	register_syscore_ops(&panel_syscore_ops);

	return 0;
}

static struct platform_driver this_driver = {
	.probe = mipi_hitachi_lcd_probe,
	.driver = {
		.name = "mipi_hitachi",
	},
};

static struct msm_fb_panel_data hitachi_panel_data = {
	.on = mipi_hitachi_lcd_on,
	.off = mipi_hitachi_lcd_off,
	.set_backlight = mipi_hitachi_set_backlight_board,
	.get_backlight_on_status = mipi_hitachi_backlight_on_status,
};

static int ch_used[3];

int mipi_hitachi_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_hitachi", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	hitachi_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &hitachi_panel_data,
			sizeof(hitachi_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}
	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_hitachi_lcd_init(void)
{
	mipi_dsi_buf_alloc(&hitachi_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&hitachi_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_hitachi_lcd_init);
