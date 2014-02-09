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
#ifdef CONFIG_LEDS_LM3530
#include <linux/led-lm3530.h>
#endif

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_hitachi.h"
#include "mdp4.h"

static struct msm_panel_common_pdata *mipi_hitachi_pdata;

struct dsi_cmd_desc local_power_on_set_gamma[5];
static struct dsi_buf hitachi_tx_buf;
static struct dsi_buf hitachi_rx_buf;
static struct msm_fb_data_type *local_mfd;
static int lcd_isactive = 0;
static int lcd_ce_enabled = 0;
static int lcd_cabc_enabled = 0;

static ssize_t kgamma_apply_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count);

static int gamma_set(void) {
	int ret = 0;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);

	ret = mipi_dsi_cmds_tx(&hitachi_tx_buf,
			local_power_on_set_gamma,
			mipi_hitachi_pdata->power_on_set_size_gamma);

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (ret < 0) {
		pr_err("%s: failed to transmit power_on_set_gamma cmds\n", __func__);
		return ret;
	}

	return ret;
}

static int ce_enable(void) {
	int ret = 0;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);

	ret = mipi_dsi_cmds_tx(&hitachi_tx_buf,
			mipi_hitachi_pdata->power_on_set_ce,
			mipi_hitachi_pdata->power_on_set_size_ce);

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (ret < 0) {
		pr_err("%s: failed to transmit power_on_set_ce cmds\n", __func__);
		return ret;
	}

	return ret;
}

static int cabc_enable(void) {
	int ret = 0;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);

	ret = mipi_dsi_cmds_tx(&hitachi_tx_buf,
			mipi_hitachi_pdata->power_on_set_cabc,
			mipi_hitachi_pdata->power_on_set_size_cabc);

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (ret < 0) {
		pr_err("%s: failed to transmit power_on_set_cabc cmds\n", __func__);
		return ret;
	}

	return ret;
}

static int lcd_power_on(void) {
	int ret = 0;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);

	ret = mipi_dsi_cmds_tx(&hitachi_tx_buf,
			mipi_hitachi_pdata->power_on_set_1,
			mipi_hitachi_pdata->power_on_set_size_1);

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (ret < 0) {
		pr_err("%s: failed to transmit power_on_set_1 cmds\n", __func__);
		return ret;
	}

	return ret;
}

static int mipi_hitachi_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int ret = 0;

	pr_info("%s started\n", __func__);

	mfd = platform_get_drvdata(pdev);
	local_mfd = mfd;
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

#ifdef CONFIG_LEDS_LM3530
	backlight_brightness_set(0);
#endif

	lcd_isactive = 1;
	lcd_power_on();

	if(lcd_ce_enabled)
		ce_enable();

	if(lcd_cabc_enabled)
		cabc_enable();

	kgamma_apply_store(NULL, NULL, NULL, 0);


	pr_info("%s finished\n", __func__);
	return ret;
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

	lcd_isactive = 0;

	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x10000000);
	ret = mipi_dsi_cmds_tx(&hitachi_tx_buf,
			mipi_hitachi_pdata->power_off_set_1,
			mipi_hitachi_pdata->power_off_set_size_1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x38, 0x14000000);
	if (ret < 0) {
		pr_err("%s: failed to transmit power_off_set_1 cmds\n", __func__);
		return ret;
	}
#ifdef CONFIG_LEDS_LM3530
	backlight_brightness_set(0);
#endif

	pr_info("%s finished\n", __func__);
	return 0;
}

static void mipi_hitachi_lcd_shutdown(void)
{
	int ret = 0;

	if(local_mfd && !local_mfd->panel_power_on) {
		pr_info("%s:panel is already off\n", __func__);
		return;
	}

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

/******************* Begin sysfs interface *******************/

static unsigned int calc_checksum(int intArr[]) {
	int i = 0;
	unsigned int chksum = 0;

	for (i=1; i<13; i++)
		chksum += intArr[i];

	return chksum;
}

static ssize_t do_kgamma_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count,
				unsigned int offset)
{
	int kgamma[13];
	int i;

	printk("%s: buf: %s\n", __func__, buf);

	sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
		&kgamma[0], &kgamma[1], &kgamma[2], &kgamma[3],
		&kgamma[4], &kgamma[5], &kgamma[6], &kgamma[7],
		&kgamma[8], &kgamma[9], &kgamma[10],&kgamma[11],
		&kgamma[12]);

	for (i=1; i<13; i++) {
		if (kgamma[i] > 255) {
			pr_info("char values  can't be over 255, got %d instead!", kgamma[i]);
			return -EINVAL;
		}
	}

	if (calc_checksum(kgamma) == (unsigned int) kgamma[0]) {
		kgamma[0] = 0xc7 + offset;
		for (i=0; i<13; i++) {
			pr_info("kgamma_p [%d] => %d \n", i, kgamma[i]);
			local_power_on_set_gamma[1+offset].payload[i] = kgamma[i];

			if(i>0)
				local_power_on_set_gamma[1+offset].payload[12+i] = kgamma[i];
		}
		return count;
	}
	return -EINVAL;
}

static ssize_t do_kgamma_show(struct device *dev, struct device_attribute *attr,
				char *buf, unsigned int offset)
{
	int kgamma[13];
	int i;

	for (i=1; i<13; i++)
		kgamma[i] = local_power_on_set_gamma[1+offset].payload[i];

	kgamma[0] = (int) calc_checksum(kgamma);

	return sprintf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
		kgamma[0], kgamma[1], kgamma[2], kgamma[3],
		kgamma[4], kgamma[5], kgamma[6], kgamma[7],
		kgamma[8], kgamma[9], kgamma[10],kgamma[11],
		kgamma[12]);
}

