/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/led-lm3530.h>
#include <linux/bootmem.h>
#include <linux/msm_ion.h>
#include <asm/mach-types.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/ion.h>
#include <mach/msm_bus_board.h>
#include <mach/socinfo.h>

#include <msm/msm_fb.h>
#include <msm/msm_fb_def.h>
#include <msm/mipi_dsi.h>
#include <msm/mdp.h>

#include "devices.h"
#include "board-aries.h"

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
/* prim = 1366 x 768 x 3(bpp) x 3(pages) */
#if defined(CONFIG_FB_MSM_MIPI_HITACHI_CMD_720P_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(768 * 1280 * 4 * 3, 0x10000)
#else
#define MSM_FB_PRIM_BUF_SIZE roundup(1920 * 1088 * 4 * 3, 0x10000)
#endif
#else
/* prim = 1366 x 768 x 3(bpp) x 2(pages) */
#if defined(CONFIG_FB_MSM_MIPI_HITACHI_CMD_720P_PT)
#define MSM_FB_PRIM_BUF_SIZE roundup(768 * 1280 * 4 * 2, 0x10000)
#else
#define MSM_FB_PRIM_BUF_SIZE roundup(1920 * 1088 * 4 * 2, 0x10000)
#endif
#endif /*CONFIG_FB_MSM_TRIPLE_BUFFER */

#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE, 4096)

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
	#if defined(CONFIG_FB_MSM_MIPI_HITACHI_CMD_720P_PT)
	#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((768 * 1280 * 3 * 2), 4096)
	#else
	#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
	#endif
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */


static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};

#define MIPI_CMD_HITACHI_720P_PANEL_NAME "mipi_cmd_hitachi_720p"
#define HDMI_PANEL_NAME "hdmi_msm"

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
static unsigned char hdmi_is_primary = 1;
#else
static unsigned char hdmi_is_primary;
#endif

unsigned char apq8064_hdmi_as_primary_selected(void)
{
	return hdmi_is_primary;
}

static void set_mdp_clocks_for_wuxga(void);

static int msm_fb_detect_panel(const char *name)
{
	if (!strncmp(name, MIPI_CMD_HITACHI_720P_PANEL_NAME,
		strnlen(MIPI_CMD_HITACHI_720P_PANEL_NAME,
			PANEL_NAME_MAX_LEN)))
		return 0;

	if (!strncmp(name, HDMI_PANEL_NAME,
			strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
		if (apq8064_hdmi_as_primary_selected())
			set_mdp_clocks_for_wuxga();
		return 0;
	}


	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name              = "msm_fb",
	.id                = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

void __init apq8064_allocate_fb_region(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));
}

#define MDP_VSYNC_GPIO 0

static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 230400000 * 2,
		.ib = 288000000 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 334080000 * 2,
		.ib = 417600000 * 2,
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
	.mdp_max_clk = 266667000,
	.mdp_max_bw = 3000000000u,
	.mdp_bw_ab_factor = 115,
	.mdp_bw_ib_factor = 125,
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
	.mdp_rev = MDP_REV_44,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.mdp_iommu_split_domain = 1,
};

void __init apq8064_mdp_writeback(struct memtype_reserve* reserve_table)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	mdp_pdata.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE;
#if defined(CONFIG_ANDROID_PMEM) && !defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov0_wb_size;
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov1_wb_size;
#endif
}

static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);
static int hdmi_gpio_config(int on);
static int hdmi_panel_power(int on);

static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
	.panel_power = hdmi_panel_power,
	.gpio_config = hdmi_gpio_config,
};

static struct platform_device hdmi_msm_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};

static char wfd_check_mdp_iommu_split_domain(void)
{
	return mdp_pdata.mdp_iommu_split_domain;
}

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
static struct msm_wfd_platform_data wfd_pdata = {
	.wfd_check_mdp_iommu_split = wfd_check_mdp_iommu_split_domain,
};

static struct platform_device wfd_panel_device = {
	.name = "wfd_panel",
	.id = 0,
	.dev.platform_data = NULL,
};

static struct platform_device wfd_device = {
	.name          = "msm_wfd",
	.id            = -1,
	.dev.platform_data = &wfd_pdata,
};
#endif

/* HDMI related GPIOs */
#define HDMI_CEC_VAR_GPIO	69
#define HDMI_DDC_CLK_GPIO	70
#define HDMI_DDC_DATA_GPIO	71
#define HDMI_HPD_GPIO		72

