/*
 * Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <mach/board_xiaomi.h>
#include "devices.h"

#ifdef CONFIG_SND_SOC_ES310
#include <sound/es310.h>
#endif

#include "board-aries.h"

#include <linux/firmware.h>
#include "firmware/audience-es310.h"
#define ES310_FIRMWARE_NAME	"audience-es310.img"
DECLARE_BUILTIN_FIRMWARE(ES310_FIRMWARE_NAME, firmware_audience_es310);

#define ES310_ADDRESS (0x3E)

#ifdef CONFIG_SND_SOC_ES310
static int es310_power_setup(int on)
{
	int rc;
	struct regulator *es310_vdd;

	es310_vdd = regulator_get(NULL, "8921_l15");

	if (IS_ERR(es310_vdd)) {
		printk(KERN_ERR "%s: Unable to get regulator es310_vdd\n",
		       __func__);
		return -1;
	}

	rc = regulator_set_voltage(es310_vdd, 2800000, 2800000);
	if (rc) {
		printk(KERN_ERR "%s: regulator set voltage failed\n", __func__);
		return rc;
	}

	rc = regulator_enable(es310_vdd);
	if (rc) {
		printk(KERN_ERR
		       "%s: Error while enabling regulator for es310 %d\n",
		       __func__, rc);
		return rc;
	}

	return 0;
}

#define MITWO_GPIO_ES310_CLK		34
#define MITWO_GPIO_ES310_RESET		37
#define MITWO_GPIO_ES310_MIC_SWITCH	38
#define MITWO_GPIO_ES310_WAKEUP		51
static struct es310_platform_data audience_es310_pdata = {
	.gpio_es310_reset = MITWO_GPIO_ES310_RESET,
	.gpio_es310_clk = MITWO_GPIO_ES310_CLK,
	.gpio_es310_wakeup = MITWO_GPIO_ES310_WAKEUP,
	.gpio_es310_mic_switch = MITWO_GPIO_ES310_MIC_SWITCH,
	.power_on = es310_power_setup,
	.fw_name = ES310_FIRMWARE_NAME,
};
#endif

static struct i2c_board_info msm_i2c_audiosubsystem_info[] = {
#ifdef CONFIG_SND_SOC_ES310
	{
		I2C_BOARD_INFO("audience_es310", ES310_ADDRESS),
		.platform_data = &audience_es310_pdata,
	}
#endif
};

static struct i2c_registry msm_i2c_audiosubsystem __initdata = {
	/* Add the I2C driver for Audio Amp */
	I2C_FFA,
	APQ_8064_GSBI1_QUP_I2C_BUS_ID,
	msm_i2c_audiosubsystem_info,
	ARRAY_SIZE(msm_i2c_audiosubsystem_info),
};

static void __init xiaomi_add_i2c_es310_devices(void)
{
	/* Run the array and install devices as appropriate */
	i2c_register_board_info(msm_i2c_audiosubsystem.bus,
				msm_i2c_audiosubsystem.info,
				msm_i2c_audiosubsystem.len);
}


void __init xiaomi_add_sound_devices(void)
{
	xiaomi_add_i2c_es310_devices();
}