static ssize_t kgamma_r_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	return do_kgamma_store(dev,attr,buf,count,0);
}

static ssize_t kgamma_r_show(struct device *dev, struct device_attribute *attr,
								char *buf)
{
	return do_kgamma_show(dev,attr,buf,0);
}

static ssize_t kgamma_g_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	return do_kgamma_store(dev,attr,buf,count,1);
}

static ssize_t kgamma_g_show(struct device *dev, struct device_attribute *attr,
								char *buf)
{
	return do_kgamma_show(dev,attr,buf,1);
}

static ssize_t kgamma_b_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	return do_kgamma_store(dev,attr,buf,count,2);
}

static ssize_t kgamma_b_show(struct device *dev, struct device_attribute *attr,
								char *buf)
{
	return do_kgamma_show(dev,attr,buf,2);
}

static ssize_t kgamma_apply_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	int ret = 0;

	/*
	 * Only attempt to apply if the LCD is active.
	 * If it isn't, the device will panic-reboot
	 */
	if(lcd_isactive) {
		ret = gamma_set();
		if (ret < 0) {
			pr_err("%s: failed to transmit power_on_set_1 cmds\n", __func__);
			return ret;
		}
	}
	else {
		pr_err("%s: Tried to apply gamma settings when LCD was off\n",__func__);
		//Is ENODEV correct here?  Perhaps it should be something else?
		return -ENODEV;
	}
	return count;
}

static ssize_t kgamma_apply_show(struct device *dev, struct device_attribute *attr,
								char *buf)
{
	return 0;
}

static ssize_t ce_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	int ret = 0;
	int on = 0;

	sscanf(buf, "%d", &on);
	lcd_ce_enabled = on?1:0;

	/*
	 * Only attempt to apply if the LCD is active.
	 * If it isn't, the device will panic-reboot
	 */
	if(lcd_isactive) {

		if(lcd_ce_enabled)
			ret = ce_enable();
		else
			ret = lcd_power_on();

		if (ret < 0) {
			pr_err("%s: failed to transmit power_on_set_1 cmds\n", __func__);
			return ret;
		}
	}
	else {
		pr_err("%s: Tried to apply gamma settings when LCD was off\n",__func__);
		//Is ENODEV correct here?  Perhaps it should be something else?
		return -ENODEV;
	}
	return count;
}

static ssize_t ce_show(struct device *dev, struct device_attribute *attr,
								char *buf)
{
	return sprintf(buf, "%d", lcd_ce_enabled);
}

static ssize_t cabc_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	int ret = 0;
	int on = 0;

	sscanf(buf, "%d", &on);
	lcd_cabc_enabled = on?1:0;

	/*
	 * Only attempt to apply if the LCD is active.
	 * If it isn't, the device will panic-reboot
	 */
	if(lcd_isactive) {
		if(lcd_cabc_enabled)
			ret = cabc_enable();
		else
			ret = lcd_power_on();

		if (ret < 0) {
			pr_err("%s: failed to transmit power_on_set_1 cmds\n", __func__);
			return ret;
		}
	}
	else {
		pr_err("%s: Tried to apply gamma settings when LCD was off\n",__func__);
		//Is ENODEV correct here?  Perhaps it should be something else?
		return -ENODEV;
	}
	return count;
}

static ssize_t cabc_show(struct device *dev, struct device_attribute *attr,
								char *buf)
{
	return sprintf(buf, "%d", lcd_cabc_enabled);
}

static DEVICE_ATTR(kgamma_r, 0644, kgamma_r_show, kgamma_r_store);
static DEVICE_ATTR(kgamma_g, 0644, kgamma_g_show, kgamma_g_store);
static DEVICE_ATTR(kgamma_b, 0644, kgamma_b_show, kgamma_b_store);
static DEVICE_ATTR(kgamma_apply, 0644, kgamma_apply_show, kgamma_apply_store);
static DEVICE_ATTR(ce, 0644, ce_show, ce_store);
static DEVICE_ATTR(cabc, 0644, cabc_show, cabc_store);


/******************* End sysfs interface *******************/

static int mipi_hitachi_lcd_probe(struct platform_device *pdev)
{
	int rc;

	if (pdev->id == 0) {
		mipi_hitachi_pdata = pdev->dev.platform_data;
		return 0;
	}

	// Make a copy of platform data
	memcpy((void*)local_power_on_set_gamma, (void*)mipi_hitachi_pdata->power_on_set_gamma,
		sizeof(local_power_on_set_gamma));

	pr_info("%s start\n", __func__);

	msm_fb_add_device(pdev);

	register_syscore_ops(&panel_syscore_ops);

	rc = device_create_file(&pdev->dev, &dev_attr_kgamma_r);
	if(rc !=0)
		return -1;

	rc = device_create_file(&pdev->dev, &dev_attr_kgamma_g);
	if(rc !=0)
		return -1;

	rc = device_create_file(&pdev->dev, &dev_attr_kgamma_b);
	if(rc !=0)
		return -1;

	rc = device_create_file(&pdev->dev, &dev_attr_kgamma_apply);
	if(rc !=0)
		return -1;

	rc = device_create_file(&pdev->dev, &dev_attr_ce);
	if(rc !=0)
		return -1;

	rc = device_create_file(&pdev->dev, &dev_attr_cabc);
	if(rc !=0)
		return -1;

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