/* power: ldo23 1.8  gpio11 vspvsn */
/* reset: gpio25 */
#define MI_RESET_GPIO		PM8921_GPIO_PM_TO_SYS(25)
#define MI_LCD_ID_GPIO		PM8921_GPIO_PM_TO_SYS(12)

static bool dsi_power_on = false;
static int mipi_dsi_panel_power(int on)
{
	static struct regulator *reg_l23, *reg_l2, *reg_lvs7, *reg_vsp;
	static int reset_gpio, lcd_id_gpio;
	int rc;

	if (!dsi_power_on) {
		reg_lvs7 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_vddio");
		if (IS_ERR_OR_NULL(reg_lvs7)) {
			pr_err("could not get 8921_lvs7, rc = %ld\n",
					PTR_ERR(reg_lvs7));
			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi1_pll_vdda");
		if (IS_ERR_OR_NULL(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
					PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		reg_l23 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_mi_vddio");
		if (IS_ERR_OR_NULL(reg_l23)) {
			pr_err("could not get 8921_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		reg_vsp = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_mi_vsp");
		if (IS_ERR_OR_NULL(reg_vsp)) {
			pr_err("could not get VSP/VSN regulator, rc = %ld\n",
				PTR_ERR(reg_vsp));
			return -ENODEV;
		}

		reset_gpio = MI_RESET_GPIO;
		rc = gpio_request(reset_gpio, "disp_rst_n");
		if (rc) {
			pr_err("request pm8921 gpio 25 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		lcd_id_gpio = MI_LCD_ID_GPIO;
		rc = gpio_request(lcd_id_gpio, "disp_id_det");
		if (rc) {
			pr_err("request pm8921 gpio 12 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		dsi_power_on = true;
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_lvs7);
		if (rc) {
			pr_err("enable lvs7 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_enable(reg_l23);
		if (rc) {
			pr_err("enable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		mdelay(1);

		rc = regulator_enable(reg_vsp);
		if (rc) {
			pr_err("enable vsp failed, rc=%d\n", rc);
			return -ENODEV;
		}
		mdelay(10);

		gpio_direction_output(reset_gpio, 1);
		mdelay(3);

		//mi_panel_id = gpio_get_value(lcd_id_gpio);
	} else {
		gpio_direction_output(reset_gpio, 0);

		rc = regulator_disable(reg_vsp);
		if (rc) {
			pr_err("disable reg_vsp failed, rc=%d\n", rc);
			return -ENODEV;
		}
		mdelay(10);

		rc = regulator_disable(reg_lvs7);
		if (rc) {
			pr_err("disable reg_lvs7 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_disable(reg_l23);
		if (rc) {
			pr_err("disable reg_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
	}

	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.dsi_power_save = mipi_dsi_panel_power,
};

static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 566092800 * 2,
		.ib = 707616000 * 2,
	},
};

static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
	.lcdc_power_save = hdmi_panel_power,
};

static int hdmi_panel_power(int on)
{
	int rc;

	pr_debug("%s: HDMI Core: %s\n", __func__, (on ? "ON" : "OFF"));
	rc = hdmi_core_power(on, 1);
	if (rc)
		rc = hdmi_cec_power(on);

	pr_debug("%s: HDMI Core: %s Success\n", __func__, (on ? "ON" : "OFF"));
	return rc;
}

static int hdmi_enable_5v(int on)
{
	return 0;
}

static int hdmi_core_power(int on, int show)
{
	static struct regulator *reg_8921_lvs7, *reg_8921_s4, *reg_ext_3p3v;
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	/* TBD: PM8921 regulator instead of 8901 */
	if (!reg_ext_3p3v) {
		reg_ext_3p3v = regulator_get(&hdmi_msm_device.dev,
					     "hdmi_mux_vdd");
		if (IS_ERR_OR_NULL(reg_ext_3p3v)) {
			pr_err("could not get reg_ext_3p3v, rc = %ld\n",
			       PTR_ERR(reg_ext_3p3v));
			reg_ext_3p3v = NULL;
			return -ENODEV;
		}
	}

	if (!reg_8921_lvs7) {
		reg_8921_lvs7 = regulator_get(&hdmi_msm_device.dev,
					      "hdmi_vdda");
		if (IS_ERR(reg_8921_lvs7)) {
			pr_err("could not get reg_8921_lvs7, rc = %ld\n",
				PTR_ERR(reg_8921_lvs7));
			reg_8921_lvs7 = NULL;
			return -ENODEV;
		}
	}
	if (!reg_8921_s4) {
		reg_8921_s4 = regulator_get(&hdmi_msm_device.dev,
					    "hdmi_lvl_tsl");
		if (IS_ERR(reg_8921_s4)) {
			pr_err("could not get reg_8921_s4, rc = %ld\n",
				PTR_ERR(reg_8921_s4));
			reg_8921_s4 = NULL;
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_s4, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_s4, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	if (on) {
		/*
		 * Configure 3P3V_BOOST_EN as GPIO, 8mA drive strength,
		 * pull none, out-high
		 */
		rc = regulator_set_optimum_mode(reg_ext_3p3v, 290000);
		if (rc < 0) {
			pr_err("set_optimum_mode ext_3p3v failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_enable(reg_ext_3p3v);
		if (rc) {
			pr_err("enable reg_ext_3p3v failed, rc=%d\n", rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_lvs7);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_vdda", rc);
			goto error1;
		}
		rc = regulator_enable(reg_8921_s4);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_lvl_tsl", rc);
			goto error2;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_ext_3p3v);
		if (rc) {
			pr_err("disable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_8921_lvs7);
		if (rc) {
			pr_err("disable reg_8921_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_8921_s4);
		if (rc) {
			pr_err("disable reg_8921_s4 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;

error2:
	regulator_disable(reg_8921_lvs7);
error1:
	regulator_disable(reg_ext_3p3v);
	return rc;
}

static int hdmi_gpio_config(int on)
{
	int rc = 0;
	static int prev_on;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(HDMI_DDC_CLK_GPIO, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", HDMI_DDC_CLK_GPIO, rc);
			goto error1;
		}
		rc = gpio_request(HDMI_DDC_DATA_GPIO, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", HDMI_DDC_DATA_GPIO, rc);
			goto error2;
		}
		rc = gpio_request(HDMI_HPD_GPIO, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", HDMI_HPD_GPIO, rc);
			goto error3;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(HDMI_DDC_CLK_GPIO);
		gpio_free(HDMI_DDC_DATA_GPIO);
		gpio_free(HDMI_HPD_GPIO);

		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;
	return 0;

error3:
	gpio_free(HDMI_DDC_DATA_GPIO);
error2:
	gpio_free(HDMI_DDC_CLK_GPIO);
error1:
	return rc;
}

static int hdmi_cec_power(int on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(HDMI_CEC_VAR_GPIO, "HDMI_CEC_VAR");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_CEC_VAR", HDMI_CEC_VAR_GPIO, rc);
			goto error;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(HDMI_CEC_VAR_GPIO);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
error:
	return rc;
}

#if defined(CONFIG_FB_MSM_MIPI_HITACHI_CMD_720P_PT)
static int mipi_hitachi_backlight_level(int level, int max, int min)
{
#ifdef CONFIG_LEDS_LM3530
	backlight_brightness_set(level);
#endif

	return 0;
}

static char sleep_out_for_cabc[2] = {0x11,0x00};
static char mcap_start[2] = {0xB0, 0x04};
static char write_ce_off[2] = {0xCA, 0x00};
static char mcap_end[2] = {0xB0, 0x03};

#ifdef CONFIG_HITACHI_CMD_720P_CABC
static char write_display_brightness[3] = {0x51, 0xE, 0xFF};
static char write_cabc[2] = {0x55, 0x01};
static char write_control_display[2] = {0x53, 0x2C};
#else
static char write_display_brightness[3] = {0x51, 0xE, 0xFF};
static char write_cabc[2] = {0x55, 0x00};
static char write_control_display[2] = {0x53, 0x00};
#endif

static char set_width[5] = {0x2A, 0x00, 0x00, 0x02, 0xCF}; /* 720 - 1 */
static char set_height[5] = {0x2B, 0x00, 0x00, 0x04, 0xFF}; /* 1280 - 1 */
static char set_address_mode[2] = {0x36, 0x00};
static char rgb_888[2] = {0x3a, 0x77};

static char display_on[2] =  {0x29,0x00};
static char display_off[2] = {0x28,0x00};
static char enter_sleep[2] = {0x10,0x00};

static char gamma_jdi_24_r[25] = {0xC7, 0x00, 0x0B, 0x12, 0x1C, 0x2A, 0x45, 0x3B, 0x50, 0x5E, 0x6B, 0x6F, 0x7F,
					0x00, 0x0B, 0x12, 0x1C, 0x2A, 0x45, 0x3B, 0x50, 0x5E, 0x6B, 0x6F, 0x7F};
static char gamma_jdi_24_g[25] = {0xC8, 0x00, 0x0B, 0x12, 0x1C, 0x2A, 0x45, 0x3B, 0x50, 0x5E, 0x6B, 0x6F, 0x7F,
					0x00, 0x0B, 0x12, 0x1C, 0x2A, 0x45, 0x3B, 0x50, 0x5E, 0x6B, 0x6F, 0x7F};
static char gamma_jdi_24_b[25] = {0xC9, 0x00, 0x0B, 0x12, 0x1C, 0x2A, 0x45, 0x3B, 0x50, 0x5E, 0x6B, 0x6F, 0x7F,
					0x00, 0x0B, 0x12, 0x1C, 0x2A, 0x45, 0x3B, 0x50, 0x5E, 0x6B, 0x6F, 0x7F };

static char write_ce_on[33] = {
	0xCA, 0x01, 0x80, 0x88, 0x8C, 0xBC, 0x8C, 0x8C,
	0x8C, 0x18, 0x3F, 0x14, 0xFF, 0x0A, 0x4A, 0x37,
	0xA0, 0x55, 0xF8, 0x0C, 0x0C, 0x20, 0x10, 0x3F,
	0x3F, 0x00, 0x00, 0x10, 0x10, 0x3F, 0x3F, 0x3F,
	0x3F};

static char cabc_test[26] = {
	0xB8, 0x18, 0x80, 0x18, 0x18, 0xCF, 0x1F, 0x00,
	0x0C, 0x0E, 0x6C, 0x0E,	0x6C, 0x0E, 0x0C, 0x0E,
	0xDA, 0x6D, 0xFF, 0xFF,	0x10, 0x8C, 0xD2, 0xFF,
	0xFF, 0xFF};
static char cabc_movie_still[8] = {0xB9, 0x00, 0x3F, 0x18, 0x18, 0x9F, 0x1F, 0x80};
static char cabc_user_inf[8] = {0xBA, 0x00, 0x3F, 0x18, 0x18, 0x9F, 0x1F, 0xD7};

static struct dsi_cmd_desc hitachi_power_on_set_1[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out_for_cabc), sleep_out_for_cabc},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(mcap_start), mcap_start},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(write_ce_off), write_ce_off},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(mcap_end), mcap_end },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_display_brightness), write_display_brightness},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_cabc), write_cabc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(set_width), set_width},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 20, sizeof(set_height), set_height},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 20, sizeof(set_address_mode), set_address_mode},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(rgb_888), rgb_888},
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc hitachi_power_on_set_gamma[] = {
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(mcap_start), mcap_start},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_jdi_24_r),  gamma_jdi_24_r},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_jdi_24_g),  gamma_jdi_24_g},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gamma_jdi_24_b),  gamma_jdi_24_b},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(mcap_end), mcap_end },
};

static struct dsi_cmd_desc hitachi_power_on_set_ce[] = {
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(mcap_start), mcap_start},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(write_ce_on), write_ce_on},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(mcap_end), mcap_end},
};

static struct dsi_cmd_desc hitachi_power_on_set_cabc[] = {
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(mcap_start), mcap_start},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_test), cabc_test},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_movie_still), cabc_movie_still},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cabc_user_inf), cabc_user_inf},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(mcap_end), mcap_end},
};

static struct dsi_cmd_desc hitachi_power_off_set_1[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 5, sizeof(enter_sleep), enter_sleep},
};


static struct msm_panel_common_pdata mipi_hitachi_pdata = {
	.backlight_level = mipi_hitachi_backlight_level,

	.power_on_set_1 = hitachi_power_on_set_1,
	.power_on_set_size_1 = ARRAY_SIZE(hitachi_power_on_set_1),

	.power_off_set_1 = hitachi_power_off_set_1,
	.power_off_set_size_1 = ARRAY_SIZE(hitachi_power_off_set_1),

	.power_on_set_gamma = hitachi_power_on_set_gamma,
	.power_on_set_size_gamma = ARRAY_SIZE(hitachi_power_on_set_gamma),

	.power_on_set_ce = hitachi_power_on_set_ce,
	.power_on_set_size_ce = ARRAY_SIZE(hitachi_power_on_set_ce),

	.power_on_set_cabc = hitachi_power_on_set_cabc,
	.power_on_set_size_cabc = ARRAY_SIZE(hitachi_power_on_set_cabc),
};

static struct platform_device mipi_dsi_hitachi_panel_device = {
	.name = "mipi_hitachi",
	.id = 0,
	.dev = {
		.platform_data = &mipi_hitachi_pdata,
	}
};
#endif

static struct platform_device *aries_panel_devices[] __initdata = {
#if defined(CONFIG_FB_MSM_MIPI_HITACHI_CMD_720P_PT)
	&mipi_dsi_hitachi_panel_device,
#endif
};

void __init apq8064_init_fb(void)
{
	platform_device_register(&msm_fb_device);

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
	platform_device_register(&wfd_panel_device);
	platform_device_register(&wfd_device);
#endif

	platform_add_devices(aries_panel_devices,
			ARRAY_SIZE(aries_panel_devices));

	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	platform_device_register(&hdmi_msm_device);
	msm_fb_register_device("dtv", &dtv_pdata);
}

#ifdef CONFIG_HITACHI_CMD_720P_CABC
#define PWM_SIMPLE_EN 0xA0
#define PWM_BRIGHTNESS 0x20
#endif

#if defined (CONFIG_LEDS_LM3530)
static struct lm3530_platform_data lm3530_data = {
#ifdef CONFIG_HITACHI_CMD_720P_CABC
	.mode = LM3530_BL_MODE_I2C_PWM,
#else
	.mode = LM3530_BL_MODE_MANUAL,
#endif
	.max_current = 0x5,
	.pwm_pol_hi = 0,
	.brt_ramp_law = 0x1,		/* linear */
	.brt_ramp_fall = 0,
	.brt_ramp_rise = 0,
	.brt_val = 0,
	.bl_en_gpio = PM8921_GPIO_PM_TO_SYS(13),
	.regulator_used = 0,
};
#endif

static struct i2c_board_info msm_i2c_backlight_info[] = {
	{
#if defined(CONFIG_LEDS_LM3530)
		I2C_BOARD_INFO("lm3530-led", 0x38),
		.platform_data = &lm3530_data,
#endif
	}
};

static struct i2c_registry apq8064_i2c_backlight_device[] __initdata = {
	{
		I2C_FFA,
		APQ_8064_GSBI1_QUP_I2C_BUS_ID,
		msm_i2c_backlight_info,
		ARRAY_SIZE(msm_i2c_backlight_info),
	},
};

void __init xiaomi_add_backlight_devices(void)
{
	int i;

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(apq8064_i2c_backlight_device); ++i) {
		i2c_register_board_info(apq8064_i2c_backlight_device[i].bus,
					apq8064_i2c_backlight_device[i].info,
					apq8064_i2c_backlight_device[i].len);
	}
}

/**
 * Set MDP clocks to high frequency to avoid DSI underflow
 * when using high resolution 1200x1920 WUXGA panels
 */
static void set_mdp_clocks_for_wuxga(void)
{
	mdp_ui_vectors[0].ab = 2000000000;
	mdp_ui_vectors[0].ib = 2000000000;
	mdp_vga_vectors[0].ab = 2000000000;
	mdp_vga_vectors[0].ib = 2000000000;
	mdp_720p_vectors[0].ab = 2000000000;
	mdp_720p_vectors[0].ib = 2000000000;
	mdp_1080p_vectors[0].ab = 2000000000;
	mdp_1080p_vectors[0].ib = 2000000000;

	if (apq8064_hdmi_as_primary_selected()) {
		dtv_bus_def_vectors[0].ab = 2000000000;
		dtv_bus_def_vectors[0].ib = 2000000000;
	}
}

void __init apq8064_set_display_params(char *prim_panel, char *ext_panel,
		unsigned char resolution)
{

	if (strnlen(prim_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.prim_panel_name, prim_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.prim_panel_name %s\n",
			msm_fb_pdata.prim_panel_name);

		if (!strncmp((char *)msm_fb_pdata.prim_panel_name,
			HDMI_PANEL_NAME, strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
			pr_debug("HDMI is the primary display by"
				" boot parameter\n");
			hdmi_is_primary = 1;
			set_mdp_clocks_for_wuxga();
		}
	}
	if (strnlen(ext_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.ext_panel_name, ext_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.ext_panel_name %s\n",
			msm_fb_pdata.ext_panel_name);
	}

	msm_fb_pdata.ext_resolution = resolution;
}
